#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

#include "value.hpp"

namespace stats {

inline Num sum(const std::vector<Num>& v) {
    Num s = 0;
    for (Num x : v) s += x;
    return s;
}

inline Num mean(const std::vector<Num>& v) { return sum(v) / (Num)v.size(); }

inline std::vector<Num> sorted(std::vector<Num> v) {
    std::sort(v.begin(), v.end());
    return v;
}

inline Num percentileSorted(const std::vector<Num>& s, Num p) {
    if (s.size() == 1) return s[0];
    Num pos = p * (Num)(s.size() - 1);
    size_t lo = (size_t)std::floor(pos);
    size_t hi = std::min(lo + 1, s.size() - 1);
    return s[lo] + (s[hi] - s[lo]) * (pos - (Num)lo);
}

inline Num percentile(const std::vector<Num>& v, Num p) {
    if (p < 0 || p > 1) throw CalcError("percentile must be between 0 and 100");
    return percentileSorted(sorted(v), p);
}

inline Num median(const std::vector<Num>& v) { return percentile(v, 0.5L); }
inline Num q1(const std::vector<Num>& v) { return percentile(v, 0.25L); }
inline Num q3(const std::vector<Num>& v) { return percentile(v, 0.75L); }
inline Num iqr(const std::vector<Num>& v) {
    auto s = sorted(v);
    return percentileSorted(s, 0.75L) - percentileSorted(s, 0.25L);
}

inline Num variance(const std::vector<Num>& v, bool sample) {
    size_t n = v.size();
    if (sample && n < 2) throw CalcError("sample statistics need at least 2 values");
    Num m = mean(v), ss = 0;
    for (Num x : v) ss += (x - m) * (x - m);
    return ss / (Num)(sample ? n - 1 : n);
}

inline Num stddev(const std::vector<Num>& v, bool sample) { return std::sqrt(variance(v, sample)); }

inline Num sem(const std::vector<Num>& v) { return stddev(v, true) / std::sqrt((Num)v.size()); }

inline Num cv(const std::vector<Num>& v) {
    Num m = mean(v);
    if (m == 0) throw CalcError("coefficient of variation is undefined for mean 0");
    return stddev(v, true) / std::fabs(m);
}

inline Num moment(const std::vector<Num>& v, int k) {
    Num m = mean(v), s = 0;
    for (Num x : v) s += std::pow(x - m, (Num)k);
    return s / (Num)v.size();
}

inline Num skewness(const std::vector<Num>& v) {
    if (v.size() < 3) throw CalcError("skewness needs at least 3 values");
    Num m2 = moment(v, 2);
    if (m2 == 0) throw CalcError("skewness is undefined for constant data");
    return moment(v, 3) / std::pow(m2, 1.5L);
}

inline Num kurtosis(const std::vector<Num>& v) {
    if (v.size() < 4) throw CalcError("kurtosis needs at least 4 values");
    Num m2 = moment(v, 2);
    if (m2 == 0) throw CalcError("kurtosis is undefined for constant data");
    return moment(v, 4) / (m2 * m2) - 3;
}

inline Num geometricMean(const std::vector<Num>& v) {
    Num s = 0;
    for (Num x : v) {
        if (x <= 0) throw CalcError("geometric mean requires positive values");
        s += std::log(x);
    }
    return std::exp(s / (Num)v.size());
}

inline Num harmonicMean(const std::vector<Num>& v) {
    Num s = 0;
    for (Num x : v) {
        if (x <= 0) throw CalcError("harmonic mean requires positive values");
        s += 1 / x;
    }
    return (Num)v.size() / s;
}

inline Num rms(const std::vector<Num>& v) {
    Num s = 0;
    for (Num x : v) s += x * x;
    return std::sqrt(s / (Num)v.size());
}

inline Num meanAbsDev(const std::vector<Num>& v) {
    Num m = mean(v), s = 0;
    for (Num x : v) s += std::fabs(x - m);
    return s / (Num)v.size();
}

inline Num medianAbsDev(const std::vector<Num>& v) {
    Num m = median(v);
    std::vector<Num> d;
    for (Num x : v) d.push_back(std::fabs(x - m));
    return median(d);
}

inline Num range(const std::vector<Num>& v) {
    auto mm = std::minmax_element(v.begin(), v.end());
    return *mm.second - *mm.first;
}

inline Num mode(const std::vector<Num>& v) {
    std::map<Num, size_t> counts;
    for (Num x : v) ++counts[x];
    Num best = v[0];
    size_t bestCount = 0;
    for (const auto& [value, count] : counts)
        if (count > bestCount) {
            best = value;
            bestCount = count;
        }
    if (bestCount < 2) throw CalcError("no mode: all values are unique");
    return best;
}

inline Num covariance(const std::vector<Num>& x, const std::vector<Num>& y, bool sample) {
    if (x.size() != y.size() || x.size() < 2) throw CalcError("needs two lists of equal length (at least 2)");
    Num mx = mean(x), my = mean(y), s = 0;
    for (size_t i = 0; i < x.size(); ++i) s += (x[i] - mx) * (y[i] - my);
    return s / (Num)(sample ? x.size() - 1 : x.size());
}

inline Num correlation(const std::vector<Num>& x, const std::vector<Num>& y) {
    Num c = covariance(x, y, true);
    Num sx = stddev(x, true), sy = stddev(y, true);
    if (sx == 0 || sy == 0) throw CalcError("a list has zero variance");
    return c / (sx * sy);
}

inline std::vector<Num> ranks(const std::vector<Num>& v) {
    std::vector<size_t> idx(v.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return v[a] < v[b]; });
    std::vector<Num> r(v.size());
    for (size_t i = 0; i < idx.size();) {
        size_t j = i;
        while (j + 1 < idx.size() && v[idx[j + 1]] == v[idx[i]]) ++j;
        Num avg = (Num)(i + j) / 2 + 1;
        for (size_t k = i; k <= j; ++k) r[idx[k]] = avg;
        i = j + 1;
    }
    return r;
}

inline Num spearman(const std::vector<Num>& x, const std::vector<Num>& y) {
    if (x.size() != y.size() || x.size() < 2) throw CalcError("needs two lists of equal length (at least 2)");
    return correlation(ranks(x), ranks(y));
}

struct LinearFit {
    Num slope = 0, intercept = 0, r2 = 0, r = 0;
};

inline LinearFit linearFit(const std::vector<Num>& x, const std::vector<Num>& y) {
    if (x.size() != y.size() || x.size() < 2) throw CalcError("needs two lists of equal length (at least 2)");
    Num mx = mean(x), my = mean(y), sxx = 0, sxy = 0, syy = 0;
    for (size_t i = 0; i < x.size(); ++i) {
        sxx += (x[i] - mx) * (x[i] - mx);
        sxy += (x[i] - mx) * (y[i] - my);
        syy += (y[i] - my) * (y[i] - my);
    }
    if (sxx == 0) throw CalcError("all x values are equal");
    LinearFit f;
    f.slope = sxy / sxx;
    f.intercept = my - f.slope * mx;
    f.r2 = syy == 0 ? 1 : sxy * sxy / (sxx * syy);
    f.r = syy == 0 ? 0 : sxy / std::sqrt(sxx * syy);
    return f;
}

inline std::vector<Num> zscores(const std::vector<Num>& v) {
    Num m = mean(v), s = stddev(v, true);
    if (s == 0) throw CalcError("z-scores are undefined for constant data");
    std::vector<Num> z;
    for (Num x : v) z.push_back((x - m) / s);
    return z;
}

inline std::vector<Num> cumulativeSum(const std::vector<Num>& v) {
    std::vector<Num> out;
    Num s = 0;
    for (Num x : v) {
        s += x;
        out.push_back(s);
    }
    return out;
}

struct Fences {
    Num low = 0, high = 0;
};

inline Fences fences(const std::vector<Num>& v) {
    auto s = sorted(v);
    Num a = percentileSorted(s, 0.25L), b = percentileSorted(s, 0.75L);
    return {a - 1.5L * (b - a), b + 1.5L * (b - a)};
}

inline std::vector<Num> outliers(const std::vector<Num>& v) {
    Fences f = fences(v);
    std::vector<Num> out;
    for (Num x : v)
        if (x < f.low || x > f.high) out.push_back(x);
    return out;
}

inline Num normalPdf(Num x, Num mu, Num sigma) {
    Num z = (x - mu) / sigma;
    return std::exp(-z * z / 2) / (sigma * std::sqrt(2 * std::acos(-1.0L)));
}

inline Num normalCdf(Num x, Num mu, Num sigma) { return 0.5L * std::erfc(-(x - mu) / (sigma * std::sqrt(2.0L))); }

inline Num normalInv(Num p, Num mu, Num sigma) {
    if (p <= 0 || p >= 1) throw CalcError("p must be strictly between 0 and 1");
    static const Num a[] = {-3.969683028665376e+01L, 2.209460984245205e+02L, -2.759285104469687e+02L,
                            1.383577518672690e+02L,  -3.066479806614716e+01L, 2.506628277459239e+00L};
    static const Num b[] = {-5.447609879822406e+01L, 1.615858368580409e+02L, -1.556989798598866e+02L,
                            6.680131188771972e+01L, -1.328068155288572e+01L};
    static const Num c[] = {-7.784894002430293e-03L, -3.223964580411365e-01L, -2.400758277161838e+00L,
                            -2.549732539343734e+00L, 4.374664141464968e+00L,  2.938163982698783e+00L};
    static const Num d[] = {7.784695709041462e-03L, 3.224671290700398e-01L, 2.445134137142996e+00L, 3.754408661907416e+00L};
    const Num low = 0.02425L;
    Num x;
    if (p < low) {
        Num q = std::sqrt(-2 * std::log(p));
        x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
    } else if (p <= 1 - low) {
        Num q = p - 0.5L, r = q * q;
        x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
    } else {
        Num q = std::sqrt(-2 * std::log(1 - p));
        x = -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
    }
    Num e = 0.5L * std::erfc(-x / std::sqrt(2.0L)) - p;
    Num u = e * std::sqrt(2 * std::acos(-1.0L)) * std::exp(x * x / 2);
    x = x - u / (1 + x * u / 2);
    return mu + sigma * x;
}

inline Num binomialPmf(long long k, long long n, Num p) {
    if (n < 0 || p < 0 || p > 1) throw CalcError("binomial needs n >= 0 and 0 <= p <= 1");
    if (k < 0 || k > n) return 0;
    if (p == 0) return k == 0 ? 1 : 0;
    if (p == 1) return k == n ? 1 : 0;
    return std::exp(std::lgamma((Num)n + 1) - std::lgamma((Num)k + 1) - std::lgamma((Num)(n - k) + 1) + (Num)k * std::log(p) +
                    (Num)(n - k) * std::log1p(-p));
}

inline Num binomialCdf(long long k, long long n, Num p) {
    if (n > 2000000) throw CalcError("n is too large (limit 2000000)");
    if (k < 0) return 0;
    if (k >= n) return 1;
    Num s = 0;
    for (long long i = 0; i <= k; ++i) s += binomialPmf(i, n, p);
    return std::min<Num>(1, s);
}

inline Num poissonPmf(long long k, Num lambda) {
    if (lambda < 0) throw CalcError("lambda must be non-negative");
    if (k < 0) return 0;
    if (lambda == 0) return k == 0 ? 1 : 0;
    return std::exp((Num)k * std::log(lambda) - lambda - std::lgamma((Num)k + 1));
}

inline Num poissonCdf(long long k, Num lambda) {
    if (k < 0) return 0;
    if (k > 5000000) throw CalcError("k is too large (limit 5000000)");
    Num s = 0;
    for (long long i = 0; i <= k; ++i) s += poissonPmf(i, lambda);
    return std::min<Num>(1, s);
}

inline Num betaContinuedFraction(Num a, Num b, Num x) {
    const Num tiny = 1e-300L;
    Num qab = a + b, qap = a + 1, qam = a - 1, c = 1, d = 1 - qab * x / qap;
    if (std::fabs(d) < tiny) d = tiny;
    d = 1 / d;
    Num h = d;
    for (int m = 1; m <= 500; ++m) {
        int m2 = 2 * m;
        Num aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1 + aa * d;
        if (std::fabs(d) < tiny) d = tiny;
        c = 1 + aa / c;
        if (std::fabs(c) < tiny) c = tiny;
        d = 1 / d;
        h *= d * c;
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1 + aa * d;
        if (std::fabs(d) < tiny) d = tiny;
        c = 1 + aa / c;
        if (std::fabs(c) < tiny) c = tiny;
        d = 1 / d;
        Num delta = d * c;
        h *= delta;
        if (std::fabs(delta - 1) < 1e-16L) break;
    }
    return h;
}

inline Num incompleteBeta(Num a, Num b, Num x) {
    if (x <= 0) return 0;
    if (x >= 1) return 1;
    Num front = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) + a * std::log(x) + b * std::log1p(-x));
    if (x < (a + 1) / (a + b + 2)) return front * betaContinuedFraction(a, b, x) / a;
    return 1 - front * betaContinuedFraction(b, a, 1 - x) / b;
}

inline Num tCdf(Num t, Num df) {
    if (df <= 0) throw CalcError("degrees of freedom must be positive");
    Num x = df / (df + t * t);
    Num tail = 0.5L * incompleteBeta(df / 2, 0.5L, x);
    return t > 0 ? 1 - tail : tail;
}

inline Num tInv(Num p, Num df) {
    if (p <= 0 || p >= 1) throw CalcError("p must be strictly between 0 and 1");
    if (df <= 0) throw CalcError("degrees of freedom must be positive");
    Num lo = -1, hi = 1;
    while (tCdf(lo, df) > p && lo > -1e15L) lo *= 2;
    while (tCdf(hi, df) < p && hi < 1e15L) hi *= 2;
    for (int i = 0; i < 300; ++i) {
        Num mid = (lo + hi) / 2;
        if (tCdf(mid, df) < p) lo = mid;
        else hi = mid;
    }
    return (lo + hi) / 2;
}

inline std::pair<Num, Num> confidenceInterval(const std::vector<Num>& v, Num level) {
    if (v.size() < 2) throw CalcError("a confidence interval needs at least 2 values");
    if (level <= 0 || level >= 1) throw CalcError("confidence level must be between 0 and 1 (for example 0.95)");
    Num t = tInv(1 - (1 - level) / 2, (Num)v.size() - 1);
    Num m = mean(v), h = t * sem(v);
    return {m - h, m + h};
}

struct TTest {
    Num t = 0, df = 0, p = 0;
};

inline TTest oneSampleT(const std::vector<Num>& v, Num mu0) {
    if (v.size() < 2) throw CalcError("a t-test needs at least 2 values");
    Num se = sem(v);
    if (se == 0) throw CalcError("t-test is undefined for constant data");
    TTest r;
    r.df = (Num)v.size() - 1;
    r.t = (mean(v) - mu0) / se;
    r.p = std::min<Num>(1, 2 * (1 - tCdf(std::fabs(r.t), r.df)));
    return r;
}

}
