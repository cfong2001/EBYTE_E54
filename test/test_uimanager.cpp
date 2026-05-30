#include <iostream>
#include <cassert>
#include <cmath>

#define private public
#include "UIManager.h"
#undef private

void test_uimanager_state_transitions() {
    TFT_eSPI dummy_tft;
    UIManager ui(dummy_tft);

    // Initial state check
    assert(ui.state == STATE_BOOT);
    assert(ui.activePage == PAGE_MAIN);

    // Test DevMenuView handling menu click (WIPING PREFERENCES)
    ui.activePage = PAGE_DEV;
    ui.activeView = ui.devMenuView;
    ui.devRiskAccepted = true;
    ui.menuSelection = ui.maxMenuSelection - 4; // STATE_CONFIRM_RESET

    ui.handleMenuClick();
    assert(ui.state == STATE_CONFIRM_RESET);

    std::cout << "test_uimanager_state_transitions passed\n";
}

void test_uimanager_visuals_menu() {
    TFT_eSPI dummy_tft;
    UIManager ui(dummy_tft);

    // Set to Visuals Menu
    ui.activePage = PAGE_VISUALS;
    ui.activeView = ui.visualsMenuView;
    ui.menuSelection = 0; // Return to MAIN

    ui.handleMenuClick();
    assert(ui.activePage == PAGE_MAIN);
    assert(ui.menuSelection == 0);

    std::cout << "test_uimanager_visuals_menu passed\n";
}

void test_uimanager_all() {
    std::cout << "Running UIManager tests...\n";
    test_uimanager_state_transitions();
    test_uimanager_visuals_menu();
}
#include "../src/Themes.h"
int numUserThemes = 3;
Theme userThemes[10] = { FALLBACK_THEME, FALLBACK_THEME, FALLBACK_THEME };
