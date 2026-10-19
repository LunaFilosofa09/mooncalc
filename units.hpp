#pragma once

#include <string>
#include <vector>

#include "value.hpp"

namespace units {

struct Unit {
    const char* name;
    const char* dimension;
    Num factor;
};

inline const std::vector<Unit>& table() {
    static const std::vector<Unit> t = {
        {"m", "length", 1},          {"km", "length", 1000},        {"cm", "length", 0.01L},
        {"mm", "length", 0.001L},    {"mi", "length", 1609.344L},   {"yd", "length", 0.9144L},
        {"ft", "length", 0.3048L},   {"in", "length", 0.0254L},     {"nmi", "length", 1852},
        {"kg", "mass", 1},           {"g", "mass", 0.001L},         {"mg", "mass", 1e-6L},
        {"lb", "mass", 0.45359237L}, {"oz", "mass", 0.028349523125L}, {"t", "mass", 1000},
        {"s", "time", 1},            {"ms", "time", 0.001L},        {"min", "time", 60},
        {"h", "time", 3600},         {"day", "time", 86400},        {"week", "time", 604800},
        {"l", "volume", 1},          {"ml", "volume", 0.001L},      {"gal", "volume", 3.785411784L},
        {"m3", "volume", 1000},      {"m/s", "speed", 1},           {"km/h", "speed", 1 / 3.6L},
        {"mph", "speed", 0.44704L},  {"kn", "speed", 0.514444444444L},
        {"deg", "angle", 1},         {"rad", "angle", 57.29577951308232L}, {"grad", "angle", 0.9L},
        {"B", "data", 1},            {"KB", "data", 1e3L},          {"MB", "data", 1e6L},
        {"GB", "data", 1e9L},        {"KiB", "data", 1024},         {"MiB", "data", 1048576},
        {"GiB", "data", 1073741824}, {"J", "energy", 1},            {"kJ", "energy", 1000},
        {"cal", "energy", 4.184L},   {"kcal", "energy", 4184},      {"Wh", "energy", 3600},
        {"kWh", "energy", 3.6e6L},   {"Pa", "pressure", 1},         {"kPa", "pressure", 1000},
        {"bar", "pressure", 1e5L},   {"atm", "pressure", 101325},   {"psi", "pressure", 6894.757293168L},
    };
    return t;
}

inline const Unit* find(const std::string& name) {
    for (const auto& u : table())
        if (name == u.name) return &u;
    return nullptr;
}

inline bool isTemperature(const std::string& n) { return n == "C" || n == "F" || n == "K"; }

inline Num toKelvin(Num v, const std::string& u) {
    if (u == "C") return v + 273.15L;
    if (u == "F") return (v - 32) * 5 / 9 + 273.15L;
    return v;
}

inline Num fromKelvin(Num k, const std::string& u) {
    if (u == "C") return k - 273.15L;
    if (u == "F") return (k - 273.15L) * 9 / 5 + 32;
    return k;
}

inline Num convert(Num value, const std::string& from, const std::string& to) {
    if (isTemperature(from) || isTemperature(to)) {
        if (!(isTemperature(from) && isTemperature(to))) throw CalcError("cannot convert between '" + from + "' and '" + to + "'");
        Num k = toKelvin(value, from);
        if (k < 0) throw CalcError("temperature below absolute zero");
        return fromKelvin(k, to);
    }
    const Unit* a = find(from);
    const Unit* b = find(to);
    if (!a) throw CalcError("unknown unit '" + from + "'");
    if (!b) throw CalcError("unknown unit '" + to + "'");
    if (std::string(a->dimension) != b->dimension)
        throw CalcError("cannot convert " + std::string(a->dimension) + " to " + b->dimension);
    return value * a->factor / b->factor;
}

inline std::vector<std::string> listing() {
    std::vector<std::string> lines;
    std::string current, line;
    for (const auto& u : table()) {
        if (current != u.dimension) {
            if (!line.empty()) lines.push_back(line);
            current = u.dimension;
            line = current + ":";
        }
        line += std::string(" ") + u.name;
    }
    lines.push_back(line);
    lines.push_back("temperature: C F K");
    return lines;
}

}
