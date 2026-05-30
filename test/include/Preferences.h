#pragma once
#include <string>
#include <map>

class Preferences {
public:
    void begin(const char* name, bool readOnly) {}
    void end() {}

    int getInt(const char* key, int defaultValue) {
        auto it = _intValues.find(key);
        if (it != _intValues.end()) {
            return it->second;
        }
        return defaultValue;
    }

    void putInt(const char* key, int value) {
        _intValues[key] = value;
    }

    bool getBool(const char* key, bool defaultValue) {
        return defaultValue;
    }

    void putBool(const char* key, bool value) {}

    float getFloat(const char* key, float defaultValue) {
        return defaultValue;
    }

    void putFloat(const char* key, float value) {}

    std::string getString(const char* key, const char* defaultValue) {
        return std::string(defaultValue);
    }

    void putString(const char* key, const class String& value) {}
    void clear() {}

private:
    std::map<std::string, int> _intValues;
};
