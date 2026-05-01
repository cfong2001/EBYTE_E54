"""
Serial Monitor for ESP32 Radar Tracker
Reads and displays radar data from the serial port
"""
import serial
import serial.tools.list_ports
import time
import sys
import re
import os
import stat

# Configuration
BAUD_RATE = 115200
TIMEOUT = 2

def is_valid_port(port):
    """Validate if the port is a safe serial port pattern or an available port."""
    if not port or len(port) > 255:
        return False

    # Check if port is in available ports
    available_ports = [p.device for p in serial.tools.list_ports.comports()]
    if port in available_ports:
        return True

    # Check against safe regex patterns for Windows and Unix-like systems
    windows_pattern = re.compile(r'^COM[1-9][0-9]{0,3}$')
    unix_pattern = re.compile(r'^/dev/(tty|cu)[a-zA-Z0-9_.-]+$')

    if windows_pattern.match(port):
        return True

    if unix_pattern.match(port):
        # On Unix-like systems, further verify it's a character device if it exists
        if os.path.exists(port):
            try:
                mode = os.stat(port).st_mode
                return stat.S_ISCHR(mode)
            except (OSError, PermissionError):
                return False
        return True

    return False

def get_default_port():
    """Find an available port to use as default, or fallback to COM5."""
    available_ports = serial.tools.list_ports.comports()
    if available_ports:
        return available_ports[0].device
    return "COM5"

def find_serial_port():
    """Prompt user for COM port or auto-detect with validation."""
    default_port = get_default_port()

    while True:
        port = input(f"Enter COM port (e.g., {default_port}) or press Enter to use {default_port}: ").strip()
        port = port if port else default_port

        if is_valid_port(port):
            return port

        print(f"Warning: {repr(port)} does not appear to be a valid or safe serial port path.")
        action = input("Enter 'r' to retry, or 'q' to quit: ").strip().lower()
        if action == 'q':
            sys.exit(0)

def monitor_serial(port):
    """Monitor serial output from ESP32"""
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=TIMEOUT)
        print(f"Connected to {port} at {BAUD_RATE} baud")
        print("Reading radar data... (Press Ctrl+C to stop)\n")
        
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(line)
                    
    except serial.SerialException as e:
        print(f"Error: {e}")
        print("Make sure the device is connected and the COM port is correct.")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nMonitoring stopped.")
        ser.close()
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    port = find_serial_port()
    monitor_serial(port)
