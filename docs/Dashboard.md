📊 Dashboard Deep Dive
Real-Time Visualization Components
1. Status Bar
Quick-glance system overview updated every 200ms:

System Status: ACTIVE / CAUTION / HIGH RISK (color-coded)
Current Infusion Rate: Real-time ml/min with trend indicator
24-Hour Cumulative Volume: Running total with safety threshold
Session Time: Elapsed time since system start

2. Vital Signs Panel
Six critical parameters with intelligent status indicators:

Hydration: 🟢 >65% | 🟡 60-65% | 🔴 <60%
Heart Rate: 🟢 60-100 bpm | 🟡 100-120 | 🔴 >120 or <50
SpO₂: 🟢 >95% | 🟡 90-95% | 🔴 <90%
Temperature: 🟢 36.5-37.5°C | 🟡 edges | 🔴 <35 or >39°C
Lactate: 🟢 <3 mmol/L | 🟡 3-4 | 🔴 >4
Cardiac Output: Continuously monitored (5±1 L/min normal)

3. Energy Transfer Dynamics
Complete visualization of Section 4.1 physics:

Energy Proxy (E_T): Visual bar chart (0-1 scale)
Absolute Energy Transfer: W/kg real-time calculation
Flow Velocity: Estimated from cardiac output (target: 15-25 cm/s)
Flow Efficiency G(v): Gaussian term showing delivery optimization
Metabolic Load: Composite stress indicator
Tissue Efficiency (η): Perfusion-adjusted absorption

4. Risk Assessment Dashboard
Multi-domain risk quantification:

Composite Risk Score: Weighted combination (0-1 scale)

60% Critical factors (blood loss, hypoxia, hypothermia)
30% Metabolic factors (dehydration, energy depletion)
10% Thermal stress (hyperthermia)


Cardiac Reserve: Real-time headroom calculation
Signal Coherence (σ): Data quality and reliability metric

5. Control System Panel
Transparent AI decision-making:

Desired Rate: Controller output before safety constraints
Actual Rate: Safety-limited commanded rate
Confidence: Inverse of state uncertainty
Hydration Urgency: Nonlinear deficit calculation
Energy Need: Gap × metabolic load amplification

6. Predictive Outlook (10-Minute Horizon)
Forward state estimation:

Predicted Hydration: Linear extrapolation with uncertainty
Predicted Energy_T: Trend-based forecast
Recommended Action: "Nominal" vs "⚠️ Preemptive boost +20%"

7. Trend Charts (30-Minute Rolling Window)
Two synchronized time-series plots:

Chart A: Hydration (%) and Energy_T (scaled 0-100)
Chart B: Infusion Rate (ml/min) and Cardiac Reserve (0-1)
Update interval: 10 seconds (reduce visual noise while maintaining responsiveness)

8. Alert Panel
Hierarchical warning system:

🟢 No Alerts: "✓ All systems nominal"
🟡 Warnings: Moderate deviations (hydration <60%, HR >100)
🔴 Critical: Immediate attention required (cardiac reserve <0.3, risk >0.75)
Auto-prioritizes most severe alert
Actionable guidance ("Infusion limited due to low cardiac reserve")

Dashboard Modes
Mode 1: Standalone Simulation (Current Implementation)

Embedded physiological simulator
Matches C++ control algorithm exactly
Self-contained demonstration (no server required)
Perfect for: Education, training, regulatory demos

Mode 2: Live Clinical Integration (Future Enhancement)
javascript// WebSocket connection to C++ backend
const ws = new WebSocket('ws://localhost:8080/ai_iv_stream');
ws.onmessage = (event) => {
    const telemetry = JSON.parse(event.data);
    updateDashboard(telemetry);
};
Mode 3: CSV Playback (Future Enhancement)

Import session logs for retrospective analysis
Variable-speed replay (0.5×, 1×, 2×, 5×)
Comparative analysis of multiple sessions
Quality improvement and research

Technical Details
Performance Optimization:

Chart updates throttled to 10-second intervals (reduce CPU load)
CSS backdrop-filter for glassmorphism effects (GPU-accelerated)
Minimal DOM manipulation (update text content only)
No external dependencies except Chart.js

Accessibility:

High contrast color scheme (WCAG AA compliant)
Status indicators use both color AND shape
Keyboard navigation support
Screen reader compatible labels

Responsive Design:

Desktop (1920×1080): Full 2-column grid layout
Tablet (768-1200px): Single column, cards stack
Mobile (≤768px): Optimized compact view
Touch-friendly controls (48px minimum tap targets)
