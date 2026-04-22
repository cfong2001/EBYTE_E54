import sys
from unittest.mock import MagicMock

# Mock hardware modules to allow import of code_correct_protocol
sys.modules['board'] = MagicMock()
sys.modules['busio'] = MagicMock()
sys.modules['neopixel'] = MagicMock()
sys.modules['adafruit_ssd1306'] = MagicMock()

import code_correct_protocol

def test_s16_le():
    """Test Convert little-endian signed int16 per LD2450 protocol.
    Protocol spec: highest bit 1=positive, 0=negative (inverted from standard)
    """
    # Positive values: highest bit (0x8000) is 1
    # Result should be (v - 0x8000)
    assert code_correct_protocol._s16_le(0x05, 0x80) == 5       # 0x8005 -> 5
    assert code_correct_protocol._s16_le(0x00, 0x80) == 0       # 0x8000 -> 0
    assert code_correct_protocol._s16_le(0xFF, 0xFF) == 32767   # 0xFFFF -> 32767 (0x7FFF)
    assert code_correct_protocol._s16_le(0x01, 0x80) == 1       # 0x8001 -> 1

    # Negative values: highest bit (0x8000) is 0
    # Result should be -v
    assert code_correct_protocol._s16_le(0x05, 0x00) == -5      # 0x0005 -> -5
    assert code_correct_protocol._s16_le(0x00, 0x00) == 0       # 0x0000 -> 0
    assert code_correct_protocol._s16_le(0xFF, 0x7F) == -32767  # 0x7FFF -> -32767
    assert code_correct_protocol._s16_le(0x01, 0x00) == -1      # 0x0001 -> -1

def test_map_xy():
    """Test mapping of radar coordinates to screen coordinates.
    sx = int(max(0, min(127, (x_mm + 3000) * 127 / 6000)))
    sy = int(max(0, min(63, 63 - (y_mm * 63 / 6000))))
    """
    # Center (0, 0) -> (63, 63)
    # sx = int(3000 * 127 / 6000) = int(63.5) = 63
    # sy = int(63 - 0) = 63
    assert code_correct_protocol.map_xy(0, 0) == (63, 63)

    # Top Left (-3000, 6000) -> (0, 0)
    # sx = int(0 * 127 / 6000) = 0
    # sy = int(63 - (6000 * 63 / 6000)) = 63 - 63 = 0
    assert code_correct_protocol.map_xy(-3000, 6000) == (0, 0)

    # Bottom Right (3000, 0) -> (127, 63)
    # sx = int(6000 * 127 / 6000) = 127
    # sy = int(63 - 0) = 63
    assert code_correct_protocol.map_xy(3000, 0) == (127, 63)

    # Extreme positive values (clamping)
    assert code_correct_protocol.map_xy(5000, 8000) == (127, 0)

    # Extreme negative values (clamping)
    assert code_correct_protocol.map_xy(-5000, -1000) == (0, 63)
