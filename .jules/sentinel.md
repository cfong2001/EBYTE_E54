## 2026-04-26 - [Terminal Injection & Path Validation]
**Vulnerability:** Untrusted user input for serial ports was directly interpolated into warning messages allowing terminal injection, and the regex for Unix paths lacked character device validation allowing arbitrary file path traversal.
**Learning:** Even internal CLI scripts need robust input validation and sanitized logging (`repr()`). Without checking `stat.S_ISCHR`, any file matching the pattern could be opened instead of an actual serial device.
**Prevention:** Always wrap untrusted inputs in `repr()` before printing to terminal to escape ANSI escape codes. Always use `os.stat` with `stat.S_ISCHR` to verify character devices on Unix systems when opening user-provided paths.
