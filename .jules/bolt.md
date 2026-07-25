## 2024-07-24 - Precalculate Expensive Math on Sensor Data Ingestion
**Learning:** In systems with a fast render loop (e.g., 60-120Hz) but slow sensor data updates (e.g., 10Hz), repeatedly calculating things like distance (`sqrtf`) and angle (`atan2f`) on the raw target coordinates within the render loop wastes significant CPU cycles.
**Action:** When working on rendering or UI loops, always check the frequency of the underlying data source. Move expensive trigonometric or square root calculations into the data reception function (e.g., `updateRadarData`), store the results in class member variables, and reuse them during rendering to free up the CPU.

## 2024-07-25 - Precalculate Trigonometry for Static UI Elements
**Learning:** In systems with a fast render loop, repeatedly evaluating trigonometric functions like `cosf` and `sinf` for static UI elements (like fixed-angle background grids or radar spokes) wastes CPU cycles on ESP32 FPUs.
**Action:** When drawing static, fixed-angle polar UI elements, replace dynamic trigonometric calculations with `constexpr` arrays of precalculated 2D direction vectors `(dx, dy)`.
