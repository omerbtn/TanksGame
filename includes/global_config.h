#pragma once

#include "config_generated.h"  // Generated at build time

#include <array>
#include <string_view>

namespace config {

struct ConfigNotFoundError {};
struct ConfigInvalidIntError {};
struct ConfigInvalidBoolError {};

constexpr std::string_view get_config_value(std::string_view key) {
    for (const auto& entry : config_entries) {
        if (entry.key == key) {
            return entry.value;
        }
    }
    throw ConfigNotFoundError{};
}

constexpr int parse_int(std::string_view sv) {
    int result = 0;
    bool valid = false;
    for (char c : sv) {
        if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
            valid = true;
        } else {
            throw ConfigInvalidIntError{};
        }
    }
    if (!valid) {
        throw ConfigInvalidIntError{};
    }
    return result;
}

constexpr bool parse_bool(std::string_view sv) {
    if (sv == "true") return true;
    if (sv == "false") return false;
    throw ConfigInvalidBoolError{};
}

template <typename T>
constexpr T get(std::string_view key);

template <>
constexpr int get<int>(std::string_view key) {
    return parse_int(get_config_value(key));
}

template <>
constexpr size_t get<size_t>(std::string_view key) {
    return static_cast<size_t>(get<int>(key));
}

template <>
constexpr bool get<bool>(std::string_view key) {
    return parse_bool(get_config_value(key));
}

template <>
constexpr std::string_view get<std::string_view>(std::string_view key) {
    return get_config_value(key);
}

}  // namespace config
