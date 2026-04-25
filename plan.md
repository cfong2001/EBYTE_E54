# Deep theoretical QC

1. `init()` checks:
`init` calls `SetMode(EBYTE_MODE_NORMAL)` right away. Because `_mode` is initialized to `0xFF` in the constructor, `_mode == EBYTE_MODE_NORMAL` (0 == 0) will be false (0xFF != 0).
The `SetMode(0)` will execute, applying `delay(50)`, setting `_M0` and `_M1` to low, running `ClearBuffer()`, calling `CompleteTask`, and setting `_mode = 0`.
This is exactly the correct behavior. The hardware requires synchronization upon boot, and because we initialize to `0xFF`, we guarantee the first call always executes.

2. `SaveParameters()` checks:
`SaveParameters()` calls `SetMode(MODE_PROGRAM)`, writes configuration, and calls `SetMode(EBYTE_MODE_NORMAL)`.
When `SetMode(MODE_PROGRAM)` is called, `_mode` (which is `0`) != `3`. The mode change executes. `_mode` becomes `3`.
When `SetMode(EBYTE_MODE_NORMAL)` is called, `_mode` (which is `3`) != `0`. The mode change executes. `_mode` becomes `0`.
If the user code redundantly calls `SetMode(EBYTE_MODE_NORMAL)` in their loop, `_mode == 0` evaluates to true, and it returns early, saving 100+ms!

3. Power loss / recovery:
If the MCU resets, the object is recreated and `_mode` defaults to `0xFF`. The state synchronizes successfully. If the EBYTE module loses power but the MCU doesn't (rare edge case without an explicit reset line), the EBYTE module will power back on in NORMAL mode by default (due to hardware pull-downs on M0/M1 or simply reading the LOW state). The MCU tracking state as `0` perfectly matches the hardware.

Conclusion: The logic is completely sound.
