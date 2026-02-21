#!/usr/bin/env python3
"""
Train and export the AI-IV sensor fusion neural network.

This script trains a small feedforward neural network that learns to
replicate the hand-crafted energy proxy formula (StateEstimator::
calculate_energy_proxy) via knowledge distillation.  Once trained, the
model is exported to two interchange formats:

  models/sensor_fusion_fdeep.json   — loaded at runtime by the C++ build
                                      via frugally-deep (libfdeep-dev)
  models/sensor_fusion.tflite       — portable TFLite flatbuffer for
                                      embedded / mobile deployment
  models/sensor_fusion.h5           — Keras HDF5 checkpoint

Model architecture (241 parameters):
  Input  (5 features)  →  Dense-16 ReLU  →  Dense-8 ReLU  →  Dense-1 Sigmoid

Input features (all normalised to [0, 1]):
  hydration_pct / 100      heart_rate_bpm / 200
  spo2_pct / 100           lactate_mmol / 20
  fatigue_idx              (already in [0, 1])

Output: E_T  (energy proxy, 0 = depleted → 1 = fully replenished)

Usage:
  pip install tensorflow numpy
  python3 tools/train_sensor_fusion_model.py

The generated models/sensor_fusion_fdeep.json is committed to the repo so
the C++ build does not require Python at runtime.  Re-run this script only
when retraining is needed (e.g. after tuning the reference formula or
architecture).
"""

import os
import sys
import math
import json
import base64
import hashlib
import argparse
import numpy as np

# ─────────────────────────────────────────────────────────────────────────────
# Reference formula (knowledge-distillation target)
# Mirrors StateEstimator::calculate_energy_proxy exactly.
# ─────────────────────────────────────────────────────────────────────────────

def _sigmoid(x: float, center: float, steepness: float) -> float:
    return 1.0 / (1.0 + math.exp(-steepness * (x - center)))

def _exp_decay(x: float, rate: float) -> float:
    return math.exp(-rate * x)

def reference_energy_proxy(
    hydration_pct: float,
    heart_rate_bpm: float,
    spo2_pct: float,
    lactate_mmol: float,
    fatigue_idx: float,
    blood_loss_idx: float = 0.0,
) -> float:
    """Exact copy of StateEstimator::calculate_energy_proxy (C++).

    blood_loss_idx defaults to 0.0 because the neural network does not take
    blood loss as an input feature — blood loss is already captured by the
    composite risk score (R_critical) in a separate SafetyMonitor pathway and
    should not be conflated with the energy proxy.  If blood loss is ever added
    as an input feature the model must be retrained with a 6-feature input layer.
    """
    h = _sigmoid(hydration_pct, 60.0, 0.1)
    b = _exp_decay(blood_loss_idx, 3.0)
    f = (1.0 - fatigue_idx) if fatigue_idx < 0.7 else 0.3 * (1.0 - fatigue_idx)
    o = _sigmoid(spo2_pct, 92.0, 0.3)
    l = _exp_decay(max(0.0, lactate_mmol - 2.0), 0.5)
    raw = 0.30 * h + 0.25 * b + 0.20 * f + 0.15 * o + 0.10 * l
    return float(min(1.0, max(0.0, raw)))


# ─────────────────────────────────────────────────────────────────────────────
# fdeep JSON helpers
# ─────────────────────────────────────────────────────────────────────────────

def _encode_f32(arr) -> str:
    """Encode a float array to base64 float32 little-endian (fdeep format)."""
    return base64.b64encode(
        np.asarray(arr, dtype=np.float32).ravel().tobytes()
    ).decode("ascii")


def _export_fdeep(model, out_path: str) -> None:
    """
    Write a frugally-deep 0.15.21-compatible JSON model file.

    fdeep stores Dense weights as a row-major (n_in, n_out) float32 array
    followed immediately by a (n_out,) bias array — both base64-encoded.
    The architecture block must use the Keras Functional / Model JSON format
    with explicit InputLayer, input_layers and output_layers fields.
    """
    import tensorflow as tf

    def _layer_weights(name):
        W, b = model.get_layer(name).get_weights()
        # Keras kernel shape: (n_in, n_out), stored row-major in fdeep
        return _encode_f32(W.ravel()), _encode_f32(b)

    w1, b1 = _layer_weights("hidden1")
    w2, b2 = _layer_weights("hidden2")
    w3, b3 = _layer_weights("energy_output")

    # Build a single test vector for fdeep self-verification
    test_in  = np.array([[0.80, 0.375, 0.98, 0.10, 0.30]], dtype=np.float32)
    test_out = model.predict(test_in, verbose=0).ravel()

    doc = {
        "architecture": {
            "class_name": "Model",
            "config": {
                "name": "sensor_fusion_energy",
                "layers": [
                    {
                        "class_name": "InputLayer",
                        "config": {
                            "batch_input_shape": [None, 5],
                            "dtype": "float32",
                            "sparse": False,
                            "name": "telemetry_input",
                        },
                        "inbound_nodes": [],
                        "name": "telemetry_input",
                    },
                    {
                        "class_name": "Dense",
                        "config": {
                            "name": "hidden1", "trainable": True,
                            "dtype": "float32", "units": 16,
                            "activation": "relu", "use_bias": True,
                        },
                        "inbound_nodes": [[["telemetry_input", 0, 0, {}]]],
                        "name": "hidden1",
                    },
                    {
                        "class_name": "Dense",
                        "config": {
                            "name": "hidden2", "trainable": True,
                            "dtype": "float32", "units": 8,
                            "activation": "relu", "use_bias": True,
                        },
                        "inbound_nodes": [[["hidden1", 0, 0, {}]]],
                        "name": "hidden2",
                    },
                    {
                        "class_name": "Dense",
                        "config": {
                            "name": "energy_output", "trainable": True,
                            "dtype": "float32", "units": 1,
                            "activation": "sigmoid", "use_bias": True,
                        },
                        "inbound_nodes": [[["hidden2", 0, 0, {}]]],
                        "name": "energy_output",
                    },
                ],
                "input_layers":  [["telemetry_input", 0, 0]],
                "output_layers": [["energy_output",   0, 0]],
            },
        },
        "image_data_format": "channels_last",
        "input_shapes":  [[5]],
        "output_shapes": [[1]],
        "tests": [
            {
                "inputs":  [{"shape": [5], "values": [_encode_f32(test_in[0])]}],
                "outputs": [{"shape": [1], "values": [_encode_f32(test_out)]}],
            }
        ],
        "trainable_params": {
            "hidden1":       {"weights": [w1], "bias": [b1]},
            "hidden2":       {"weights": [w2], "bias": [b2]},
            "energy_output": {"weights": [w3], "bias": [b3]},
        },
    }
    doc["hash"] = hashlib.sha256(
        json.dumps(doc, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()

    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(doc, f, indent=2)
    print(f"  fdeep JSON   → {out_path}  ({os.path.getsize(out_path):,} bytes)")


# ─────────────────────────────────────────────────────────────────────────────
# Training
# ─────────────────────────────────────────────────────────────────────────────

def generate_dataset(n_samples: int = 10_000, seed: int = 42):
    rng = np.random.default_rng(seed)
    H   = rng.uniform(30,  100, n_samples)
    HR  = rng.uniform(40,  160, n_samples)
    S   = rng.uniform(80,  100, n_samples)
    L   = rng.uniform(0,    12, n_samples)
    F   = rng.uniform(0,     1, n_samples)
    y   = np.array([
        reference_energy_proxy(H[i], HR[i], S[i], L[i], F[i])
        for i in range(n_samples)
    ], dtype=np.float32)
    X = np.column_stack([H / 100.0, HR / 200.0, S / 100.0, L / 20.0, F]).astype(np.float32)
    return X, y.reshape(-1, 1)


def build_model():
    import tensorflow as tf
    inp = tf.keras.Input(shape=(5,), name="telemetry_input")
    x   = tf.keras.layers.Dense(16, activation="relu",    name="hidden1")(inp)
    x   = tf.keras.layers.Dense(8,  activation="relu",    name="hidden2")(x)
    out = tf.keras.layers.Dense(1,  activation="sigmoid", name="energy_output")(x)
    model = tf.keras.Model(inputs=inp, outputs=out, name="sensor_fusion_energy")
    model.compile(
        optimizer="adam",
        loss="mse",
        metrics=[tf.keras.metrics.MeanAbsoluteError(name="mae")],
    )
    return model


def train(epochs: int = 100, batch_size: int = 256, seed: int = 42):
    import tensorflow as tf
    tf.random.set_seed(seed)

    print("Generating training data …")
    X, y = generate_dataset(seed=seed)

    model = build_model()
    model.summary()

    print(f"\nTraining {epochs} epochs …")
    model.fit(
        X, y,
        epochs=epochs,
        batch_size=batch_size,
        validation_split=0.2,
        verbose=0,
    )
    # Target MAE threshold — must match test_neural_estimator.cpp spot-check tolerance
    MAE_TARGET = 0.05

    val_loss, val_mae = model.evaluate(X[8000:], y[8000:], verbose=0)
    print(f"  Val MAE: {val_mae:.5f}  (< {MAE_TARGET} target)")
    if val_mae > MAE_TARGET:
        print("  WARNING: val MAE exceeds target — consider more epochs or a larger model.")
    return model


def export_all(model, out_dir: str = "models") -> None:
    import tensorflow as tf

    os.makedirs(out_dir, exist_ok=True)

    # 1. Keras HDF5 checkpoint (source of truth for retraining)
    h5_path = os.path.join(out_dir, "sensor_fusion.h5")
    model.save(h5_path)
    print(f"  Keras H5     → {h5_path}  ({os.path.getsize(h5_path):,} bytes)")

    # 2. TFLite flatbuffer (portable, for embedded / mobile deployment)
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite = converter.convert()
    tflite_path = os.path.join(out_dir, "sensor_fusion.tflite")
    with open(tflite_path, "wb") as f:
        f.write(tflite)
    print(f"  TFLite       → {tflite_path}  ({len(tflite):,} bytes)")

    # Verify TFLite inference matches Keras
    interp = tf.lite.Interpreter(model_content=tflite)
    interp.allocate_tensors()
    inp_d = interp.get_input_details()
    out_d = interp.get_output_details()
    test_in  = np.array([[0.80, 0.375, 0.98, 0.10, 0.30]], dtype=np.float32)
    keras_out = model.predict(test_in, verbose=0)[0][0]
    interp.set_tensor(inp_d[0]["index"], test_in)
    interp.invoke()
    tflite_out = interp.get_tensor(out_d[0]["index"])[0][0]
    diff = abs(keras_out - tflite_out)
    print(f"  TFLite self-check: keras={keras_out:.4f} tflite={tflite_out:.4f} diff={diff:.5f}")
    assert diff < 1e-4, "TFLite output diverges from Keras"

    # 3. frugally-deep JSON (loaded by C++ runtime via -DENABLE_NEURAL_ESTIMATOR)
    fdeep_path = os.path.join(out_dir, "sensor_fusion_fdeep.json")
    _export_fdeep(model, fdeep_path)


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Train AI-IV sensor fusion model")
    parser.add_argument("--epochs",     type=int, default=100)
    parser.add_argument("--batch-size", type=int, default=256)
    parser.add_argument("--out-dir",    default="models")
    args = parser.parse_args()

    print("=== AI-IV Sensor Fusion Model Training ===\n")
    model = train(epochs=args.epochs, batch_size=args.batch_size)
    print("\nExporting …")
    export_all(model, out_dir=args.out_dir)
    print("\nDone.")


if __name__ == "__main__":
    main()
