#pragma once
#include <string>

class String {
    std::string str;
public:
    String(const char* s = "") : str(s) {}
    String(std::string s) : str(s) {}

    bool startsWith(const char* prefix) const {
        return str.find(prefix) == 0;
    }

    const char* c_str() const {
        return str.c_str();
    }

    String& operator+=(char c) {
        str += c;
        return *this;
    }

    String& operator+=(const char* c) {
        str += c;
        return *this;
    }
};
