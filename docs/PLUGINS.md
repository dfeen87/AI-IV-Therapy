# Plugin System

This document defines the **official plugin architecture** for the AI-Optimized Intravenous Therapy Control System.

The plugin system is part of the **v1.0 stable API**.  
All interfaces documented here are considered **locked** for this release.

---

## Purpose

Plugins provide a **controlled extension mechanism** for:

- Instrumentation and logging
- Safety enforcement
- Session recording and audit trails
- Research experimentation
- Integration testing

Plugins are designed to **observe and harden** the system without modifying the core control architecture.

---

## Design Principles

All plugins must adhere to the following rules:

- Plugins are **optional**
- Plugins must **fail safely**
- Plugins must **not access the DOM**
- Plugins must **not introduce nondeterminism**
- The core control model must remain **pure and testable**
- Plugin execution must **never halt the control loop**

The system prioritizes **safety, auditability, and determinism** over flexibility.

---

## Plugin Lifecycle

Plugins hook into the control loop through well-defined lifecycle callbacks.

Execution order is deterministic and consistent across updates.

### Available Hooks

```text
onInit
onTelemetry
onModelComputed
onBeforeRender
onAfterRender
onError
```

---

## Plugin Interface

```javascript
class AIIVPlugin {
    name = 'UnnamedPlugin';

    onInit(ctx) {}
    onTelemetry(telemetry, ctx) {}
    onModelComputed(model, ctx) {}
    onBeforeRender(telemetry, model, ctx) {}
    onAfterRender(telemetry, model, ctx) {}
    onError(error, ctx) {}
}
```

All hooks are optional.

---

## Hook Semantics

### onInit(ctx)

Called once when the plugin is registered.

Use for:

- Initialization
- Self-checks
- Capability logging

### onTelemetry(telemetry, ctx)

Called immediately after telemetry acquisition.

Use for:

- Validation
- Logging
- Signal quality checks

Must not modify telemetry in place.

### onModelComputed(model, ctx)

Called after the control model is evaluated.

Use for:

- Safety clamping
- Risk observation
- Derived metrics

Model mutation is permitted only for safety enforcement and must be logged.

### onBeforeRender(telemetry, model, ctx)

Called before UI rendering.

Use for:

- Annotation
- Visualization metadata
- Debug overlays (non-DOM)

### onAfterRender(telemetry, model, ctx)

Called after rendering completes.

Use for:

- Session recording
- Export buffers
- Performance instrumentation

### onError(error, ctx)

Called if an exception occurs in the control loop.

Use for:

- Error logging
- Safe shutdown signaling
- Research diagnostics

---

## Plugin Registration

Plugins are registered explicitly:

```javascript
PluginManager.register({
    name: 'ExamplePlugin',
    onModelComputed(model) {
        console.log(model.infusionRate);
    }
});
```

Registration order defines execution order.

---

## Execution Safety

All plugin callbacks are executed inside guarded try/catch blocks.

A plugin failure:

- Is logged
- Does not propagate
- Does not interrupt the control loop

This guarantees system stability even under experimental extensions.

---

## Example Plugins

### Telemetry Logger

```javascript
PluginManager.register({
    name: 'TelemetryLogger',
    onModelComputed(model) {
        if (model.riskScore > 0.5) {
            console.warn('[AI-IV] Elevated risk detected', model);
        }
    }
});
```

### Session Recorder

```javascript
PluginManager.register({
    name: 'SessionRecorder',
    buffer: [],
    onAfterRender(telemetry, model) {
        this.buffer.push({ t: Date.now(), telemetry, model });
    }
});
```

### Safety Clamp

```javascript
PluginManager.register({
    name: 'SafetyClamp',
    onModelComputed(model) {
        if (model.infusionRate > 1.4) {
            model.infusionRate = 1.4;
            console.warn('[AI-IV] Infusion rate clamped for safety');
        }
    }
});
```

---

## Versioning and Stability

- Plugin hooks documented here are stable as of v1.0
- Breaking changes will only occur in a major version update
- New hooks may be added in minor releases without removing existing hooks

---

## Intended Use

The plugin system is intended for:

- Pre-clinical research
- Simulation instrumentation
- Safety experimentation
- Algorithm comparison
- C++ parity validation

It is **not** intended for:

- Production medical deployment
- UI modification
- Runtime configuration management

---

## Summary

The plugin system provides a disciplined, auditable extension surface that preserves the integrity of the AI-IV control model while enabling research flexibility.

This architecture is a foundational component of the v1.0 release.
