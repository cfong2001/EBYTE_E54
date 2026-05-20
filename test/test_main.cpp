#include <iostream>

void test_zone_manager_all();
int main_perf();
int test_motion_compensation_main();
void test_broadcast_server_updateData();

int main() {
    test_motion_compensation_main();
    test_zone_manager_all();
    main_perf();
    test_broadcast_server_updateData();
    return 0;
}
