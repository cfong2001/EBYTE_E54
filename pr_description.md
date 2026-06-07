🎯 **What:** The vulnerability fixed is the use of the standard Arduino `random()` function to generate a Wi-Fi password in `UIManager.h`.
⚠️ **Risk:** `random()` is not a cryptographically secure random number generator (CSPRNG). If left unfixed, an attacker could potentially predict the generated Wi-Fi password, leading to unauthorized access to the device or the network it connects to.
🛡️ **Solution:** Replaced `random()` with `esp_random()`, which utilizes the ESP32's true hardware random number generator, ensuring the generated password is secure and unpredictable.
