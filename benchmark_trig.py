import math
import time

COS_TABLE = tuple(math.cos(math.radians(i)) for i in range(360))
SIN_TABLE = tuple(math.sin(math.radians(i)) for i in range(360))

def test_math_trig():
    start = time.monotonic()
    for angle_deg in range(0, 360000):
        rad = math.radians(angle_deg)
        x = math.cos(rad)
        y = math.sin(rad)
    end = time.monotonic()
    return end - start

def test_table_trig():
    start = time.monotonic()
    for angle_deg in range(0, 360000):
        idx = angle_deg % 360
        x = COS_TABLE[idx]
        y = SIN_TABLE[idx]
    end = time.monotonic()
    return end - start

t1 = test_math_trig()
t2 = test_table_trig()

print(f"math trig: {t1:.5f}s")
print(f"table lookup: {t2:.5f}s")
print(f"Improvement: {t1/t2:.2f}x faster")
