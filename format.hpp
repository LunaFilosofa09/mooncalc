#pragma once

#include <cmath>
#include <sstream>
#include <string>

#include "value.hpp"

namespace fmtx {

inline std::string number(Num v) {
    if (std::isnan(v)) return "nan";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    if (std::fabs(v) < 1e-14L) v = 0;
    std::ostringstream os;
    os.precision(15);
    os << v;
    return os.str();
}

inline std::string complex(const Complex& z) {
    if (isReal(z)) return number(z.real());
    Num re = std::fabs(z.real()) < 1e-14L ? 0 : z.real();
    Num im = z.imag();
    std::string mag = std::fabs(std::fabs(im) - 1) < 1e-14L ? "" : number(std::fabs(im));
    bool neg = im < 0;
    if (re == 0) return std::string(neg ? "-" : "") + mag + "i";
    return number(re) + (neg ? " - " : " + ") + mag + "i";
}

inline std::string inlineValue(const Value& v) {
    if (!v.isMatrix) return complex(v.scalar);
    std::string s = "[";
    for (size_t i = 0; i < v.matrix.rows; ++i) {
        if (i) s += "; ";
        for (size_t j = 0; j < v.matrix.cols; ++j) s += (j ? ", " : "") + complex(v.matrix.at(i, j));
    }
    return s + "]";
}

}
