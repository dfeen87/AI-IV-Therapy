#!/usr/bin/env python3
"""
Simple REST API client example for AI-IV Therapy Control System
Demonstrates real-time monitoring of telemetry and system state
"""

import requests
import time
import json
from datetime import datetime

API_BASE = "http://localhost:8080/api"

def print_header(text):
    print(f"\n{'='*60}")
    print(f"  {text}")
    print(f"{'='*60}\n")

def get_status():
    """Get system status"""
    response = requests.get(f"{API_BASE}/status")
    data = response.json()
    print(f"Status: {data['status']}")
    print(f"System: {data['system']}")
    print(f"API Version: {data['api_version']}")
    print(f"Timestamp: {data['timestamp']}")

def get_telemetry():
    """Get current telemetry"""
    response = requests.get(f"{API_BASE}/telemetry")
    data = response.json()
    print(f"Timestamp: {data['timestamp']}")
    print(f"Hydration: {data['hydration_pct']:.1f}%")
    print(f"Heart Rate: {data['heart_rate_bpm']:.0f} bpm")
    print(f"Temperature: {data['temp_celsius']:.1f}°C")
    print(f"SpO2: {data['spo2_pct']:.1f}%")
    print(f"Lactate: {data['lactate_mmol']:.2f} mmol/L")
    print(f"Cardiac Output: {data['cardiac_output_L_min']:.2f} L/min")

def get_state():
    """Get patient state"""
    response = requests.get(f"{API_BASE}/state")
    data = response.json()
    print(f"Hydration: {data['hydration_pct']:.1f}%")
    print(f"Energy T: {data['energy_T']:.3f}")
    print(f"Metabolic Load: {data['metabolic_load']:.3f}")
    print(f"Cardiac Reserve: {data['cardiac_reserve']:.3f}")
    print(f"Risk Score: {data['risk_score']:.3f}")

def get_control():
    """Get control output"""
    response = requests.get(f"{API_BASE}/control")
    data = response.json()
    print(f"Timestamp: {data['timestamp']}")
    print(f"Infusion Rate: {data['infusion_rate_ml_min']:.3f} ml/min")
    print(f"Rationale: {data['rationale']}")

def get_config():
    """Get system configuration"""
    response = requests.get(f"{API_BASE}/config")
    data = response.json()
    print("Configuration:")
    for key, value in data['config'].items():
        print(f"  {key}: {value}")

def get_alerts():
    """Get recent alerts"""
    response = requests.get(f"{API_BASE}/alerts")
    data = response.json()
    print(f"Total Alerts: {data['count']}")
    if data['alerts']:
        for alert in data['alerts'][-5:]:  # Last 5 alerts
            print(f"  [{alert['severity']}] {alert['timestamp']}: {alert['message']}")
    else:
        print("  No alerts")

def monitor_loop(duration_seconds=30, interval_seconds=2):
    """Monitor telemetry in real-time"""
    print_header("Real-Time Monitoring")
    print(f"Monitoring for {duration_seconds} seconds (interval: {interval_seconds}s)")
    print(f"Press Ctrl+C to stop\n")
    
    start_time = time.time()
    try:
        while time.time() - start_time < duration_seconds:
            response = requests.get(f"{API_BASE}/telemetry")
            telemetry = response.json()
            
            response = requests.get(f"{API_BASE}/control")
            control = response.json()
            
            print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                  f"Hydration: {telemetry['hydration_pct']:5.1f}% | "
                  f"HR: {telemetry['heart_rate_bpm']:3.0f} bpm | "
                  f"Temp: {telemetry['temp_celsius']:4.1f}°C | "
                  f"Infusion: {control['infusion_rate_ml_min']:5.3f} ml/min")
            
            time.sleep(interval_seconds)
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user")

def main():
    print_header("AI-IV Therapy REST API Client Demo")
    
    try:
        # Test connectivity
        print("Testing API connectivity...")
        response = requests.get(f"{API_BASE}/status", timeout=5)
        if response.status_code == 200:
            print("✓ Connected to AI-IV REST API\n")
        else:
            print(f"✗ Failed to connect (status: {response.status_code})")
            return
        
        # Display system information
        print_header("System Status")
        get_status()
        
        print_header("System Configuration")
        get_config()
        
        print_header("Current Telemetry")
        get_telemetry()
        
        print_header("Patient State")
        get_state()
        
        print_header("Control Output")
        get_control()
        
        print_header("Recent Alerts")
        get_alerts()
        
        # Real-time monitoring
        monitor_loop(duration_seconds=20, interval_seconds=2)
        
        print_header("Demo Complete")
        print("For more information, see docs/REST_API.md")
        
    except requests.exceptions.ConnectionError:
        print("\n✗ Error: Cannot connect to REST API server")
        print("  Make sure the AI-IV system is running with REST API enabled:")
        print("  ./ai_iv_with_api")
    except Exception as e:
        print(f"\n✗ Error: {e}")

if __name__ == "__main__":
    main()
