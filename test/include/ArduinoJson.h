#ifndef MOCK_ARDUINOJSON_H
#define MOCK_ARDUINOJSON_H

#include <string>

class JsonObject {
public:
    template<typename T>
    void operator=(T val) {}

    // Mock operator[] that returns something assignable
    struct MockAssigner {
        template<typename T>
        void operator=(T val) {}
    };

    MockAssigner operator[](const char* key) {
        return MockAssigner();
    }
};

class JsonArray {
public:
    template<typename T>
    T add() { return T(); }
};

class JsonDocument {
public:
    // Mock operator[] that returns something that can call to<JsonArray>()
    struct MockArrayCreator {
        template<typename T>
        T to() { return T(); }
    };

    MockArrayCreator operator[](const char* key) {
        return MockArrayCreator();
    }
};

inline void serializeJson(JsonDocument& doc, std::string& output) {
    output = "{}";
}
#define String std::string

#endif
