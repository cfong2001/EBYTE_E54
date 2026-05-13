## 2026-04-26 - [Terminal Injection & Path Validation]
**Vulnerability:** Untrusted user input for serial ports was directly interpolated into warning messages allowing terminal injection, and the regex for Unix paths lacked character device validation allowing arbitrary file path traversal.
**Learning:** Even internal CLI scripts need robust input validation and sanitized logging (`repr()`). Without checking `stat.S_ISCHR`, any file matching the pattern could be opened instead of an actual serial device.
**Prevention:** Always wrap untrusted inputs in `repr()` before printing to terminal to escape ANSI escape codes. Always use `os.stat` with `stat.S_ISCHR` to verify character devices on Unix systems when opening user-provided paths.
## 2026-05-24 - [Secure Random Number Generation for Passwords]
**Vulnerability:** Weak or predictable random number generators used for passwords or pairing pins can lead to trivial dictionary attacks or unauthorized access.
**Learning:** Using `esp_random()` directly for creating WiFi SoftAP passwords provides a cryptographically secure pseudo-random number generator on the ESP32 platform, ensuring passwords are much harder to predict compared to `random()` or static defaults.
**Prevention:** Always use secure hardware-based PRNGs (like `esp_random()` on ESP32) when generating secrets, keys, or passwords dynamically.
