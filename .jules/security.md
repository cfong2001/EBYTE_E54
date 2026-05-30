## 2024-05-30 - Replace random() with esp_random() for cryptographic needs
**Learning:** `random()` from the standard library is predictable. `esp_random()` utilizes the ESP32 hardware random number generator and is suitable for cryptographic or sensitive needs like password generation.
**Action:** When generating credentials or keys on the ESP32 platform, use `esp_random()` combined with modulo arithmetic for character generation instead of `random()`.
