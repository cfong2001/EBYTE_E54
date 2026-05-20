# ==============================================================================
# NOTE: This file is for testing, diagnostics, or deployment purposes only.
# It is NOT essential to the core functionality or compilation of the main C++
# application located in the /src directory.
# AI Agents and developers should NOT attempt to optimize, refactor, or
# modify this file unless explicitly requested to do so by the user.
# ==============================================================================
import math
import time

def generate_mock_data(num_points=10000):
    import random
    random.seed(42)
    return [(random.randint(-8000, 8000), random.randint(-8000, 8000)) for _ in range(num_points)]

data = generate_mock_data(100000)

def test_sqrt():
    count = 0
    start = time.monotonic()
    for x, y in data:
        dist = math.sqrt(x*x + y*y)
        if 100 < dist < 8000:
            count += 1
    end = time.monotonic()
    return end - start, count

def test_squared():
    count = 0
    start = time.monotonic()
    for x, y in data:
        dist_sq = x*x + y*y
        if 10000 < dist_sq < 64000000:
            count += 1
    end = time.monotonic()
    return end - start, count

t1, c1 = test_sqrt()
t2, c2 = test_squared()

print(f"sqrt: {t1:.5f}s, count: {c1}")
print(f"squared: {t2:.5f}s, count: {c2}")
print(f"Improvement: {t1/t2:.2f}x faster")
