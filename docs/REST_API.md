# REST API Documentation

## Overview

The AI-IV Therapy Control System includes an optional REST API for global network access to system telemetry, patient state, control decisions, and alerts. This API provides read-only observability into the running system without affecting control logic.

## Design Principles

- **Non-blocking**: Runs in a separate thread, does not affect control loop timing
- **Read-only**: GET endpoints only for safety (no control modifications via API)
- **Thread-safe**: All data access is protected with mutexes
- **Lightweight**: Standard POSIX sockets, no external dependencies
- **JSON responses**: Modern, machine-readable format

## Building with REST API Support

### Standard Build (without REST API)
```bash
g++ -std=c++17 -pthread src/adaptive_iv_therapy_control_system.cpp -o ai_iv
```

### Build with REST API Enabled
```bash
g++ -std=c++17 -pthread -DENABLE_REST_API \
    src/adaptive_iv_therapy_control_system.cpp \
    src/rest_api_server.cpp \
    -o ai_iv
```

## Running the Server

When built with REST API support, the server automatically starts on port **8080** and binds to **0.0.0.0** (all network interfaces), providing global network access.

```bash
./ai_iv
```

The API will be accessible at `http://<your-ip>:8080/api/`

## API Endpoints

### Root Endpoint
**GET** `/` or `/api/`

Returns available endpoints and service information.

**Example Response:**
```json
{
  "service": "AI-IV Therapy REST API",
  "version": "1.0.0",
  "endpoints": [
    "/api/status",
    "/api/telemetry",
    "/api/telemetry/history",
    "/api/control",
    "/api/state",
    "/api/alerts",
    "/api/config"
  ]
}
```

### System Status
**GET** `/api/status`

Returns current system status and health.

**Example Response:**
```json
{
  "status": "running",
  "timestamp": "2026-02-14T02:56:04.123Z",
  "api_version": "1.0.0",
  "system": "AI-IV Therapy Control System"
}
```

### Current Telemetry
**GET** `/api/telemetry`

Returns the most recent telemetry snapshot from sensors.

**Example Response:**
```json
{
  "timestamp": "2026-02-14T02:56:04.123Z",
  "hydration_pct": 72.50,
  "heart_rate_bpm": 75.30,
  "temp_celsius": 37.20,
  "spo2_pct": 98.00,
  "lactate_mmol": 2.10,
  "cardiac_output_L_min": 5.20
}
```

### Telemetry History
**GET** `/api/telemetry/history`

Returns historical telemetry data (last 1000 samples).

**Example Response:**
```json
{
  "history": [
    {
      "timestamp": "2026-02-14T02:55:00.000Z",
      "hydration_pct": 70.00,
      "heart_rate_bpm": 72.00,
      "temp_celsius": 37.00,
      "spo2_pct": 97.50,
      "lactate_mmol": 2.00,
      "cardiac_output_L_min": 5.00
    },
    ...
  ],
  "count": 1000
}
```

### Control State
**GET** `/api/control`

Returns current control output and decision rationale.

**Example Response:**
```json
{
  "timestamp": "2026-02-14T02:56:04.123Z",
  "infusion_rate_ml_min": 0.850,
  "rationale": "Adaptive control with risk amplification"
}
```

### Patient State
**GET** `/api/state`

Returns estimated patient physiological state.

**Example Response:**
```json
{
  "hydration_pct": 72.50,
  "energy_T": 0.650,
  "metabolic_load": 0.320,
  "cardiac_reserve": 0.750,
  "risk_score": 0.280
}
```

### Alerts
**GET** `/api/alerts`

Returns recent system alerts and warnings (last 100).

**Example Response:**
```json
{
  "alerts": [
    {
      "timestamp": "2026-02-14T02:55:30.000Z",
      "severity": "warning",
      "message": "Telemetry signal quality below threshold"
    },
    {
      "timestamp": "2026-02-14T02:54:15.000Z",
      "severity": "info",
      "message": "Infusion rate change limited by safety constraints"
    }
  ],
  "count": 2
}
```

### Configuration
**GET** `/api/config`

Returns current system configuration parameters.

**Example Response:**
```json
{
  "config": {
    "patient_weight_kg": "75.0",
    "patient_age_years": "35.0",
    "max_infusion_rate": "1.5",
    "baseline_hr_bpm": "70.0",
    "session_id": "1739498164"
  }
}
```

## Error Responses

All errors return appropriate HTTP status codes with JSON error messages:

**404 Not Found:**
```json
{
  "error": "Endpoint not found"
}
```

**405 Method Not Allowed:**
```json
{
  "error": "Method not allowed"
}
```

## Security Considerations

⚠️ **Important Security Notes:**

1. **No Authentication**: The current implementation does not include authentication. Do not expose this API to untrusted networks.

2. **Read-Only**: The API is intentionally read-only (GET only) to prevent unauthorized control modifications.

3. **Firewall Configuration**: In production environments, configure firewalls to restrict access:
   ```bash
   # Example: Allow only from specific IP
   iptables -A INPUT -p tcp --dport 8080 -s 192.168.1.0/24 -j ACCEPT
   iptables -A INPUT -p tcp --dport 8080 -j DROP
   ```

4. **HTTPS**: For production use, deploy behind a reverse proxy with HTTPS:
   ```nginx
   server {
       listen 443 ssl;
       server_name ai-iv.example.com;
       
       ssl_certificate /path/to/cert.pem;
       ssl_certificate_key /path/to/key.pem;
       
       location /api/ {
           proxy_pass http://localhost:8080/api/;
       }
   }
   ```

5. **Network Isolation**: Consider running on a dedicated medical device network isolated from public internet.

## Example Usage

### Command Line (curl)
```bash
# Get current status
curl http://localhost:8080/api/status

# Get current telemetry
curl http://localhost:8080/api/telemetry

# Get recent alerts
curl http://localhost:8080/api/alerts

# Pretty print JSON
curl http://localhost:8080/api/state | python -m json.tool
```

### Python Client
```python
import requests
import time

API_BASE = "http://localhost:8080/api"

# Monitor telemetry
while True:
    response = requests.get(f"{API_BASE}/telemetry")
    data = response.json()
    print(f"Hydration: {data['hydration_pct']}%, HR: {data['heart_rate_bpm']} bpm")
    time.sleep(1)
```

### JavaScript Client
```javascript
const API_BASE = 'http://localhost:8080/api';

async function getSystemStatus() {
    const response = await fetch(`${API_BASE}/status`);
    const data = await response.json();
    console.log('System Status:', data);
}

async function monitorTelemetry() {
    setInterval(async () => {
        const response = await fetch(`${API_BASE}/telemetry`);
        const data = await response.json();
        console.log(`Hydration: ${data.hydration_pct}%, HR: ${data.heart_rate_bpm} bpm`);
    }, 1000);
}
```

## Integration with Monitoring Systems

### Prometheus/Grafana
The JSON endpoints can be scraped and visualized:
```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'ai-iv'
    metrics_path: '/api/telemetry'
    static_configs:
      - targets: ['localhost:8080']
```

### ELK Stack
Use Logstash to ingest API data:
```ruby
input {
  http_poller {
    urls => {
      telemetry => "http://localhost:8080/api/telemetry"
    }
    schedule => { every => "1s" }
  }
}
```

## Performance

- **Response Time**: < 1ms for most endpoints
- **Memory Overhead**: ~1MB for history buffers
- **Thread Impact**: Minimal (separate thread, non-blocking)
- **Control Loop**: Zero impact on deterministic control timing

## Future Enhancements

Potential future additions (not yet implemented):
- Authentication (API keys, OAuth2)
- HTTPS support
- WebSocket streaming for real-time updates
- Rate limiting
- Request logging and analytics
- Historical data export endpoints
- System configuration updates (POST endpoints with authentication)

## Support

For issues or questions about the REST API, please refer to the main project documentation or open an issue on GitHub.
