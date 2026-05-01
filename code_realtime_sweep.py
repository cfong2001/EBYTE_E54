# ESP32-S3 ProS3 - HLK-LD2450 Radar Tracker
# FULL SCREEN with DATA-DRIVEN sweep (no artificial rotation)
# Advanced protocol (0xAA 0x55)

import time, gc, math
import board, busio, neopixel
from shared.ui_utils import draw_dotted_circle, map_xy
import adafruit_ssd1306
from utils import s16_le

# ----------------- Hardware config -----------------
i2c = None
oled = None
uart = None
pixel = None

def init_hardware():
    global i2c, oled, uart, pixel
    i2c = board.STEMMA_I2C()
    oled = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c, addr=0x3D)

    uart = busio.UART(board.IO43, board.IO44, baudrate=256000,
                     receiver_buffer_size=8192, timeout=0)

    pixel = neopixel.NeoPixel(board.NEOPIXEL, 1, brightness=0.2, auto_write=True)

    print("Full Screen Radar - Data-Driven Sweep")

# ----------------- Protocol -----------------
SYNC = b"\xAA\x55"

# ----------------- Display Config - FULL SCREEN -----------------
CX = 64   # Center X
CY = 63   # Bottom of screen

# Range circles - full screen radii
RADII = [20, 40, 60]  # pixels

# Mapping functions - FULL SCREEN

# Target tracking
targets = []
filter_states = {} # State dict for multi-anchor stabilization
buffer = bytearray()
last_draw = time.monotonic()
frame_count = 0
bytes_in = 0
color_wheel = 0

def draw_grid():
    """Draw full-screen grid with range circles"""
    for r in RADII:
        draw_dotted_circle(oled, CX, CY, r)
    
    # Draw horizontal baseline
    oled.line(0, CY, 127, CY, 1)

def draw_sweep_to_target(x_mm, y_mm):
    """Draw sweep line from center to target position"""
    # Calculate angle from radar coordinates
    angle_rad = math.atan2(-y_mm, -x_mm)
    angle_deg = math.degrees(angle_rad) + 90
    
    if 0 <= angle_deg <= 180:
        # Draw line from center to max range in target direction
        angle = math.radians(angle_deg - 90)
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        for r in range(0, RADII[-1] + 1, 2):
            x = int(CX + r * cos_a)
            y = int(CY + r * sin_a)
            if 0 <= x < 128 and 0 <= y < 64:
                oled.pixel(x, y, 1)

def draw_targets(target_list):
    """Draw targets and sweep lines to them"""
    for (x_mm, y_mm, age, idx) in target_list:
        if age > 10:
            continue
        
        # Draw sweep line to this target (if recent)
        if age < 3:
            draw_sweep_to_target(x_mm, y_mm)
        
        # Map to FULL screen coordinates
        sx, sy = map_xy(x_mm, y_mm)
        
        # Draw target based on index
        if idx == 0:  # Primary - filled square
            oled.fill_rect(sx-2, sy-2, 5, 5, 1)
        elif idx == 1:  # Secondary - hollow square
            oled.rect(sx-2, sy-2, 5, 5, 1)
        else:  # Tertiary - cross
            oled.line(sx-2, sy, sx+2, sy, 1)
            oled.line(sx, sy-2, sx, sy+2, 1)

def wheel(n):
    """Color wheel for NeoPixel"""
    n %= 255
    if n < 85:   return (255-n*3, n*3, 0)
    if n < 170:  n-=85; return (0, 255-n*3, n*3)
    n-=170;      return (n*3, 0, 255-n*3)

def draw_display():
    """Draw complete full-screen radar"""
    global color_wheel
    
    # Clear screen
    oled.fill(0)
    
    # Draw grid first
    draw_grid()
    
    # Count active targets
    active_count = 0
    closest_dist_sq = 999999999.0
    
    for target_data in targets:
        x_mm, y_mm, age, idx = target_data
        
        if age < 5:
            dist_sq = x_mm * x_mm + y_mm * y_mm
            if dist_sq < closest_dist_sq:
                closest_dist_sq = dist_sq
            active_count += 1
    
    # Draw targets and sweep lines
    draw_targets(targets)
    
    # Status at bottom right (no title in upper left)
    if active_count > 0:
        closest_dist = math.sqrt(closest_dist_sq) / 1000.0
        status = "%.1fm [%d]" % (closest_dist, active_count)
        oled.text(status, 70, 56, 1)
        pixel[0] = (50, 0, 0)  # Red when targets
    else:
        pixel[0] = wheel(color_wheel)
        color_wheel = (color_wheel + 3) % 255
    
    oled.show()

# Main loop
def main():
    global targets, buffer, last_draw, frame_count, bytes_in, color_wheel, filter_states
    last_stat = time.monotonic()
    frames_ok = 0

    while True:
        # Read UART
        if uart.in_waiting:
            data = uart.read(uart.in_waiting)
            bytes_in += len(data)
            buffer.extend(data)

        # Process frames - Advanced protocol (0xAA 0x55)
        while len(buffer) >= 10:
            idx = buffer.find(SYNC)
            if idx < 0:
                if len(buffer) > 100:
                    buffer = buffer[-50:]
                break

            if idx > 0:
                buffer = buffer[idx:]

            if len(buffer) < 6:
                break

            # Get frame length
            length = buffer[2] | (buffer[3] << 8)

            if length < 10 or length > 512:
                buffer = buffer[2:]
                continue

            if len(buffer) < length:
                break

            frame = buffer[0:length]
            buffer = buffer[length:]

            # Parse frame for targets
            frames_ok += 1
            raw_targets = []

            # Extract targets from payload (starts at byte 6)
            if len(frame) > 7:
                count = min(3, frame[6])
                offset = 7

                for target_idx in range(count):
                    if offset + 6 <= len(frame) - 2:
                        x = s16_le(frame[offset], frame[offset + 1])
                        y = s16_le(frame[offset + 2], frame[offset + 3])
                        s = s16_le(frame[offset + 4], frame[offset + 5])

                        dist_sq = x * x + y * y
                        if (x != 0 or y != 0) and (10000 < dist_sq < 64000000):
                            raw_targets.append({'id': target_idx, 'active': True, 'x': x, 'y': y, 'speed': s})
                        else:
                            raw_targets.append({'id': target_idx, 'active': False, 'x': 0, 'y': 0, 'speed': 0})
                        offset += 6

            # Apply Multi-Anchor Stabilization
            from utils import calculate_multi_anchor_stabilization
            stabilized = calculate_multi_anchor_stabilization(raw_targets, filter_states, dt=0.1)

            new_targets = []
            for t in stabilized:
                if t['active']:
                    new_targets.append((t['x'], t['y'], 0, t['id']))

            # Age existing targets and merge
            aged_targets = [(x, y, age + 1, idx) for x, y, age, idx in targets if age < 15]

            # Merge: keep new targets, add aged ones not overlapping
            targets = new_targets[:]
            for old_target in aged_targets:
                ox, oy, oage, oidx = old_target
                is_duplicate = False
                for nx, ny, _, _ in new_targets:
                    dist_sq = (ox - nx)**2 + (oy - ny)**2
                    if dist_sq < 90000:
                        is_duplicate = True
                        break
                if not is_duplicate:
                    targets.append(old_target)

            targets = targets[:10]

        # Update display at 20 FPS
        now = time.monotonic()
        if now - last_draw > 0.05:
            draw_display()
            last_draw = now

        # Status report every second
        if now - last_stat > 1.0:
            print("RX {:5d} B/s  frames {:3d}/s".format(bytes_in, frames_ok))
            bytes_in = 0
            frames_ok = 0
            last_stat = now

        time.sleep(0.001)

if __name__ == '__main__':
    init_hardware()
    main()
