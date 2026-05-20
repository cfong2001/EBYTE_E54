#include <iostream>

void test_zone_manager_all();
void test_motion_compensation_all();
int main_perf();

int main() {
    test_zone_manager_all();
    test_motion_compensation_all();
    main_perf();
    return 0;
}
