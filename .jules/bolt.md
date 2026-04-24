## 2024-05-24 - CircuitPython Math Sqrt Overhead
**Learning:** In CircuitPython loops (like radar deduplication), `math.sqrt(x*x + y*y)` is nearly 40-50% slower than checking `x*x + y*y < threshold_squared` (0.17us vs 0.10us per call in my profiling). Since radar data can emit multiple frames per second and process many points, this adds up.
**Action:** Always use squared distances (`x*x + y*y`) for distance thresholds/filtering and defer `math.sqrt` to the final formatting step before rendering to UI.
