## 2024-07-24 - Precalculate Expensive Math on Sensor Data Ingestion
**Learning:** In systems with a fast render loop (e.g., 60-120Hz) but slow sensor data updates (e.g., 10Hz), repeatedly calculating things like distance (`sqrtf`) and angle (`atan2f`) on the raw target coordinates within the render loop wastes significant CPU cycles.
**Action:** When working on rendering or UI loops, always check the frequency of the underlying data source. Move expensive trigonometric or square root calculations into the data reception function (e.g., `updateRadarData`), store the results in class member variables, and reuse them during rendering to free up the CPU.

## 2024-07-24 - Avoid Runtime Trigonometry for Static UI Elements
**Learning:** In fast rendering loops on embedded systems like the ESP32, repeated evaluations of trigonometric functions (`cosf`, `sinf`) for static UI angles (like fixed grid spokes or ticks) waste valuable FPU cycles per frame.
**Action:** Replace dynamically calculated loop angles and their associated runtime `cosf`/`sinf` evaluations with a `constexpr` struct array of precalculated 2D direction vectors (cosine and sine values) to eliminate unnecessary mathematical overhead during rendering.

## 2024-08-04 - Avoid Dynamic String Allocation in High-Frequency Handlers
**Learning:** In C++ on embedded systems, dynamically allocating `String` objects inside high-frequency functions such as UI click handlers or rendering loops causes unnecessary heap allocation overhead and memory fragmentation.
**Action:** Always prefer `const char*` and standard C-string functions like `strncmp` for string manipulations and comparisons in fast-executing loops or event handlers to eliminate dynamic memory allocation overhead.
