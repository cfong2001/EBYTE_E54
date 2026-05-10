import math

# Pre-computed trigonometric lookup tables for UI rendering
COS_TABLE = tuple(math.cos(math.radians(i)) for i in range(360))
SIN_TABLE = tuple(math.sin(math.radians(i)) for i in range(360))


def draw_dotted_circle(display, cx, cy, r, start_angle_deg=0, end_angle_deg=360, step_deg=None, offset_deg=0, brightness=1.0):
    """
    Draw a dotted circle or arc on a display.

    Args:
        display: The display object (e.g., adafruit_ssd1306, TFT) with width, height, and pixel() method.
        cx: Center X coordinate.
        cy: Center Y coordinate.
        r: Radius.
        start_angle_deg: Starting angle in degrees (default 0).
        end_angle_deg: Ending angle in degrees (default 360).
        step_deg: Step between dots in degrees. If None, calculated based on radius for ~3px spacing.
        offset_deg: Angle offset in degrees (default 0).
        brightness: Brightness/color value (default 1.0). If < 0.1, nothing is drawn.
    """
    if brightness < 0.1:
        return

    # Default step logic based on radius if not provided
    if step_deg is None:
        # Approximate step to get dots about 3 pixels apart on the circumference
        circumference = 2 * math.pi * r
        if circumference > 0:
            # 3 pixels arc length = 3 / circumference * 360 degrees
            step_deg = max(1, int((3 / circumference) * 360))
        else:
            step_deg = 4

    for angle_deg in range(start_angle_deg, end_angle_deg + 1, step_deg):
        idx = int(round(angle_deg + offset_deg)) % 360
        x = int(cx + r * COS_TABLE[idx])
        # Note: In standard CircuitPython display coordinate systems, y increases downwards.
        # However, some math implementations use standard Cartesian where y increases upwards,
        # hence they use cy - r * sin(a) or cy + r * sin(a) depending on if they added negative offsets.
        # We use cy + r * math.sin(rad) which matches standard Cartesian angle rotation if y goes down.
        # If a specific script needs inverted y, they can pass negative offset or flip signs,
        # but the existing scripts predominantly use `cy + r * math.sin(a)`.
        y = int(cy + r * SIN_TABLE[idx])

        if 0 <= x < display.width and 0 <= y < display.height:
            # Most simple displays use 1 for white/on, TFTs might use color values.
            # We assume brightness is used as color for TFT or just 1 for monochrome.
            color = int(brightness) if isinstance(brightness, (int, float)) and brightness > 1 else 1
            display.pixel(x, y, color)

def map_xy(x_mm, y_mm, display_width=128, display_height=64, max_range_mm=6000):
    """
    Map radar coordinates to screen coordinates.
    Assumes X is horizontal (centered) and Y is vertical (distance).
    """
    sx = int(max(0, min(display_width - 1, (x_mm + max_range_mm/2) * (display_width - 1) / max_range_mm)))
    sy = int(max(0, min(display_height - 1, (display_height - 1) - (y_mm * (display_height - 1) / max_range_mm))))
    return sx, sy
