## 2024-07-24 - Precalculate Expensive Math on Sensor Data Ingestion
**Learning:** In systems with a fast render loop (e.g., 60-120Hz) but slow sensor data updates (e.g., 10Hz), repeatedly calculating things like distance (`sqrtf`) and angle (`atan2f`) on the raw target coordinates within the render loop wastes significant CPU cycles.
**Action:** When working on rendering or UI loops, always check the frequency of the underlying data source. Move expensive trigonometric or square root calculations into the data reception function (e.g., `updateRadarData`), store the results in class member variables, and reuse them during rendering to free up the CPU.

## 2024-07-24 - Avoid Runtime Trigonometry for Static UI Elements
**Learning:** In fast rendering loops on embedded systems like the ESP32, repeated evaluations of trigonometric functions (`cosf`, `sinf`) for static UI angles (like fixed grid spokes or ticks) waste valuable FPU cycles per frame.
**Action:** Replace dynamically calculated loop angles and their associated runtime `cosf`/`sinf` evaluations with a `constexpr` struct array of precalculated 2D direction vectors (cosine and sine values) to eliminate unnecessary mathematical overhead during rendering.
## 2024-07-31 - Replace Fixed-Step Trigonometry with Vector Rotation Matrices
**Learning:** Fixed-step iterative trigonometric operations (like calling `cosf` and `sinf` in a loop over varying angles) can be extremely costly on FPU-constrained devices like the ESP32, particularly in hot rendering loops like radial backgrounds or boot screens.
**Action:** Replace iterative angular trigonometry with 2D vector rotation matrices. Initialize a starting vector and iteratively apply a pre-calculated constant rotation matrix (using the step size's sine and cosine values) to eliminate `sinf`/`cosf` evaluations entirely from the loop.
