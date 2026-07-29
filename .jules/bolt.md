## 2024-07-24 - Precalculate Expensive Math on Sensor Data Ingestion
**Learning:** In systems with a fast render loop (e.g., 60-120Hz) but slow sensor data updates (e.g., 10Hz), repeatedly calculating things like distance (`sqrtf`) and angle (`atan2f`) on the raw target coordinates within the render loop wastes significant CPU cycles.
**Action:** When working on rendering or UI loops, always check the frequency of the underlying data source. Move expensive trigonometric or square root calculations into the data reception function (e.g., `updateRadarData`), store the results in class member variables, and reuse them during rendering to free up the CPU.

## 2024-07-24 - Avoid Runtime Trigonometry for Static UI Elements
**Learning:** In fast rendering loops on embedded systems like the ESP32, repeated evaluations of trigonometric functions (`cosf`, `sinf`) for static UI angles (like fixed grid spokes or ticks) waste valuable FPU cycles per frame.
**Action:** Replace dynamically calculated loop angles and their associated runtime `cosf`/`sinf` evaluations with a `constexpr` struct array of precalculated 2D direction vectors (cosine and sine values) to eliminate unnecessary mathematical overhead during rendering.

## 2024-07-29 - Avoid Arduino String allocations in UI menus
**Learning:** Instantiating Arduino `String` objects in high-frequency loops or handlers (e.g., UI menu handling) causes heap allocation overhead and fragmentation.
**Action:** When replacing Arduino `String::startsWith("literal")` with C-string equivalents, use the pattern `strncmp(str, "literal", strlen("literal")) == 0`. Modern compilers optimize `strlen()` on string literals at compile-time, providing safe bounds-checking with zero runtime allocation or length calculation overhead.
