#!/usr/bin/env python3
"""
CircuitPython Code Deployment Utility
Deploys CircuitPython code to ESP32 via serial REPL
"""
import stat
import serial
import serial.tools.list_ports
import time
import os
import sys
import re


def is_valid_port(port):
    """Validate if the port is a safe serial port pattern or an available port."""
    if len(port) > 255:
        return False

    available_ports = [p.device for p in serial.tools.list_ports.comports()]
    if port in available_ports:
        return True

    windows_pattern = re.compile(r'^COM[1-9][0-9]{0,3}$')
    unix_pattern = re.compile(r'^/dev/(tty|cu)[a-zA-Z0-9_.-]+$')

    if windows_pattern.match(port):
        return True

    if unix_pattern.match(port):
        try:
            mode = os.stat(port).st_mode
            if stat.S_ISCHR(mode):
                return True
        except (OSError, FileNotFoundError):
            pass

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
        while True:
            action = input("Enter 'r' to retry, or 'q' to quit: ").strip().lower()
            if action == 'q':
                sys.exit(0)
            elif action == 'r':
                break

def list_circuitpython_files(base_dir):
    """List available CircuitPython code files"""
    cp_dir = os.path.join(base_dir, 'circuitpython')
    if not os.path.exists(cp_dir):
        print(f"Error: CircuitPython directory not found at {cp_dir}")
        return []
    
    files = [f for f in os.listdir(cp_dir) if f.endswith('.py')]
    return sorted(files)

def deploy_code(port, code_file_path, monitor_duration=10):
    """Deploy CircuitPython code to ESP32"""
    try:
        # Read the code file
        with open(code_file_path, 'r') as f:
            code_content = f.read()
        
        # Open serial connection
        ser = serial.Serial(port, 115200, timeout=2)
        print(f"Connected to {port}")
        print(f"Deploying: {os.path.basename(code_file_path)}")
        
        # Stop current program with Ctrl+C
        ser.write(b'\x03\r')
        time.sleep(0.5)
        
        # Clear input buffer
        while ser.in_waiting:
            ser.read(ser.in_waiting)
        
        print("Writing code to /code.py...")
        
        # Open file in REPL
        ser.write(b"with open('/code.py', 'w') as f:\r\n")
        time.sleep(0.2)
        
        # Write code line by line
        line_count = 0
        for line in code_content.split('\n'):
            # Escape backslashes and single quotes
            escaped = line.replace('\\', '\\\\').replace("'", "\\'")
            cmd = f"    f.write('{escaped}\\n')\r\n"
            ser.write(cmd.encode())
            time.sleep(0.02)  # Small delay to prevent buffer overflow
            line_count += 1
            
            # Progress indicator every 50 lines
            if line_count % 50 == 0:
                print(f"  Written {line_count} lines...")
        
        print(f"  Written {line_count} lines total")
        
        # Close the file writing block
        ser.write(b"\r\n")
        time.sleep(0.5)
        
        # Reload with Ctrl+D
        print("Reloading ESP32...")
        ser.write(b'\x04')
        time.sleep(3)
        
        # Monitor output
        print("\n" + "="*60)
        print("Radar Output (monitoring for {} seconds):".format(monitor_duration))
        print("="*60)
        
        start_time = time.time()
        while time.time() - start_time < monitor_duration:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(line)
        
        print("="*60)
        print("\n✓ Deployment complete!")
        print("Wave your hand in front of the radar to test detection")
        
        ser.close()
        return True
        
    except FileNotFoundError:
        print(f"Error: Code file not found at {code_file_path}")
        return False
    except serial.SerialException as e:
        print(f"Error: Serial connection failed - {e}")
        print("Check that:")
        print("  - The ESP32 is connected")
        print("  - The COM port is correct")
        print("  - No other program is using the port")
        return False
    except OSError as e:
        print(f"Error: System or I/O error occurred - {e}")
        return False
    except KeyboardInterrupt:
        print("\nDeployment interrupted by user.")
        return False
    except Exception as e:
        print("Error: An unexpected error occurred during deployment.")
        return False

def main():
    """Main deployment workflow"""
    print("="*60)
    print("CircuitPython Deployment Tool")
    print("="*60)
    
    # Get base directory (one level up from utils)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    base_dir = os.path.dirname(script_dir)
    
    # List available code files
    files = list_circuitpython_files(base_dir)
    if not files:
        print("No CircuitPython files found!")
        return 1
    
    print("\nAvailable CircuitPython files:")
    for i, f in enumerate(files, 1):
        print(f"  {i}. {f}")
    
    # Select file
    while True:
        try:
            choice = input(f"\nSelect file (1-{len(files)}): ").strip()
            idx = int(choice) - 1
            if 0 <= idx < len(files):
                selected_file = files[idx]
                break
            print(f"Please enter a number between 1 and {len(files)}")
        except ValueError:
            print("Please enter a valid number")
        except KeyboardInterrupt:
            print("\n\nCancelled")
            return 0
    
    # Get COM port
    print()
    port = find_serial_port()
    
    # Get monitoring duration
    try:
        duration = input("\nMonitor duration in seconds (default: 10): ").strip()
        duration = int(duration) if duration else 10
    except ValueError:
        duration = 10
    
    # Deploy
    print()
    code_file_path = os.path.join(base_dir, 'circuitpython', selected_file)
    success = deploy_code(port, code_file_path, duration)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
