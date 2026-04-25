
## 2026-04-25 - [Single Precision Float Math Optimization]
**Learning:** Using standard double precision math functions (`sqrt`, `sin`, `cos`, `atan2`, `abs`) implicitly casts variables back and forth to double when dealing with `float` types. For the ESP32 platform, utilizing single precision variants (`sqrtf`, `sinf`, `cosf`, `atan2f`, `fabsf`) uses the hardware FPU natively and prevents the type casting overhead.
**Action:** Default to using single precision functions (e.g. `sinf`, `sqrtf`, `fabsf`) whenever the data variables are strictly typed as `float`. Avoid implicit type promotion in constrained performance-critical paths.
