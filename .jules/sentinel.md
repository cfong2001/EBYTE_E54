## 2026-04-26 - [Terminal Injection & Path Validation]
**Vulnerability:** Untrusted user input for serial ports was directly interpolated into warning messages allowing terminal injection, and the regex for Unix paths lacked character device validation allowing arbitrary file path traversal.
**Learning:** Even internal CLI scripts need robust input validation and sanitized logging (`repr()`). Without checking `stat.S_ISCHR`, any file matching the pattern could be opened instead of an actual serial device.
**Prevention:** Always wrap untrusted inputs in `repr()` before printing to terminal to escape ANSI escape codes. Always use `os.stat` with `stat.S_ISCHR` to verify character devices on Unix systems when opening user-provided paths.

## $(date +%Y-%m-%d) - Replaced Weak PRNG with Hardware RNG in Simulation
**Vulnerability:** The simulation logic in `src/main.cpp` used the Arduino `random()` function, which is a weak pseudorandom number generator (PRNG) and is not cryptographically secure. While its impact was limited to testing logic, it represents poor code hygiene and a potential security risk if copy-pasted into production code.
**Learning:** Arduino's default `random()` relies on a predictable PRNG. The ESP32 provides a secure hardware-based true random number generator via `esp_random()`.
**Prevention:** Always include `<esp_random.h>` and use `esp_random()` for random number generation in ESP32 projects to ensure cryptographic security.
