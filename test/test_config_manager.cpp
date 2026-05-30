#include <iostream>
#include <cassert>

#define UIMANAGER_H
class UIManager {};
#define ICON_SMART 1
#define TELEMETRY_OFF 0

#include "ZoneManager.h"
#include "../src/Themes.h"

#define private public
#include "ConfigManager.h"
#undef private

void test_generateWiFiPassword() {
    std::cout << "Running test_generateWiFiPassword..." << std::endl;
    ConfigManager cm;

    String pass1 = cm.generateWiFiPassword();
    assert(pass1.length() == 12);

    // Check characters are valid
    bool allValid = true;
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(size_t i = 0; i < pass1.length(); i++) {
        bool found = false;
        for(size_t j = 0; j < sizeof(charset) - 1; j++) {
            if(pass1.c_str()[i] == charset[j]) {
                found = true;
                break;
            }
        }
        if(!found) {
            allValid = false;
            break;
        }
    }
    assert(allValid == true);

    // Verify it was saved to preferences
    assert(cm.prefs.getString("wifi_pass") == pass1);

    std::cout << "  ✓ test_generateWiFiPassword passed" << std::endl;
}

void test_isFallbackPending() {
    std::cout << "Running test_isFallbackPending..." << std::endl;
    ConfigManager cm;

    cm.prefs.putBool("fb_pend", true);
    assert(cm.isFallbackPending() == true);

    cm.prefs.putBool("fb_pend", false);
    assert(cm.isFallbackPending() == false);

    std::cout << "  ✓ test_isFallbackPending passed" << std::endl;
}

void test_checkFallback() {
    std::cout << "Running test_checkFallback..." << std::endl;
    ConfigManager cm;

    // Case 1: not pending
    cm.prefs.putBool("fb_pend", false);
    assert(cm.checkFallback() == false);

    // Case 2: pending, boots < 2
    cm.prefs.putBool("fb_pend", true);
    cm.prefs.putInt("fb_boots", 0);
    assert(cm.checkFallback() == true);
    assert(cm.prefs.getInt("fb_boots", 0) == 1);

    // Note: Boot loop case with boots >= 2 triggers ESP.restart() which is mocked to do nothing,
    // but we can't easily assert on restoreFromFallback() side effects without more complex setup,
    // so we'll stick to basic state tests.

    std::cout << "  ✓ test_checkFallback passed" << std::endl;
}

void test_config_manager_all() {
    std::cout << "Testing ConfigManager..." << std::endl;
    test_generateWiFiPassword();
    test_isFallbackPending();
    test_checkFallback();
    std::cout << "All ConfigManager tests passed!" << std::endl;
}
