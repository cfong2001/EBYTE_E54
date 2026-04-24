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
from shared.ui_utils import map_xy

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

def main():
    parser = argparse.ArgumentParser(description="Test HLK-LD2450 utility modules")
    parser.add_argument("--s16", action="store_true", help="Run only s16_le tests")
    parser.add_argument("--ld2450", action="store_true", help="Run only ld2450_s16 tests")
    parser.add_argument("--map", action="store_true", help="Run only map_xy tests")
    parser.add_argument("--all", action="store_true", help="Run all tests")

    args = parser.parse_args()

    # Default to all if nothing selected
    if not any([args.s16, args.ld2450, args.map, args.all]):
        args.all = True

    try:
        if args.all or args.s16:
            test_s16_le()

        if args.all or args.ld2450:
            test_ld2450_s16()

        if args.all or args.map:
            test_map_xy()

        logger.info("\nAll selected tests executed successfully.")

    except AssertionError as e:
        logger.error(f"TEST FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        logger.error(f"UNEXPECTED ERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
