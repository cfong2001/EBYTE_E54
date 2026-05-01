#!/usr/bin/env python3
"""
Unified Testing Script for HLK-LD2450 Radar Data Processing Utilities.
Tests data parsing mapping and decoding logic using manual-provided sample data.
"""

import argparse
import sys
import logging

# Ensure our local path is at front for testing modules without collision
sys.path.insert(0, ".")
from utils import s16_le, ld2450_s16
from shared.ui_utils import map_xy, draw_dotted_circle

# Configure standard logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger('RadarTests')

def test_s16_le():
    """Test standard 2's complement 16-bit little-endian conversion"""
    logger.info("Running s16_le tests...")

    # 0 = 0x00 0x00
    assert s16_le(0x00, 0x00) == 0, "s16_le failed for 0"

    # 100 = 0x64 0x00
    assert s16_le(0x64, 0x00) == 100, "s16_le failed for 100"

    # -100 = 0x9C 0xFF (two's complement)
    assert s16_le(0x9C, 0xFF) == -100, "s16_le failed for -100"

    # Max positive 32767 = 0xFF 0x7F
    assert s16_le(0xFF, 0x7F) == 32767, "s16_le failed for max positive"

    # Max negative -32768 = 0x00 0x80
    assert s16_le(0x00, 0x80) == -32768, "s16_le failed for max negative"

    logger.info("  ✓ s16_le tests passed!")

def test_ld2450_s16():
    """Test custom LD2450 signed integer conversion (inverted highest bit)"""
    logger.info("Running ld2450_s16 tests...")

    # Manual Protocol spec: highest bit 1=positive, 0=negative.

    # 0 = 0x00 0x00
    assert ld2450_s16(0x00, 0x00) == 0, "ld2450_s16 failed for 0"

    # Positive 100: 0x64 0x80 (highest bit 1)
    assert ld2450_s16(0x64, 0x80) == 100, "ld2450_s16 failed for +100"

    # Negative 100: 0x64 0x00 (highest bit 0)
    assert ld2450_s16(0x64, 0x00) == -100, "ld2450_s16 failed for -100"

    logger.info("  ✓ ld2450_s16 tests passed!")

def test_map_xy():
    """Test standard mapping of millimeter coordinate space to pixel display space"""
    logger.info("Running map_xy tests...")

    # Standard mapping for a 128x64 display
    # Radar space X is -3000 to +3000 mapped to 0-127
    # Radar space Y is 0 to 6000 mapped to 63-0 (Y starts at bottom)

    # Center bottom (Target at 0mm X, 0mm Y)
    sx, sy = map_xy(0, 0)
    assert sx == 63, f"map_xy failed for X=0. Expected 63, got {sx}"
    assert sy == 63, f"map_xy failed for Y=0. Expected 63, got {sy}"

    # Far left bottom (Target at -3000mm X, 0mm Y)
    sx, sy = map_xy(-3000, 0)
    assert sx == 0, f"map_xy failed for X=-3000. Expected 0, got {sx}"
    assert sy == 63, f"map_xy failed for Y=0. Expected 63, got {sy}"

    # Far right top (Target at 3000mm X, 6000mm Y)
    sx, sy = map_xy(3000, 6000)
    assert sx == 127, f"map_xy failed for X=3000. Expected 127, got {sx}"
    assert sy == 0, f"map_xy failed for Y=6000. Expected 0, got {sy}"

    # Out of bounds clamping
    sx, sy = map_xy(4000, 8000)
    assert sx == 127, f"map_xy bounds clamping failed for X=4000. Expected 127, got {sx}"
    assert sy == 0, f"map_xy bounds clamping failed for Y=8000. Expected 0, got {sy}"

    logger.info("  ✓ map_xy tests passed!")

def test_multi_anchor_stabilization():
    """Test 3D multi-anchor stabilization math with dynamic number of anchors"""
    from utils import calculate_multi_anchor_stabilization
    import math

    logger.info("Running multi_anchor_stabilization tests...")

    state_dict = {}

    # Initial frame: 3 static targets (Anchors)
    frame1 = [
        {'id': 0, 'active': True, 'x': -1000, 'y': 2000, 'speed': 0},
        {'id': 1, 'active': True, 'x': 1000, 'y': 2000, 'speed': 0},
        {'id': 2, 'active': True, 'x': 0, 'y': 3000, 'speed': 0}
    ]

    stab1 = calculate_multi_anchor_stabilization(frame1, state_dict, dt=0.1)

    # Verify initial initialization passes cleanly
    for s in stab1:
        assert s['active'], "Target should be active"

    # Frame 2: Sensor translates +500mm X, +500mm Y (so targets appear to move -500, -500)
    frame2 = [
        {'id': 0, 'active': True, 'x': -1500, 'y': 1500, 'speed': 0},
        {'id': 1, 'active': True, 'x': 500, 'y': 1500, 'speed': 0},
        {'id': 2, 'active': True, 'x': -500, 'y': 2500, 'speed': 0}
    ]

    stab2 = calculate_multi_anchor_stabilization(frame2, state_dict, dt=0.1)

    # Because they are anchors moving together, the algorithm should reject the uniform translation
    # and pull them back toward their original positions (filtering lag applies, but direction should be correct)
    for i in range(3):
        # We expect the stabilized X to be closer to frame1 X than frame2 X
        dist_f1 = abs(stab2[i]['x'] - frame1[i]['x'])
        dist_f2 = abs(stab2[i]['x'] - frame2[i]['x'])
        assert dist_f1 < dist_f2, f"Target {i} translation not rejected! stab: {stab2[i]['x']} orig: {frame1[i]['x']} raw: {frame2[i]['x']}"

    # Frame 3: One target drops out (simulated loss). Handover test.
    frame3 = [
        {'id': 0, 'active': True, 'x': -1500, 'y': 1500, 'speed': 0},
        {'id': 1, 'active': True, 'x': 500, 'y': 1500, 'speed': 0},
        {'id': 2, 'active': False, 'x': 0, 'y': 0, 'speed': 0}
    ]

    stab3 = calculate_multi_anchor_stabilization(frame3, state_dict, dt=0.1)
    assert not stab3[2]['active'], "Dropped target should be inactive"

    # The remaining two anchors should still be able to reject rotation and translation

    # Frame 4: Sensor Rotates (Yaw twist). Targets appear to rotate around the centroid
    # Let's artificially rotate the raw points by ~90 degrees counter-clockwise around the origin
    # to test extreme mathematical handling (though unrealistic for 1 frame)
    frame4 = [
        {'id': 0, 'active': True, 'x': -1500, 'y': -1500, 'speed': 0},
        {'id': 1, 'active': True, 'x': -1500, 'y': 500, 'speed': 0},
        {'id': 2, 'active': False, 'x': 0, 'y': 0, 'speed': 0}
    ]

    stab4 = calculate_multi_anchor_stabilization(frame4, state_dict, dt=0.1)

    # Ensure no math errors (like divide by zero or sqrt of negative) occurred and output is valid float
    assert not math.isnan(stab4[0]['x']), "NaN in stabilized math"
    assert not math.isnan(stab4[0]['y']), "NaN in stabilized math"

    # Frame 5: Zero anchors (all targets moving fast). Should fallback gracefully
    frame5 = [
        {'id': 0, 'active': True, 'x': -1500, 'y': -1500, 'speed': 200},
        {'id': 1, 'active': True, 'x': -1500, 'y': 500, 'speed': 200},
        {'id': 2, 'active': False, 'x': 0, 'y': 0, 'speed': 0}
    ]
    stab5 = calculate_multi_anchor_stabilization(frame5, state_dict, dt=0.1)

    assert stab5[0]['x'] == frame5[0]['x'], "Zero anchors should pass raw coordinate through to filter"

    # Frame 6: Dynamic Anchor Validation
    # Let's say we have 3 targets that are previously stable
    state_dict = {}
    frame6_1 = [
        {'id': 0, 'active': True, 'x': 0, 'y': 1000, 'speed': 0},
        {'id': 1, 'active': True, 'x': 1000, 'y': 1000, 'speed': 0},
        {'id': 2, 'active': True, 'x': -1000, 'y': 1000, 'speed': 0}
    ]
    calculate_multi_anchor_stabilization(frame6_1, state_dict, dt=0.1)

    # Give the filter a moment to settle state so they are considered stable anchors
    calculate_multi_anchor_stabilization(frame6_1, state_dict, dt=0.1)
    calculate_multi_anchor_stabilization(frame6_1, state_dict, dt=0.1)

    # Next frame: targets 0 and 1 stay perfectly rigid together (they moved +100x),
    # but target 2 moves -500x (breaking rigidity heavily). It should be rejected as an anchor.
    frame6_2 = [
        {'id': 0, 'active': True, 'x': 100, 'y': 1000, 'speed': 0},
        {'id': 1, 'active': True, 'x': 1100, 'y': 1000, 'speed': 0},
        {'id': 2, 'active': True, 'x': -1500, 'y': 1000, 'speed': 0}
    ]
    calculate_multi_anchor_stabilization(frame6_2, state_dict, dt=0.1)

    # In a 3-point geometry where one moves, the two that maintain distance relative to each other
    # will have lower combined error than the one that moved relative to both.
    # Note: because it's a simulated jump without velocity ramp-up, the Alpha-Beta filter might temporarily
    # flag all as anomalous due to high instantaneous residual, but the important part is 2 is definitely rejected.
    assert state_dict[2]['is_anchor'] == False, f"Anomalous moving target failed to be rejected from anchors. state: {state_dict[2]}"

    # Frame 7: Acceleration test
    # Target 0 starts accelerating positively in X
    frame7_1 = [{'id': 0, 'active': True, 'x': 0, 'y': 0, 'speed': 0}]
    frame7_2 = [{'id': 0, 'active': True, 'x': 10, 'y': 0, 'speed': 100}]
    frame7_3 = [{'id': 0, 'active': True, 'x': 30, 'y': 0, 'speed': 200}]

    sd2 = {}
    calculate_multi_anchor_stabilization(frame7_1, sd2, dt=0.1)
    calculate_multi_anchor_stabilization(frame7_2, sd2, dt=0.1)
    stab_7 = calculate_multi_anchor_stabilization(frame7_3, sd2, dt=0.1)

    assert sd2[0]['acc_x'] > 0, "Acceleration not properly calculated/tracked"

    logger.info("  ✓ multi_anchor_stabilization tests passed!")


class MockDisplay:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = []

    def pixel(self, x, y, color):
        self.pixels.append((x, y, color))

def test_draw_dotted_circle():
    """Test drawing dotted circles and arcs with various parameters."""
    logger.info("Running draw_dotted_circle tests...")

    # Test 1: Basic full circle
    display = MockDisplay(128, 64)
    # cx=64, cy=32, r=10, step_deg=90 -> angles: 0, 90, 180, 270, 360
    draw_dotted_circle(display, 64, 32, 10, 0, 360, 90)

    assert len(display.pixels) == 5, f"Expected 5 pixels, got {len(display.pixels)}"
    assert (74, 32, 1) in display.pixels, "Missing 0 degree pixel"
    assert (64, 42, 1) in display.pixels, "Missing 90 degree pixel"

    # Test 2: Low brightness (should not draw)
    display2 = MockDisplay(128, 64)
    draw_dotted_circle(display2, 64, 32, 10, brightness=0.05)
    assert len(display2.pixels) == 0, "Should not draw anything if brightness < 0.1"

    # Test 3: Out of bounds
    display3 = MockDisplay(10, 10)
    draw_dotted_circle(display3, 100, 100, 10, 0, 360, 90)
    assert len(display3.pixels) == 0, "Should not draw out of bounds pixels"

    # Test 4: Default step calculation
    display4 = MockDisplay(128, 64)
    draw_dotted_circle(display4, 64, 32, 5, 0, 360) # small radius
    assert len(display4.pixels) > 0, "Should auto-calculate step and draw pixels"

    logger.info("  ✓ draw_dotted_circle tests passed!")

def main():
    parser = argparse.ArgumentParser(description="Test HLK-LD2450 utility modules")
    parser.add_argument("--s16", action="store_true", help="Run only s16_le tests")
    parser.add_argument("--ld2450", action="store_true", help="Run only ld2450_s16 tests")
    parser.add_argument("--map", action="store_true", help="Run only map_xy tests")
    parser.add_argument("--multi", action="store_true", help="Run only multi_anchor tests")
    parser.add_argument("--draw", action="store_true", help="Run only draw_dotted_circle tests")
    parser.add_argument("--all", action="store_true", help="Run all tests")

    args = parser.parse_args()

    # Default to all if nothing selected
    if not any([args.s16, args.ld2450, args.map, args.multi, args.draw, args.all]):
        args.all = True

    try:
        if args.all or args.s16:
            test_s16_le()

        if args.all or args.ld2450:
            test_ld2450_s16()

        if args.all or args.map:
            test_map_xy()

        if args.all or args.multi:
            test_multi_anchor_stabilization()

        if args.all or args.draw:
            test_draw_dotted_circle()

        logger.info("\nAll selected tests executed successfully.")

    except AssertionError as e:
        logger.error(f"TEST FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        logger.error(f"UNEXPECTED ERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
