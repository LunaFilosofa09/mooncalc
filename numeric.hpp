#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "value.hpp"

namespace numeric {

using RealFn = std::function<Num(Num)>;

inline Num roundSig(Num v, int digits) {
    if (v == 0 || !std::isfinite(v)) return v;
    int e = (int)std::floor(std::log10(std::fabs(v)));
    Num scale = std::pow(10.0L, digits - 1 - e);
    if (!std::isfinite(scale) || scale == 0) return v;
    return std::round(v * scale) / scale;
}

inline Complex roundSig(Complex z, int digits) { return Complex(roundSig(z.real(), digits), roundSig(z.imag(), digits)); }

struct LinearResult {
    enum class Status { Unique, None, Infinite };
    Status status = Status::None;
    std::vector<Num> x;
};

inline LinearResult solveLinear(std::vector<std::vector<Num>> a, std::vector<Num> b) {
    size_t n = a.size();
    Num scale = 0;
    for (size_t i = 0; i < n; ++i) {
        for (Num v : a[i]) scale = std::max(scale, std::fabs(v));
        scale = std::max(scale, std::fabs(b[i]));
    }
    Num eps = 1e-10L * (scale > 0 ? scale : 1);

    std::vector<int> pivotCol(n, -1);
    size_t row = 0;
    for (size_t col = 0; col < n && row < n; ++col) {
        size_t p = row;
        for (size_t i = row + 1; i < n; ++i)
            if (std::fabs(a[i][col]) > std::fabs(a[p][col])) p = i;
        if (std::fabs(a[p][col]) <= eps) continue;
        std::swap(a[p], a[row]);
        std::swap(b[p], b[row]);
        Num d = a[row][col];
        for (size_t j = 0; j < n; ++j) a[row][j] /= d;
        b[row] /= d;
        for (size_t i = 0; i < n; ++i) {
            if (i == row) continue;
            Num f = a[i][col];
            if (f == 0) continue;
            for (size_t j = 0; j < n; ++j) a[i][j] -= f * a[row][j];
            b[i] -= f * b[row];
        }
        pivotCol[row] = (int)col;
        ++row;
    }

    LinearResult res;
    for (size_t i = row; i < n; ++i)
        if (std::fabs(b[i]) > eps) return res;
    if (row < n) {
        res.status = LinearResult::Status::Infinite;
        return res;
    }
    res.status = LinearResult::Status::Unique;
    res.x.assign(n, 0);
    for (size_t i = 0; i < n; ++i) res.x[pivotCol[i]] = b[i];
    return res;
}

inline Complex horner(const std::vector<Complex>& a, Complex z) {
    Complex r = a.back();
    for (size_t i = a.size() - 1; i-- > 0;) r = r * z + a[i];
    return r;
}

inline std::vector<Complex> polyRoots(const std::vector<Num>& c) {
    size_t d = c.size() - 1;
    std::vector<Complex> a(d + 1);
    for (size_t i = 0; i <= d; ++i) a[i] = c[i] / c[d];

    Num bound = 0;
    for (size_t i = 0; i < d; ++i) bound = std::max(bound, std::abs(a[i]));
    bound += 1;

    std::vector<Complex> r(d);
    const Num pi = std::acos(-1.0L);
    for (size_t k = 0; k < d; ++k) r[k] = std::polar(bound * 0.5L, 2 * pi * k / d + 0.5L);

    for (int it = 0; it < 2000; ++it) {
        Num maxDelta = 0;
        for (size_t i = 0; i < d; ++i) {
            Complex den = 1;
            for (size_t j = 0; j < d; ++j)
                if (j != i) den *= (r[i] - r[j]);
            if (std::abs(den) == 0) den = 1e-30L;
            Complex delta = horner(a, r[i]) / den;
            r[i] -= delta;
            maxDelta = std::max(maxDelta, std::abs(delta));
        }
        if (maxDelta < 1e-17L * bound) break;
    }

    return r;
}

inline Num derivative(const RealFn& f, Num x) {
    const Num h = 1e-3L;
    return (-f(x + 2 * h) + 8 * f(x + h) - 8 * f(x - h) + f(x - 2 * h)) / (12 * h);
}

struct SimpsonState {
    const RealFn& f;
    long calls = 0;
};

inline Num simpsonRec(SimpsonState& st, Num a, Num b, Num fa, Num fm, Num fb, Num whole, Num eps, int depth) {
    if (++st.calls > 400000) throw CalcError("integral did not converge");
    Num m = (a + b) / 2, lm = (a + m) / 2, rm = (m + b) / 2;
    Num flm = st.f(lm), frm = st.f(rm);
    if (!std::isfinite(flm) || !std::isfinite(frm)) throw CalcError("integrand is not finite on the interval");
    Num left = (m - a) / 6 * (fa + 4 * flm + fm);
    Num right = (b - m) / 6 * (fm + 4 * frm + fb);
    Num delta = left + right - whole;
    if (depth <= 0 || std::fabs(delta) <= 15 * eps) return left + right + delta / 15;
    Num half = std::max(eps / 2, 1e-16L);
    return simpsonRec(st, a, m, fa, flm, fm, left, half, depth - 1) +
           simpsonRec(st, m, b, fm, frm, fb, right, half, depth - 1);
}

inline Num integrate(const RealFn& raw, Num a, Num b) {
    if (a == b) return 0;
    RealFn f = [&raw](Num x) {
        try {
            return raw(x);
        } catch (const CalcError&) {
            return std::numeric_limits<Num>::quiet_NaN();
        }
    };
    Num sign = 1;
    if (a > b) {
        std::swap(a, b);
        sign = -1;
    }
    auto edge = [&](Num x, Num inward) {
        Num v = f(x);
        if (!std::isfinite(v)) v = f(x + inward * 1e-12L * (b - a));
        if (!std::isfinite(v)) throw CalcError("integrand is not finite at an endpoint");
        return v;
    };
    Num fa = edge(a, 1), fb = edge(b, -1);
    Num m = (a + b) / 2;
    Num fm = f(m);
    if (!std::isfinite(fm)) throw CalcError("integrand is not finite on the interval");
    Num whole = (b - a) / 6 * (fa + 4 * fm + fb);
    SimpsonState st{f};
    return sign * simpsonRec(st, a, b, fa, fm, fb, whole, 1e-12L, 40);
}

inline Num findRoot(const RealFn& f, Num x) {
    for (int i = 0; i < 100; ++i) {
        Num fx = f(x);
        if (!std::isfinite(fx)) throw CalcError("function is not finite near the guess");
        if (fx == 0) return x;
        Num d = derivative(f, x);
        if (d == 0 || !std::isfinite(d)) throw CalcError("derivative vanished; try another guess");
        Num next = x - fx / d;
        if (std::fabs(next - x) <= 1e-15L * (1 + std::fabs(next))) {
            x = next;
            break;
        }
        x = next;
    }
    Num fx = f(x);
    if (!std::isfinite(fx) || std::fabs(fx) > 1e-7L * (1 + std::fabs(x))) throw CalcError("no root found near the guess");
    return x;
}

inline Num goldenSection(const RealFn& f, Num a, Num b, bool maximize) {
    const Num phi = (std::sqrt(5.0L) - 1) / 2;
    auto g = [&](Num x) { return maximize ? -f(x) : f(x); };
    Num c = b - phi * (b - a), d = a + phi * (b - a);
    Num gc = g(c), gd = g(d);
    for (int i = 0; i < 200 && std::fabs(b - a) > 1e-13L * (1 + std::fabs(a) + std::fabs(b)); ++i) {
        if (gc < gd) {
            b = d;
            d = c;
            gd = gc;
            c = b - phi * (b - a);
            gc = g(c);
        } else {
            a = c;
            c = d;
            gc = gd;
            d = a + phi * (b - a);
            gd = g(d);
        }
    }
    return (a + b) / 2;
}

inline Num limit(const RealFn& f, Num at) {
    if (std::isinf(at)) {
        Num sign = at > 0 ? 1 : -1;
        Num x1 = 1e8L, x2 = 1e10L;
        Num last = f(sign * x1), far = f(sign * x2);
        if (!std::isfinite(far) || !std::isfinite(last)) throw CalcError("limit does not exist");
        if (std::fabs(far) > 1e9L && std::fabs(last) > 1e7L && (far > 0) == (last > 0))
            return far > 0 ? std::numeric_limits<Num>::infinity() : -std::numeric_limits<Num>::infinity();
        Num extrapolated = (far * x2 - last * x1) / (x2 - x1);
        if (std::fabs(far - last) > 1e-3L * (1 + std::fabs(far)) + 1e-6L * std::fabs(extrapolated))
            throw CalcError("limit does not exist");
        Num value = roundSig(extrapolated, 10);
        return std::fabs(value) < 1e-12L ? 0 : value;
    }

    auto safe = [&](Num x, bool& ok) {
        try {
            Num v = f(x);
            ok = std::isfinite(v);
            return v;
        } catch (const CalcError&) {
            ok = false;
            return Num(0);
        }
    };

    bool okL, okR;
    Num l = safe(at - 1e-4L, okL), r = safe(at + 1e-4L, okR);
    if (okL && okR) {
        Num l2 = safe(at - 1e-5L, okL), r2 = safe(at + 1e-5L, okR);
        Num l2_ = l2, r2_ = r2;
        if (okL && okR) {
            Num scale = 1 + std::fabs(l2) + std::fabs(r2);
            if (std::fabs(l2 - r2) > 1e-3L * scale) {
                if (std::fabs(l2) > 1e3L && std::fabs(r2) > 1e3L && (l2 > 0) == (r2 > 0) && std::fabs(l2) > std::fabs(l) &&
                    std::fabs(r2) > std::fabs(r))
                    return l2 > 0 ? std::numeric_limits<Num>::infinity() : -std::numeric_limits<Num>::infinity();
                throw CalcError("left and right limits differ");
            }
        }
        if (std::fabs(l2_) > 1e6L && std::fabs(r2_) > 1e6L && (l2_ > 0) == (r2_ > 0))
            return l2_ > 0 ? std::numeric_limits<Num>::infinity() : -std::numeric_limits<Num>::infinity();
        Num table[6][6];
        Num h0 = 0.1L;
        for (int k = 0; k < 6; ++k) {
            Num h = h0 / std::pow(2.0L, k);
            bool a, b;
            Num fl = safe(at - h, a), fr = safe(at + h, b);
            if (!a || !b) throw CalcError("limit does not exist");
            table[k][0] = (fl + fr) / 2;
            for (int j = 1; j <= k; ++j) {
                Num p = std::pow(4.0L, j);
                table[k][j] = (p * table[k][j - 1] - table[k - 1][j - 1]) / (p - 1);
            }
        }
        return roundSig(table[5][5], 10);
    }
    if (okL || okR) {
        bool ok;
        Num v = safe(at + (okR ? 1 : -1) * 1e-12L, ok);
        if (ok) return std::fabs(v) < 1e-5L ? 0 : roundSig(v, 7);
    }
    throw CalcError("limit does not exist");
}

inline long long toInteger(Num v, const char* what) {
    if (!std::isfinite(v) || v != std::floor(v) || std::fabs(v) > 9.0e18L)
        throw CalcError(std::string(what) + " must be an integer");
    return (long long)v;
}

inline Num gcdNum(long long a, long long b) {
    a = std::llabs(a);
    b = std::llabs(b);
    while (b) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return (Num)a;
}

inline Num combinations(long long n, long long r) {
    if (n < 0 || r < 0) throw CalcError("nCr requires non-negative integers");
    if (r > n) return 0;
    r = std::min(r, n - r);
    Num result = 1;
    for (long long i = 1; i <= r; ++i) result = result * (Num)(n - r + i) / (Num)i;
    return std::round(result);
}

inline Num permutations(long long n, long long r) {
    if (n < 0 || r < 0) throw CalcError("nPr requires non-negative integers");
    if (r > n) return 0;
    Num result = 1;
    for (long long i = 0; i < r; ++i) result *= (Num)(n - i);
    return result;
}

using u64 = std::uint64_t;
using u128 = unsigned __int128;

inline u64 mulMod(u64 a, u64 b, u64 m) { return (u64)((u128)a * b % m); }

inline u64 powMod(u64 b, u64 e, u64 m) {
    u64 r = 1;
    b %= m;
    while (e) {
        if (e & 1) r = mulMod(r, b, m);
        b = mulMod(b, b, m);
        e >>= 1;
    }
    return r;
}

inline bool isPrime(u64 n) {
    if (n < 2) return false;
    for (u64 p : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL, 29ULL, 31ULL, 37ULL}) {
        if (n == p) return true;
        if (n % p == 0) return false;
    }
    u64 d = n - 1;
    int s = 0;
    while ((d & 1) == 0) {
        d >>= 1;
        ++s;
    }
    for (u64 a : {2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL, 29ULL, 31ULL, 37ULL}) {
        u64 x = powMod(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int i = 1; i < s; ++i) {
            x = mulMod(x, x, n);
            if (x == n - 1) {
                composite = false;
                break;
            }
        }
        if (composite) return false;
    }
    return true;
}

inline u64 pollardRho(u64 n) {
    if (n % 2 == 0) return 2;
    for (u64 c = 1;; ++c) {
        u64 x = 2, y = 2, d = 1;
        auto step = [&](u64 v) { return (mulMod(v, v, n) + c) % n; };
        while (d == 1) {
            x = step(x);
            y = step(step(y));
            u64 diff = x > y ? x - y : y - x;
            u64 a = diff, b = n;
            while (b) {
                u64 t = a % b;
                a = b;
                b = t;
            }
            d = a;
        }
        if (d != n) return d;
    }
}

inline void factorRec(u64 n, std::vector<u64>& out) {
    if (n == 1) return;
    if (isPrime(n)) {
        out.push_back(n);
        return;
    }
    u64 d = pollardRho(n);
    factorRec(d, out);
    factorRec(n / d, out);
}

inline std::vector<std::pair<u64, int>> factorize(u64 n) {
    std::vector<u64> primes;
    for (u64 p = 2; p < 1000 && p * p <= n; ++p)
        while (n % p == 0) {
            primes.push_back(p);
            n /= p;
        }
    factorRec(n, primes);
    std::sort(primes.begin(), primes.end());
    std::vector<std::pair<u64, int>> out;
    for (u64 p : primes) {
        if (!out.empty() && out.back().first == p) ++out.back().second;
        else out.push_back({p, 1});
    }
    return out;
}

inline std::string toBase(long long v, int base) {
    if (base < 2 || base > 36) throw CalcError("base must be between 2 and 36");
    if (v == 0) return "0";
    bool neg = v < 0;
    unsigned long long u = neg ? 0ULL - (unsigned long long)v : (unsigned long long)v;
    std::string s;
    while (u) {
        int d = (int)(u % base);
        s.insert(s.begin(), (char)(d < 10 ? '0' + d : 'A' + d - 10));
        u /= base;
    }
    return neg ? "-" + s : s;
}

}
