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
## 2025-05-20 - Open WiFi Access Point
**Vulnerability:** The `WiFi.softAP` function was called with an empty password string, creating an open WiFi access point without authentication or encryption.
**Learning:** Hardcoding empty passwords for Access Points allows any nearby attacker to connect, potentially accessing sensitive telemetry or disrupting device functionality.
**Prevention:** Always provide a strong password (minimum 8 characters) when initializing a soft AP to enable WPA2-PSK encryption, preventing unauthorized access.
## YYYY-MM-DD - Sentinel: Wokwi Setup
**Insight:** Wokwi needs specific `diagram.json` and a placeholder sketch to provide simulated inputs on a separate UART channel without changing standard core code structure for local compilation.
**Action:** The user wanted to organize the wokwi files and provide a simulated input mechanism inside Wokwi. We created `wokwi/diagram.json` containing the ESP32 and UI peripherals. We provided a standalone `wokwi/sketch.ino` that feeds simulated byte streams to ESP32 RX over a loop so Wokwi users can test parsing algorithms directly without external python scripts or custom hardware logic.
## YYYY-MM-DD - Sentinel: Wokwi README
**Insight:** Users needed instructions on how to actually include their local `.cpp` and `.h` files into the Wokwi simulated environment.
**Action:** Created `wokwi/README.md` detailing the two main approaches (Wokwi Web Editor vs VS Code Extension) and updated `sketch.ino` to act as an explicit template outlining how to structure the web tabs to compile correctly with standard `src/` core logic.
## 2026-05-25 - Professional Code Standards
**Learning:** Unprofessional comments, emojis, and internal monologue in source code reduce readability and professional standards.
**Action:** Always maintain professional tone and remove internal monologues and excessive emojis.
