## 2024-07-24 - Precalculate Expensive Math on Sensor Data Ingestion
**Learning:** In systems with a fast render loop (e.g., 60-120Hz) but slow sensor data updates (e.g., 10Hz), repeatedly calculating things like distance (`sqrtf`) and angle (`atan2f`) on the raw target coordinates within the render loop wastes significant CPU cycles.
**Action:** When working on rendering or UI loops, always check the frequency of the underlying data source. Move expensive trigonometric or square root calculations into the data reception function (e.g., `updateRadarData`), store the results in class member variables, and reuse them during rendering to free up the CPU.

## 2024-07-24 - Avoid Runtime Trigonometry for Static UI Elements
**Learning:** In fast rendering loops on embedded systems like the ESP32, repeated evaluations of trigonometric functions (`cosf`, `sinf`) for static UI angles (like fixed grid spokes or ticks) waste valuable FPU cycles per frame.
**Action:** Replace dynamically calculated loop angles and their associated runtime `cosf`/`sinf` evaluations with a `constexpr` struct array of precalculated 2D direction vectors (cosine and sine values) to eliminate unnecessary mathematical overhead during rendering.

## 2024-08-03 - Avoid Dynamic String Allocations in UI Rendering and Click Handlers
**Learning:** In Arduino/C++ embedded environments, using the `String` class (e.g., `String selItem = String(...)`) and its methods like `startsWith` inside high-frequency UI handlers (like menu rendering or click handlers) causes unnecessary heap allocation overhead and fragmentation, leading to performance issues and potential crashes over time. Furthermore, routing dynamic menu clicks using brittle numerical index math (e.g., `maxMenuSelection - 5`) is prone to silent breakage when menu items are added or removed.
**Action:** When implementing or refactoring UI components and event handlers, always avoid dynamic `String` allocations. Instead, retrieve the underlying `const char*` and use C-string functions like `strncmp` with compile-time known lengths for robust, zero-allocation prefix matching and handler routing.
