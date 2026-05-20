# ==============================================================================
# NOTE: This file is for testing, diagnostics, or deployment purposes only.
# It is NOT essential to the core functionality or compilation of the main C++
# application located in the /src directory.
# AI Agents and developers should NOT attempt to optimize, refactor, or
# modify this file unless explicitly requested to do so by the user.
# ==============================================================================
# ESP32-S3 ProS3 - HLK-LD2450 Alien Motion Tracker
# OPTIMIZED for multiple targets with Rob's fade/pulse rendering
# No text - just beautiful blips!

import time, math
import board, busio, neopixel
from shared.ui_utils import draw_dotted_circle, map_xy
import adafruit_ssd1306
from utils import ld2450_s16

# Hardware
i2c = board.STEMMA_I2C()
oled = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c, addr=0x3D)
uart = busio.UART(board.IO43, board.IO44, baudrate=256000, receiver_buffer_size=8192, timeout=0)
pixel = neopixel.NeoPixel(board.NEOPIXEL, 1, brightness=0.2, auto_write=True)

print("Alien Motion Tracker - Multi-Target Optimized")

# Protocol
SYNC = b"\xAA\xFF\x03\x00"
FOOTER = b"\x55\xCC"
FRAME_SIZE = 30

# Display - Full screen
CX, CY = 64, 63
RADII = [20, 40, 60]

# Target tracking with fade-out (Rob's technique)
class Target:
    def __init__(self, x, y, target_id):
        self.x = x
        self.y = y
        self.target_id = target_id
        self.birth_time = time.monotonic()
        self.last_seen = time.monotonic()
        self.active = True  # Recently detected
    
    def age(self):
        return time.monotonic() - self.birth_time
    
    def time_since_seen(self):
        return time.monotonic() - self.last_seen
    
    def update(self, x, y, vel_x=0, vel_y=0, acc_x=0, acc_y=0):
        # We can predict position visually here if we wanted,
        # but the main update is from the compensated coordinate.
        self.x = x
        self.y = y
        self.last_seen = time.monotonic()
        self.active = True

targets = []
filter_states = {} # State dict for multi-anchor stabilization
buffer = bytearray()
bytes_in = 0
frames_ok = 0

# Animation
pulse_start = 0
pulse_active = False


def draw_target_blip(sx, sy, radius, brightness):
    """Draw target blip with Rob's style - filled circle with glow"""
    if brightness < 0.1:
        return
    
    # Inner filled circle
    for dx in range(-radius, radius + 1):
        for dy in range(-radius, radius + 1):
            if dx*dx + dy*dy <= radius*radius:
                px, py = sx + dx, sy + dy
                if 0 <= px < 128 and 0 <= py < 64:
                    oled.pixel(px, py, 1)
    
    # Glow ring (partial)
    glow_radius = radius + 3
    steps = int(glow_radius * 6.28)
    for i in range(0, steps, 2):
        a = (i / steps) * 2 * math.pi
        x = int(sx + glow_radius * math.cos(a))
        y = int(sy + glow_radius * math.sin(a))
        if 0 <= x < 128 and 0 <= y < 64:
            oled.pixel(x, y, 1)

def draw_sweep_lines():
    """Draw sweep lines to all active targets"""
    for target in targets:
        if not target.active or target.time_since_seen() > 0.5:
            continue
        
        # Calculate angle
        angle = math.atan2(-target.y, -target.x)
        angle_deg = math.degrees(angle) + 90
        
        if 0 <= angle_deg <= 180:
            # Draw thin line from center to target
            for r in range(0, RADII[-1] + 1, 3):
                a = math.radians(angle_deg - 90)
                x = int(CX + r * math.cos(a))
                y = int(CY + r * math.sin(a))
                if 0 <= x < 128 and 0 <= y < 64:
                    oled.pixel(x, y, 1)

def draw_display():
    """Draw complete radar with Rob's fade/pulse technique"""
    global pulse_active, pulse_start
    
    oled.fill(0)
    
    now = time.monotonic()
    
    # Calculate pulse animation (0.4 second cycle like Rob's)
    pulse_time = 0
    if pulse_active:
        pulse_time = now - pulse_start
        if pulse_time > 0.4:
            pulse_active = False
    
    # Draw grid (always visible)
    for r in RADII:
        draw_dotted_circle(oled, CX, CY, r)
    oled.line(0, CY, 127, CY, 1)
    
    # Draw sweep lines to active targets
    draw_sweep_lines()
    
    # Draw targets with Rob's three-tier system
    active_count = 0
    
    for target in targets:
        age = target.age()
        time_since = target.time_since_seen()
        
        # Calculate brightness based on age and detection
        if time_since < 1.0:
            # ACTIVE - recently seen, full brightness with pulse
            brightness = 1.0
            active_count += 1
            
            # Pulse effect (radius varies with pulse_time)
            if pulse_active and pulse_time < 0.4:
                pulse_scale = math.sin((pulse_time / 0.4) * math.pi)
                radius = 4 + int(pulse_scale * 3)
            else:
                radius = 4
            
        elif time_since < 3.0:
            # FADING - not recently seen, fade out over 2 seconds
            fade_time = time_since - 1.0
            brightness = 1.0 - (fade_time / 2.0)
            radius = 3
        else:
            # TOO OLD - remove
            continue
        
        # Map to screen
        sx, sy = map_xy(target.x, target.y)
        
        # Draw the blip
        draw_target_blip(sx, sy, radius, brightness)
    
    # NeoPixel status
    if active_count > 0:
        pixel[0] = (50, 0, 0)  # Red when active targets
    else:
        # Rainbow fade when clear
        hue = (now * 50) % 126
        r = int((1 + math.sin(hue * 0.05)) * 25)
        g = int((1 + math.sin(hue * 0.05 + 2.1)) * 25)
        b = int((1 + math.sin(hue * 0.05 + 4.2)) * 25)
        pixel[0] = (r, g, b)
    
    oled.show()

def find_or_create_target(x, y, target_id):
    """Find existing target or create new one (with deduplication)"""
    # Check if this is close to an existing target
    for target in targets:
        dist_sq = (target.x - x)**2 + (target.y - y)**2
        if dist_sq < 90000:
            return target
    
    # Create new target
    new_target = Target(x, y, target_id)
    targets.append(new_target)
    return new_target

def cleanup_old_targets():
    """Remove targets not seen in 3+ seconds (Rob's FADE_DURATION)"""
    global targets
    targets = [t for t in targets if t.time_since_seen() < 3.0]

last_draw = time.monotonic()
last_cleanup = time.monotonic()
last_stat = time.monotonic()

while True:
    # Read UART
    if uart.in_waiting:
        data = uart.read(uart.in_waiting)
        if data:
            bytes_in += len(data)
            buffer.extend(data)
    
    # Parse Basic protocol frames
    while len(buffer) >= FRAME_SIZE:
        # Find sync pattern
        sync_idx = buffer.find(SYNC)
        
        if sync_idx < 0:
            if len(buffer) > 100:
                buffer = buffer[-50:]
            break
        
        if sync_idx > 0:
            buffer = buffer[sync_idx:]
        
        if len(buffer) < FRAME_SIZE:
            break
        
        # Verify footer
        if buffer[28] == 0x55 and buffer[29] == 0xCC:
            frames_ok += 1
            frame = buffer[0:FRAME_SIZE]
            buffer = buffer[FRAME_SIZE:]
            
            # Parse 3 targets
            any_new = False
            raw_targets = []

            for t_idx in range(3):
                offset = 4 + (t_idx * 8)
                
                x = ld2450_s16(frame[offset], frame[offset + 1])
                y = ld2450_s16(frame[offset + 2], frame[offset + 3])
                s = ld2450_s16(frame[offset + 4], frame[offset + 5])
                
                # Target exists if not all zeros
                dist_sq = x * x + y * y
                if (x != 0 or y != 0) and (10000 < dist_sq < 64000000):
                    raw_targets.append({'id': t_idx, 'active': True, 'x': x, 'y': y, 'speed': s})
                else:
                    raw_targets.append({'id': t_idx, 'active': False, 'x': 0, 'y': 0, 'speed': 0})

            # Apply Multi-Anchor Stabilization
            from utils import calculate_multi_anchor_stabilization
            stabilized = calculate_multi_anchor_stabilization(raw_targets, filter_states, dt=0.1)
            
            for t in stabilized:
                if t['active']:
                    target = find_or_create_target(t['x'], t['y'], t['id'])
                    target.update(t['x'], t['y'], t['vel_x'], t['vel_y'], t['acc_x'], t['acc_y'])
                    any_new = True

            # Trigger pulse animation when new targets detected
            if any_new:
                pulse_start = time.monotonic()
                pulse_active = True
            
            # Mark all targets as not active by default
            for target in targets:
                target.active = False
        else:
            buffer = buffer[2:]
    
    # Update display at 20 FPS
    now = time.monotonic()
    if now - last_draw > 0.05:
        draw_display()
        last_draw = now
    
    # Cleanup old targets every second
    if now - last_cleanup > 1.0:
        cleanup_old_targets()
        last_cleanup = now
    
    # Status report
    if now - last_stat > 1.0:
        active = sum(1 for t in targets if t.time_since_seen() < 1.0)
        total = len(targets)
        print("RX {:5d} B/s | frames {:3d}/s | targets {:d}/{:d}".format(
            bytes_in, frames_ok, active, total))
        bytes_in = 0
        frames_ok = 0
        last_stat = now
    
    time.sleep(0.001)
