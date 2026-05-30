#pragma once

#include <string>
#include <map>

class Preferences {
public:
    void begin(const char* name, bool readOnly) {}
    void end() {}
    void clear() { _intValues.clear(); _boolValues.clear(); _stringValues.clear(); }

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


    bool getBool(const char* key, bool defaultValue = false) {
        auto it = _boolValues.find(key);
        if (it != _boolValues.end()) return it->second;
        return defaultValue;
    }

    void putBool(const char* key, bool value) {
        _boolValues[key] = value;
    }

    String getString(const char* key, String defaultValue = "") {
        auto it = _stringValues.find(key);
        if (it != _stringValues.end()) return it->second;
        return defaultValue;
    }

    void putString(const char* key, String value) {
        _stringValues[key] = value;
    }

private:
    std::map<std::string, bool> _boolValues;
    std::map<std::string, String> _stringValues;

    std::map<std::string, int> _intValues;
};
