## 2024-04-23 - Avoid Expensive `math.sqrt` in Bounds and Distance Checks
**Learning:** Embedded targets running CircuitPython incur significant overhead per iteration for floating-point calculations like `math.sqrt`. When absolute distance logic is only used for relative bounding checks or duplication comparisons against a threshold (e.g., `dist < 300` or `100 < dist < 8000`), computing the square root is mathematically unnecessary.
**Action:** Replace `math.sqrt(x*x + y*y) < C` with `(x*x + y*y) < C*C`. This transforms a costly FP operation into a faster integer arithmetic comparison.
