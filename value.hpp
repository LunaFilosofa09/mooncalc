#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using Num = long double;
using Complex = std::complex<Num>;

struct CalcError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

inline bool isReal(const Complex& z) {
    return std::fabs(z.imag()) <= 1e-13L * (1 + std::fabs(z.real()));
}

struct Matrix {
    size_t rows = 0, cols = 0;
    std::vector<Complex> data;

    Matrix() = default;
    Matrix(size_t r, size_t c) : rows(r), cols(c), data(r * c) {}

    Complex& at(size_t r, size_t c) { return data[r * cols + c]; }
    const Complex& at(size_t r, size_t c) const { return data[r * cols + c]; }
};

struct Value {
    Complex scalar;
    Matrix matrix;
    bool isMatrix = false;

    Value() = default;
    Value(Complex c) : scalar(c) {}
    Value(Num r) : scalar(r, 0) {}
    Value(Matrix m) : matrix(std::move(m)), isMatrix(true) {}
};

namespace mat {

inline Matrix identity(size_t n) {
    Matrix m(n, n);
    for (size_t i = 0; i < n; ++i) m.at(i, i) = 1;
    return m;
}

inline Matrix transpose(const Matrix& a) {
    Matrix t(a.cols, a.rows);
    for (size_t i = 0; i < a.rows; ++i)
        for (size_t j = 0; j < a.cols; ++j) t.at(j, i) = a.at(i, j);
    return t;
}

inline Matrix multiply(const Matrix& a, const Matrix& b) {
    if (a.cols != b.rows) throw CalcError("dimension mismatch in matrix product");
    Matrix r(a.rows, b.cols);
    for (size_t i = 0; i < a.rows; ++i)
        for (size_t k = 0; k < a.cols; ++k)
            for (size_t j = 0; j < b.cols; ++j) r.at(i, j) += a.at(i, k) * b.at(k, j);
    return r;
}

inline Num maxAbs(const Matrix& a) {
    Num m = 0;
    for (const auto& z : a.data) m = std::max(m, std::abs(z));
    return m;
}

inline Complex determinant(Matrix a) {
    if (a.rows != a.cols) throw CalcError("determinant requires a square matrix");
    size_t n = a.rows;
    Complex det = 1;
    for (size_t c = 0; c < n; ++c) {
        size_t p = c;
        for (size_t r = c + 1; r < n; ++r)
            if (std::abs(a.at(r, c)) > std::abs(a.at(p, c))) p = r;
        if (std::abs(a.at(p, c)) == 0) return 0;
        if (p != c) {
            for (size_t j = 0; j < n; ++j) std::swap(a.at(p, j), a.at(c, j));
            det = -det;
        }
        det *= a.at(c, c);
        for (size_t r = c + 1; r < n; ++r) {
            Complex f = a.at(r, c) / a.at(c, c);
            for (size_t j = c; j < n; ++j) a.at(r, j) -= f * a.at(c, j);
        }
    }
    return det;
}

inline Matrix inverse(Matrix a) {
    if (a.rows != a.cols) throw CalcError("inverse requires a square matrix");
    size_t n = a.rows;
    Num eps = 1e-13L * std::max<Num>(1, maxAbs(a));
    Matrix inv = identity(n);
    for (size_t c = 0; c < n; ++c) {
        size_t p = c;
        for (size_t r = c + 1; r < n; ++r)
            if (std::abs(a.at(r, c)) > std::abs(a.at(p, c))) p = r;
        if (std::abs(a.at(p, c)) <= eps) throw CalcError("matrix is singular");
        if (p != c)
            for (size_t j = 0; j < n; ++j) {
                std::swap(a.at(p, j), a.at(c, j));
                std::swap(inv.at(p, j), inv.at(c, j));
            }
        Complex d = a.at(c, c);
        for (size_t j = 0; j < n; ++j) {
            a.at(c, j) /= d;
            inv.at(c, j) /= d;
        }
        for (size_t r = 0; r < n; ++r) {
            if (r == c) continue;
            Complex f = a.at(r, c);
            if (f == Complex(0)) continue;
            for (size_t j = 0; j < n; ++j) {
                a.at(r, j) -= f * a.at(c, j);
                inv.at(r, j) -= f * inv.at(c, j);
            }
        }
    }
    return inv;
}

inline size_t rank(Matrix a) {
    Num eps = 1e-10L * std::max<Num>(1, maxAbs(a));
    size_t r = 0;
    for (size_t c = 0; c < a.cols && r < a.rows; ++c) {
        size_t p = r;
        for (size_t i = r + 1; i < a.rows; ++i)
            if (std::abs(a.at(i, c)) > std::abs(a.at(p, c))) p = i;
        if (std::abs(a.at(p, c)) <= eps) continue;
        for (size_t j = 0; j < a.cols; ++j) std::swap(a.at(p, j), a.at(r, j));
        for (size_t i = r + 1; i < a.rows; ++i) {
            Complex f = a.at(i, c) / a.at(r, c);
            for (size_t j = c; j < a.cols; ++j) a.at(i, j) -= f * a.at(r, j);
        }
        ++r;
    }
    return r;
}

}

namespace ops {

template <class F>
Value zip(const Value& a, const Value& b, const char* op, F f) {
    if (!a.isMatrix && !b.isMatrix) return Value(f(a.scalar, b.scalar));
    if (a.isMatrix && b.isMatrix) {
        if (a.matrix.rows != b.matrix.rows || a.matrix.cols != b.matrix.cols)
            throw CalcError(std::string("dimension mismatch in ") + op);
        Matrix m(a.matrix.rows, a.matrix.cols);
        for (size_t i = 0; i < m.data.size(); ++i) m.data[i] = f(a.matrix.data[i], b.matrix.data[i]);
        return Value(m);
    }
    const Matrix& src = a.isMatrix ? a.matrix : b.matrix;
    Matrix m(src.rows, src.cols);
    for (size_t i = 0; i < m.data.size(); ++i)
        m.data[i] = a.isMatrix ? f(a.matrix.data[i], b.scalar) : f(a.scalar, b.matrix.data[i]);
    return Value(m);
}

inline Value add(const Value& a, const Value& b) {
    return zip(a, b, "addition", [](Complex x, Complex y) { return x + y; });
}

inline Value sub(const Value& a, const Value& b) {
    return zip(a, b, "subtraction", [](Complex x, Complex y) { return x - y; });
}

inline Value mul(const Value& a, const Value& b) {
    if (a.isMatrix && b.isMatrix) return Value(mat::multiply(a.matrix, b.matrix));
    return zip(a, b, "multiplication", [](Complex x, Complex y) { return x * y; });
}

inline Value div(const Value& a, const Value& b) {
    if (b.isMatrix) throw CalcError("cannot divide by a matrix (use inv)");
    if (b.scalar == Complex(0)) throw CalcError("division by zero");
    return zip(a, b, "division", [](Complex x, Complex y) { return x / y; });
}

inline Value neg(const Value& a) { return zip(a, Value(Num(-1)), "negation", [](Complex x, Complex y) { return x * y; }); }

inline Complex scalarPow(const Complex& a, const Complex& b) {
    if (isReal(a) && isReal(b)) {
        Num x = a.real(), y = b.real();
        if (x >= 0 || y == std::floor(y)) return Complex(std::pow(x, y), 0);
    }
    if (a == Complex(0)) return b == Complex(0) ? Complex(1) : Complex(0);
    return std::pow(a, b);
}

inline Value pow(const Value& a, const Value& b) {
    if (b.isMatrix) throw CalcError("a matrix cannot be an exponent");
    if (!a.isMatrix) return Value(scalarPow(a.scalar, b.scalar));
    if (a.matrix.rows != a.matrix.cols) throw CalcError("matrix power requires a square matrix");
    if (!isReal(b.scalar) || b.scalar.real() != std::floor(b.scalar.real()))
        throw CalcError("matrix power requires an integer exponent");
    long long n = (long long)b.scalar.real();
    Matrix base = a.matrix;
    if (n < 0) {
        base = mat::inverse(base);
        n = -n;
    }
    Matrix result = mat::identity(a.matrix.rows);
    while (n > 0) {
        if (n & 1) result = mat::multiply(result, base);
        base = mat::multiply(base, base);
        n >>= 1;
    }
    return Value(result);
}

}
