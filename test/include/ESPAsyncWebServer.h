#ifndef MOCK_ASYNC_WEB_SERVER_H
#define MOCK_ASYNC_WEB_SERVER_H

#include <functional>

#define HTTP_GET 0

class AsyncWebServerRequest {
public:
    void send(int code, const char* contentType, const char* content) {}
    void send(int code, const char* contentType, const std::string& content) {}
};

typedef std::function<void(AsyncWebServerRequest *request)> ArRequestHandlerFunction;

class AsyncWebServer {
public:
    AsyncWebServer(int port) {}
    void begin() {}
    void end() {}
    void on(const char* uri, int method, ArRequestHandlerFunction handler) {}
};

#endif
