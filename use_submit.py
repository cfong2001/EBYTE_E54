import sys
try:
    from utils import submit
    submit(commit_message="feat: Add BIND I/O menu option to dynamically map physical buttons to UI functions", description="Adds a BIND I/O tool in the DEV OPTIONS menu to dynamically assign functions to physical inputs (encoder press, encoder long press, key0 press, key0 long press). User inputs map to standard functions like Select, Back, Guide, and Tooltip, and timeout smoothly after 30 seconds of inactivity.")
except Exception as e:
    print(f"Failed to submit: {e}")
