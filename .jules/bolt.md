## 2024-07-24 - Precalculate Expensive Math on Sensor Data Ingestion
**Learning:** In systems with a fast render loop (e.g., 60-120Hz) but slow sensor data updates (e.g., 10Hz), repeatedly calculating things like distance (`sqrtf`) and angle (`atan2f`) on the raw target coordinates within the render loop wastes significant CPU cycles.
**Action:** When working on rendering or UI loops, always check the frequency of the underlying data source. Move expensive trigonometric or square root calculations into the data reception function (e.g., `updateRadarData`), store the results in class member variables, and reuse them during rendering to free up the CPU.

## 2024-07-24 - Avoid Runtime Trigonometry for Static UI Elements
**Learning:** In fast rendering loops on embedded systems like the ESP32, repeated evaluations of trigonometric functions (`cosf`, `sinf`) for static UI angles (like fixed grid spokes or ticks) waste valuable FPU cycles per frame.
**Action:** Replace dynamically calculated loop angles and their associated runtime `cosf`/`sinf` evaluations with a `constexpr` struct array of precalculated 2D direction vectors (cosine and sine values) to eliminate unnecessary mathematical overhead during rendering.
## 2023-10-27 - Vector Rotation for Radial Rendering
**Learning:** In ESP32 graphics loops (like drawing radar grids or sweeping arcs), calling `cosf()` and `sinf()` inside a loop that steps by a fixed angle per iteration consumes significant CPU cycles.
**Action:** Replace trigonometric function calls in iterative radial loops with a fixed 2D vector rotation using a 2x2 rotation matrix, applying precalculated `constexpr` sine/cosine constants for the step angle to continuously advance the direction vector.
