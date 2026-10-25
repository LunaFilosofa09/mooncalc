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

inline Matrix rref(Matrix a, std::vector<size_t>* pivots = nullptr) {
    Num eps = 1e-10L * std::max<Num>(1, maxAbs(a));
    size_t r = 0;
    for (size_t c = 0; c < a.cols && r < a.rows; ++c) {
        size_t p = r;
        for (size_t i = r + 1; i < a.rows; ++i)
            if (std::abs(a.at(i, c)) > std::abs(a.at(p, c))) p = i;
        if (std::abs(a.at(p, c)) <= eps) {
            for (size_t i = r; i < a.rows; ++i) a.at(i, c) = 0;
            continue;
        }
        for (size_t j = 0; j < a.cols; ++j) std::swap(a.at(p, j), a.at(r, j));
        Complex d = a.at(r, c);
        for (size_t j = 0; j < a.cols; ++j) a.at(r, j) /= d;
        for (size_t i = 0; i < a.rows; ++i) {
            if (i == r) continue;
            Complex f = a.at(i, c);
            if (f == Complex(0)) continue;
            for (size_t j = 0; j < a.cols; ++j) a.at(i, j) -= f * a.at(r, j);
        }
        if (pivots) pivots->push_back(c);
        ++r;
    }
    for (auto& z : a.data) {
        if (std::fabs(z.real()) <= eps) z = Complex(0, z.imag());
        if (std::fabs(z.imag()) <= eps) z = Complex(z.real(), 0);
    }
    return a;
}

inline Matrix nullspace(const Matrix& a) {
    std::vector<size_t> pivots;
    Matrix r = rref(a, &pivots);
    std::vector<size_t> freeCols;
    for (size_t c = 0; c < a.cols; ++c)
        if (std::find(pivots.begin(), pivots.end(), c) == pivots.end()) freeCols.push_back(c);
    if (freeCols.empty()) throw CalcError("nullspace is trivial (full column rank)");
    Matrix n(a.cols, freeCols.size());
    for (size_t k = 0; k < freeCols.size(); ++k) {
        n.at(freeCols[k], k) = 1;
        for (size_t i = 0; i < pivots.size(); ++i) n.at(pivots[i], k) = -r.at(i, freeCols[k]);
    }
    return n;
}

inline Matrix linsolve(const Matrix& a, const Matrix& b) {
    if (a.rows != b.rows) throw CalcError("A and b must have the same number of rows");
    Matrix aug(a.rows, a.cols + b.cols);
    for (size_t i = 0; i < a.rows; ++i) {
        for (size_t j = 0; j < a.cols; ++j) aug.at(i, j) = a.at(i, j);
        for (size_t j = 0; j < b.cols; ++j) aug.at(i, a.cols + j) = b.at(i, j);
    }
    std::vector<size_t> pivots;
    Matrix r = rref(aug, &pivots);
    for (size_t p : pivots)
        if (p >= a.cols) throw CalcError("the system has no solution");
    if (pivots.size() < a.cols) throw CalcError("the system has infinitely many solutions (rank " + std::to_string(pivots.size()) + " < " + std::to_string(a.cols) + ")");
    Matrix x(a.cols, b.cols);
    for (size_t i = 0; i < a.cols; ++i)
        for (size_t j = 0; j < b.cols; ++j) x.at(i, j) = r.at(i, a.cols + j);
    return x;
}

inline Num norm1(const Matrix& a) {
    Num best = 0;
    for (size_t j = 0; j < a.cols; ++j) {
        Num s = 0;
        for (size_t i = 0; i < a.rows; ++i) s += std::abs(a.at(i, j));
        best = std::max(best, s);
    }
    return best;
}

inline Matrix expm(const Matrix& a) {
    if (a.rows != a.cols) throw CalcError("expm requires a square matrix");
    Num norm = 0;
    for (size_t i = 0; i < a.rows; ++i) {
        Num s = 0;
        for (size_t j = 0; j < a.cols; ++j) s += std::abs(a.at(i, j));
        norm = std::max(norm, s);
    }
    int squarings = norm > 0.5L ? (int)std::ceil(std::log2(norm / 0.5L)) : 0;
    Matrix scaled = a;
    for (auto& z : scaled.data) z /= std::pow(2.0L, squarings);
    Matrix result = identity(a.rows), term = identity(a.rows);
    for (int k = 1; k <= 24; ++k) {
        term = multiply(term, scaled);
        for (auto& z : term.data) z /= (Num)k;
        for (size_t i = 0; i < result.data.size(); ++i) result.data[i] += term.data[i];
    }
    for (int s = 0; s < squarings; ++s) result = multiply(result, result);
    return result;
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

inline Value elementMul(const Value& a, const Value& b) {
    return zip(a, b, "element-wise multiplication", [](Complex x, Complex y) { return x * y; });
}

inline Value elementDiv(const Value& a, const Value& b) {
    return zip(a, b, "element-wise division", [](Complex x, Complex y) {
        if (y == Complex(0)) throw CalcError("division by zero");
        return x / y;
    });
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

inline Value elementPow(const Value& a, const Value& b) {
    return zip(a, b, "element-wise power", [](Complex x, Complex y) { return scalarPow(x, y); });
}

inline Value leftDivide(const Value& a, const Value& b) {
    if (!a.isMatrix) return div(b, a);
    if (!b.isMatrix) throw CalcError("A\\b requires b to be a vector or matrix");
    return Value(mat::linsolve(a.matrix, b.matrix));
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
