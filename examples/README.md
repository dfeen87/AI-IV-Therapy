# REST API Client Examples

This directory contains example client applications demonstrating how to interact with the AI-IV Therapy Control System REST API.

## Prerequisites

- Python 3.6+ (for Python examples)
- `requests` library: `pip install requests`
- AI-IV system built with REST API support

## Building the Server with REST API

```bash
cd /home/runner/work/AI-IV-Therapy/AI-IV-Therapy
g++ -std=c++17 -O2 -pthread -DENABLE_REST_API \
    src/adaptive_iv_therapy_control_system.cpp \
    src/rest_api_server.cpp \
    -o ai_iv_with_api
```

## Starting the Server

```bash
./ai_iv_with_api
```

The REST API will be available at `http://localhost:8080/api/`

## Running the Examples

### Python Client Demo

```bash
python3 examples/rest_api_client.py
```

This demonstrates:
- System status checking
- Configuration retrieval
- Real-time telemetry monitoring
- Patient state observation
- Control output tracking
- Alert monitoring

### Manual Testing with curl

```bash
# Get system status
curl http://localhost:8080/api/status | python -m json.tool

# Get current telemetry
curl http://localhost:8080/api/telemetry | python -m json.tool

# Get patient state
curl http://localhost:8080/api/state | python -m json.tool

# Get control output
curl http://localhost:8080/api/control | python -m json.tool

# Get system configuration
curl http://localhost:8080/api/config | python -m json.tool

# Get recent alerts
curl http://localhost:8080/api/alerts | python -m json.tool

# Get telemetry history
curl http://localhost:8080/api/telemetry/history | python -m json.tool
```

## Creating Your Own Client

The REST API provides simple JSON endpoints that can be accessed from any programming language. Here's a minimal example:

### Python
```python
import requests

response = requests.get('http://localhost:8080/api/telemetry')
data = response.json()
print(f"Hydration: {data['hydration_pct']}%")
```

### JavaScript (Node.js)
```javascript
const fetch = require('node-fetch');

async function getTelemetry() {
    const response = await fetch('http://localhost:8080/api/telemetry');
    const data = await response.json();
    console.log(`Hydration: ${data.hydration_pct}%`);
}
```

### Shell Script (bash)
```bash
#!/bin/bash
TELEMETRY=$(curl -s http://localhost:8080/api/telemetry)
echo "Telemetry: $TELEMETRY"
```

## See Also

- [REST API Documentation](../docs/REST_API.md) - Complete API reference
- [Main README](../README.md) - Project overview and build instructions
