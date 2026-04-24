"""
Serial Monitor for ESP32 Radar Tracker
Reads and displays radar data from the serial port
"""
import serial
import serial.tools.list_ports
import time
import sys

# Configuration
BAUD_RATE = 115200
TIMEOUT = 2

def find_serial_port():
    """Prompt user for COM port or auto-detect"""
    ports = serial.tools.list_ports.comports()
    if not ports:
        port = input("No ports found. Enter port manually (e.g., COM5 or /dev/ttyUSB0): ").strip()
        return port if port else "COM5"

    print("\nAvailable serial ports:")
    for i, p in enumerate(ports, 1):
        print(f"  {i}. {p.device} - {p.description}")

    while True:
        choice = input(f"\nSelect port (1-{len(ports)}) or enter path manually [default: 1]: ").strip()

        if not choice:
            return ports[0].device

        if choice.isdigit():
            idx = int(choice) - 1
            if 0 <= idx < len(ports):
                return ports[idx].device
            else:
                print(f"Please enter a number between 1 and {len(ports)}.")
                continue

        return choice

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
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    port = find_serial_port()
    monitor_serial(port)
