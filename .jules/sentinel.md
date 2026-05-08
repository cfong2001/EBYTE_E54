## 2026-04-26 - [Terminal Injection & Path Validation]
**Vulnerability:** Untrusted user input for serial ports was directly interpolated into warning messages allowing terminal injection, and the regex for Unix paths lacked character device validation allowing arbitrary file path traversal.
**Learning:** Even internal CLI scripts need robust input validation and sanitized logging (`repr()`). Without checking `stat.S_ISCHR`, any file matching the pattern could be opened instead of an actual serial device.
**Prevention:** Always wrap untrusted inputs in `repr()` before printing to terminal to escape ANSI escape codes. Always use `os.stat` with `stat.S_ISCHR` to verify character devices on Unix systems when opening user-provided paths.
## 2024-05-05 - [Serial Buffer Overflow DoS]
**Vulnerability:** The configuration import process appended incoming serial data to a String buffer without checking its length, allowing a Denial of Service (DoS) attack via memory exhaustion if an attacker sent an endless stream of characters without a closing bracket.
**Learning:** Even low-level hardware interfaces like UART/Serial must enforce strict input size bounds, especially when allocating strings dynamically on constrained microcontrollers.
**Prevention:** Always implement a maximum length check on buffers receiving untrusted data and handle overflows gracefully by clearing the buffer and aborting the operation.
## 2024-05-08 - [CLI Input Validation (CWE-20)]
**Vulnerability:** Interactive CLI prompts allowed arbitrary user input to bypass validation and trigger the default fallback behavior (retry) because the input was only checked against one valid option (`'q'`) instead of enforcing a strict finite set (`['r', 'q']`).
**Learning:** Even simple CLI utilities need strict input validation. Allowing unrecognized input to default to an active operation can lead to unexpected loops or state transitions (CWE-20).
**Prevention:** Always wrap interactive `input()` calls in a `while True` loop that strictly validates against a predefined list of allowed choices, reprompting until valid input is received.
