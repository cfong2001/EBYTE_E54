## 2025-04-25 - HardwareSerial Buffer Size Optimization
**Learning:** For ESP32 hardware using high-speed UART (like 256000 bps for E54 radars), the default 256-byte HardwareSerial RX buffer can easily overflow within ~10ms if `loop()` execution delays.
**Action:** Use `HardwareSerial::setRxBufferSize(1024)` before calling `begin()` when working with baud rates 115200 or higher to prevent packet loss and improve reliability.
