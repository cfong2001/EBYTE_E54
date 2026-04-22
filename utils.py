"""
Shared utility functions for HLK-LD2450 radar data processing.
"""

def s16_le(b0, b1):
    """
    Convert two bytes to a signed 16-bit integer using standard 2's complement.
    Used in some versions of the radar protocol.
    """
    v = b0 | (b1 << 8)
    return v - 0x10000 if v & 0x8000 else v

def ld2450_s16(b0, b1):
    """
    Convert little-endian signed int16 per LD2450 protocol spec.
    Protocol spec: highest bit 1=positive, 0=negative (inverted from standard).
    """
    v = b0 | (b1 << 8)
    if v & 0x8000:
        return v - 0x8000  # Positive: remove sign bit
    else:
        return -v if v != 0 else 0  # Negative: negate the value
