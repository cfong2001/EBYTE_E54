#include <iostream>

void test_zone_manager_all();
int main_perf();
void test_radar_all();
void test_broadcast_server_updateData();

int main() {
    test_zone_manager_all();
    main_perf();
    test_radar_all();
    test_broadcast_server_updateData();
    return 0;
}
