#pragma once

#include "Arduino.h"
#include <map>
#include <string>
#include <vector>

class JsonObject {
public:
    std::map<std::string, String> vals;
    String& operator[](const char* key) { return vals[key]; }
};

class JsonVariant {
public:
    std::string val_str;
    int val_int = 0;
    bool val_bool = false;
    double val_double = 0.0;

    JsonVariant() {}
    JsonVariant(int v) : val_int(v) {}
    JsonVariant(bool v) : val_bool(v) {}
    JsonVariant(const char* v) : val_str(v) {}
    JsonVariant(const String& v) : val_str(v.c_str()) {}
    JsonVariant(double v) : val_double(v) {}

    JsonVariant& operator=(int v) { val_int = v; return *this; }
    JsonVariant& operator=(bool v) { val_bool = v; return *this; }
    JsonVariant& operator=(const char* v) { val_str = v; return *this; }
    JsonVariant& operator=(const String& v) { val_str = v.c_str(); return *this; }
    JsonVariant& operator=(double v) { val_double = v; return *this; }

    String as_String() const { return String(val_str.c_str()); }
    template<typename T> T as() const;
    template<typename T> bool is() const { return true; } // mock

    JsonVariant operator[](const char* key) const {
        return JsonVariant();
    }
};

template<> inline int JsonVariant::as<int>() const { return val_int; }
template<> inline bool JsonVariant::as<bool>() const { return val_bool; }
template<> inline String JsonVariant::as<String>() const { return String(val_str.c_str()); }
template<> inline const char* JsonVariant::as<const char*>() const { return val_str.c_str(); }


class JsonArray {
public:
    std::vector<JsonVariant> arr;

    std::vector<JsonVariant>::const_iterator begin() const { return arr.begin(); }
    std::vector<JsonVariant>::const_iterator end() const { return arr.end(); }

    template<typename T> T add() { return T(); }
};

template<> inline JsonArray JsonVariant::as<JsonArray>() const { return JsonArray(); }

class JsonDocument {
public:
    std::map<std::string, JsonVariant> doc;

    JsonVariant& operator[](const char* key) {
        return doc[key];
    }

    template<typename T> bool is() const { return true; } // mock
    template<typename T> T as() const;
    template<typename T> T to() { return T(); }
};

template<> inline JsonArray JsonDocument::as<JsonArray>() const { return JsonArray(); }

class DeserializationError {
public:
    operator bool() const { return false; }
    const char* c_str() const { return ""; }
};

inline DeserializationError deserializeJson(JsonDocument& doc, const String& jsonStr) {
    return DeserializationError();
}

inline void serializeJson(const JsonDocument& doc, MockSerial& serial) {
    serial.println("{}");
}
inline void serializeJson(const JsonDocument& doc, String& s) {
    s = "{}";
}
