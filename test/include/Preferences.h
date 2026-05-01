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

private:
    std::map<std::string, int> _intValues;
};
