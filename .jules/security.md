## 2024-05-30 - Replace random() with esp_random() for cryptographic needs
**Learning:** `random()` from the standard library is predictable. `esp_random()` utilizes the ESP32 hardware random number generator and is suitable for cryptographic or sensitive needs like password generation.
**Action:** When generating credentials or keys on the ESP32 platform, use `esp_random()` combined with modulo arithmetic for character generation instead of `random()`.
## 2024-05-30 - Fix Missing CORS Headers
**Learning:** When serving APIs directly from hardware devices over HTTP, missing CORS headers can easily lead to Cross-Site Request Forgery (CSRF) or data leakage if another web page accesses the hardware's local IP address.
**Action:** When creating new API routes using `ESPAsyncWebServer`, always verify that CORS headers (e.g., `Access-Control-Allow-Origin`) are present to ensure secure cross-origin interaction, especially for IoT devices operating on local networks. Use `request->beginResponse()` instead of the shortcut `request->send()` to gain access to the `AsyncWebServerResponse` object where headers can be added safely.
