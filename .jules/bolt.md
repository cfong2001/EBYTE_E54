## 2024-05-19 - Defer math.sqrt calls in display loops
**Learning:** Found a performance bottleneck where `math.sqrt()` was being called on every target inside rendering loops solely to find the minimum distance for text display. On constrained embedded hardware, evaluating `math.sqrt` inside tight loops causes measurable overhead.
**Action:** When finding the closest target distance, track `closest_dist_sq` by calculating squared distances (`x*x + y*y`) inside the loop, and apply `math.sqrt()` to the final `closest_dist_sq` outside the loop before rendering.

## 2026-04-25 - [Single Precision Float Math Optimization]
**Learning:** Using standard double precision math functions (`sqrt`, `sin`, `cos`, `atan2`, `abs`) implicitly casts variables back and forth to double when dealing with `float` types. For the ESP32 platform, utilizing single precision variants (`sqrtf`, `sinf`, `cosf`, `atan2f`, `fabsf`) uses the hardware FPU natively and prevents the type casting overhead.
**Action:** Default to using single precision functions (e.g. `sinf`, `sqrtf`, `fabsf`) whenever the data variables are strictly typed as `float`. Avoid implicit type promotion in constrained performance-critical paths.
