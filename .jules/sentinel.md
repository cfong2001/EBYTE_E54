## 2026-04-26 - [Terminal Injection & Path Validation]
**Vulnerability:** Untrusted user input for serial ports was directly interpolated into warning messages allowing terminal injection, and the regex for Unix paths lacked character device validation allowing arbitrary file path traversal.
**Learning:** Even internal CLI scripts need robust input validation and sanitized logging (`repr()`). Without checking `stat.S_ISCHR`, any file matching the pattern could be opened instead of an actual serial device.
**Prevention:** Always wrap untrusted inputs in `repr()` before printing to terminal to escape ANSI escape codes. Always use `os.stat` with `stat.S_ISCHR` to verify character devices on Unix systems when opening user-provided paths.
## 2024-05-05 - [Serial Buffer Overflow DoS]
**Vulnerability:** The configuration import process appended incoming serial data to a String buffer without checking its length, allowing a Denial of Service (DoS) attack via memory exhaustion if an attacker sent an endless stream of characters without a closing bracket.
**Learning:** Even low-level hardware interfaces like UART/Serial must enforce strict input size bounds, especially when allocating strings dynamically on constrained microcontrollers.
**Prevention:** Always implement a maximum length check on buffers receiving untrusted data and handle overflows gracefully by clearing the buffer and aborting the operation.
## 2026-05-07 - [Improper Input Validation in CLI Prompts]
**Vulnerability:** Interactive CLI prompts for retry/quit actions accepted any input other than 'q' and treated it as a default 'retry' action, allowing arbitrary input to bypass the intended prompt choices.
**Learning:** For security and reliability, input prompts should use strict validation. Failing to validate user input against a finite set of allowed values violates CWE-20 and can lead to unexpected behavior.
**Prevention:** Always use a `while True` loop to strictly validate user input against expected values (e.g., `['r', 'q']`) before proceeding. Do not use default fallback actions for arbitrary input.
