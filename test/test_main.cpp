#include <iostream>

void test_zone_manager_all();
int main_perf();
void test_config_manager_all();
int test_motion_compensation_main();
void test_radar_all();

int main() {
    test_motion_compensation_main();
    test_zone_manager_all();
    main_perf();
    test_radar_all();
    test_config_manager_all();
    return 0;
}
