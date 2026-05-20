#ifndef MOCK_WIFI_H
#define MOCK_WIFI_H

class IPAddress {
public:
    IPAddress() {}
};

class MockWiFi {
public:
    void softAP(const char* ssid, const char* pass) {}
    IPAddress softAPIP() { return IPAddress(); }
    void softAPdisconnect(bool) {}
};

extern MockWiFi WiFi;

#endif
