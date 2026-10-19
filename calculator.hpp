#pragma once

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "numeric.hpp"
#include "value.hpp"

struct Solution {
    enum class Kind { Linear, Quadratic, Polynomial, Numeric, Identity, NoSolution, System };
    Kind kind = Kind::NoSolution;
    std::string variable;
    int degree = 0;
    std::vector<Complex> roots;
    std::vector<std::pair<std::string, Num>> values;
};

struct UserFunction {
    std::vector<std::string> params;
    std::string body;
};

struct Outcome {
    Value value;
    std::string defined;
};

class Calculator {
public:
    bool degrees = false;

    Calculator() {
        vars_["pi"] = Value(std::acos(-1.0L));
        vars_["e"] = Value(std::exp(1.0L));
        vars_["i"] = Value(Complex(0, 1));
        vars_["inf"] = Value(std::numeric_limits<Num>::infinity());
        vars_["ans"] = Value(Num(0));
        registerFunctions();
    }

    Outcome execute(const std::string& line) {
        src_ = line;
        pos_ = 0;
        std::string name, body;
        std::vector<std::string> params;
        if (matchFunctionDef(name, params, body)) {
            if (isReservedName(name)) throw CalcError("cannot redefine '" + name + "'");
            if (body.empty()) throw CalcError("function body is empty");
            std::set<std::string> seen;
            for (const auto& p : params) {
                if (isConstant(p)) throw CalcError("'" + p + "' cannot be a parameter");
                if (!seen.insert(p).second) throw CalcError("duplicate parameter '" + p + "'");
            }
            userFuncs_[name] = {params, body};
            std::string sig = name + "(";
            for (size_t i = 0; i < params.size(); ++i) sig += (i ? ", " : "") + params[i];
            return Outcome{Value(), sig + ")"};
        }
        if (matchAssignment(name)) {
            if (isReservedName(name)) throw CalcError("cannot assign to '" + name + "'");
            Value result = expr();
            finish();
            vars_[name] = result;
            vars_["ans"] = result;
            return Outcome{result, ""};
        }
        Value result = expr();
        finish();
        vars_["ans"] = result;
        return Outcome{result, ""};
    }

    Value evaluate(const std::string& text) { return evalText(text); }

    Num evaluateReal(const std::string& text) { return realOf(evalText(text), "expression"); }

    std::vector<Num> sample(const std::string& expr, const std::string& var, Num a, Num b, int n) {
        std::vector<Num> ys(n, std::nan(""));
        for (int i = 0; i < n; ++i) {
            Num x = a + (b - a) * i / (n - 1);
            try {
                Value v = evalText(expr, {{var, Value(x)}});
                if (!v.isMatrix && isReal(v.scalar)) ys[i] = v.scalar.real();
            } catch (const CalcError&) {
            }
        }
        return ys;
    }

    Solution solve(const std::string& input) {
        std::string body = input;
        std::vector<std::string> vars;
        size_t colon = input.find(':');
        if (colon != std::string::npos) {
            for (const auto& v : splitTop(input.substr(0, colon))) {
                std::string name = trim(v);
                if (!isIdent(name)) throw CalcError("invalid unknown name '" + name + "'");
                if (isConstant(name)) throw CalcError("'" + name + "' cannot be the unknown");
                vars.push_back(name);
            }
            body = input.substr(colon + 1);
        }
        std::vector<std::string> eqs;
        for (const auto& e : splitTop(body)) {
            std::string t = trim(e);
            if (t.empty()) throw CalcError("incomplete equation");
            eqs.push_back(t);
        }
        if (eqs.empty()) throw CalcError("incomplete equation");
        if (eqs.size() == 1) return solveSingle(eqs[0], vars);
        return solveSystem(eqs, vars);
    }

    const std::map<std::string, Value>& variables() const { return vars_; }
    const std::map<std::string, UserFunction>& userFunctions() const { return userFuncs_; }

private:
    using Fn = std::function<Value(const std::vector<Value>&)>;
    struct FnInfo {
        int minArgs;
        int maxArgs;
        Fn fn;
    };

    struct DepthGuard {
        int& depth;
        explicit DepthGuard(int& d) : depth(d) {
            if (++depth > 600) {
                --depth;
                throw CalcError("expression nested too deeply");
            }
        }
        ~DepthGuard() { --depth; }
    };

    std::string src_;
    size_t pos_ = 0;
    int depth_ = 0;
    std::map<std::string, Value> vars_;
    std::map<std::string, FnInfo> funcs_;
    std::map<std::string, UserFunction> userFuncs_;

    static const std::set<std::string>& lazyNames() {
        static const std::set<std::string> names = {"diff", "integral", "limit", "sum", "prod", "findroot", "argmin", "argmax"};
        return names;
    }

    static bool isConstant(const std::string& n) { return n == "pi" || n == "e" || n == "i" || n == "inf" || n == "ans"; }

    bool isReservedName(const std::string& n) const {
        return isConstant(n) || funcs_.count(n) || lazyNames().count(n);
    }

    static std::string trim(const std::string& s) {
        size_t a = s.find_first_not_of(" \t\r");
        if (a == std::string::npos) return "";
        return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
    }

    static bool isIdent(const std::string& s) {
        if (s.empty() || !(std::isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
        return std::all_of(s.begin(), s.end(), [](char ch) { return std::isalnum((unsigned char)ch) || ch == '_'; });
    }

    static std::vector<std::string> splitTop(const std::string& s) {
        std::vector<std::string> out;
        int depth = 0;
        size_t start = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '(' || c == '[') ++depth;
            else if (c == ')' || c == ']') --depth;
            else if (c == ',' && depth == 0) {
                out.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        out.push_back(s.substr(start));
        return out;
    }

    static Num realOf(const Value& v, const char* ctx) {
        if (v.isMatrix) throw CalcError(std::string(ctx) + " requires a scalar value");
        if (!isReal(v.scalar)) throw CalcError(std::string(ctx) + " produced a complex value where a real one is required");
        return v.scalar.real();
    }

    void finish() {
        skipWs();
        if (pos_ < src_.size()) throw CalcError(std::string("unexpected character '") + src_[pos_] + "'");
    }

    Value evalText(const std::string& text, const std::map<std::string, Value>& bind = {}) {
        std::string savedSrc = src_;
        size_t savedPos = pos_;
        std::map<std::string, std::optional<Value>> saved;
        for (const auto& [k, v] : bind) {
            auto it = vars_.find(k);
            saved[k] = it == vars_.end() ? std::nullopt : std::optional<Value>(it->second);
            vars_[k] = v;
        }
        auto restore = [&] {
            src_ = savedSrc;
            pos_ = savedPos;
            for (auto& [k, o] : saved) {
                if (o) vars_[k] = *o;
                else vars_.erase(k);
            }
        };
        try {
            src_ = text;
            pos_ = 0;
            Value r = expr();
            finish();
            restore();
            return r;
        } catch (...) {
            restore();
            throw;
        }
    }

    std::set<std::string> unknownIdents(const std::string& text) const {
        std::set<std::string> out;
        size_t i = 0, n = text.size();
        while (i < n) {
            unsigned char ch = text[i];
            if (std::isdigit(ch) || ch == '.') {
                while (i < n && (std::isalnum((unsigned char)text[i]) || text[i] == '.')) ++i;
            } else if (std::isalpha(ch) || ch == '_') {
                size_t j = i;
                while (j < n && (std::isalnum((unsigned char)text[j]) || text[j] == '_')) ++j;
                std::string id = text.substr(i, j - i);
                size_t k = j;
                while (k < n && std::isspace((unsigned char)text[k])) ++k;
                bool isCall = k < n && text[k] == '(';
                if (!isCall && !vars_.count(id)) out.insert(id);
                i = j;
            } else {
                ++i;
            }
        }
        return out;
    }

    std::pair<std::string, std::string> splitEquation(const std::string& eq) const {
        size_t p = eq.find('=');
        if (p == std::string::npos) return {eq, "0"};
        std::string lhs = eq.substr(0, p), rhs = eq.substr(p + 1);
        if (rhs.find('=') != std::string::npos) throw CalcError("equation has more than one '='");
        if (trim(lhs).empty() || trim(rhs).empty()) throw CalcError("incomplete equation");
        return {lhs, rhs};
    }

    Solution solveSingle(const std::string& eq, const std::vector<std::string>& vars) {
        if (vars.size() > 1) throw CalcError("one equation cannot determine several unknowns");
        const auto sidesPair = splitEquation(eq);
        const std::string lhs = sidesPair.first, rhs = sidesPair.second;

        std::string var;
        if (!vars.empty()) {
            var = vars[0];
        } else {
            std::set<std::string> unknown = unknownIdents(lhs);
            for (const auto& u : unknownIdents(rhs)) unknown.insert(u);
            if (unknown.empty()) throw CalcError("no unknown found; use 'solve x: equation' to specify it");
            if (unknown.size() > 1) throw CalcError("several unknowns found; use 'solve x: equation' to pick one");
            var = *unknown.begin();
        }

        auto f = [&](Num x) -> Num {
            Value bound(x);
            return realOf(evalText(lhs, {{var, bound}}), "equation") - realOf(evalText(rhs, {{var, bound}}), "equation");
        };

        Solution s;
        s.variable = var;

        std::vector<Num> coeffs;
        if (fitPolynomial(f, coeffs)) {
            int degree = (int)coeffs.size() - 1;
            s.degree = degree;
            if (degree == 0) {
                s.kind = coeffs[0] == 0 ? Solution::Kind::Identity : Solution::Kind::NoSolution;
                return s;
            }
            if (degree == 1) {
                s.kind = Solution::Kind::Linear;
                s.roots.push_back(numeric::roundSig(Complex(-coeffs[0] / coeffs[1], 0), 13));
                return s;
            }
            if (degree == 2) {
                s.kind = Solution::Kind::Quadratic;
                solveQuadratic(coeffs[2], coeffs[1], coeffs[0], s.roots);
                return s;
            }
            s.kind = Solution::Kind::Polynomial;
            s.roots = polynomialRoots(coeffs);
            return s;
        }

        s.kind = Solution::Kind::Numeric;
        s.roots = scanRoots(f);
        return s;
    }

    static void solveQuadratic(Num a, Num b, Num c, std::vector<Complex>& roots) {
        Num disc = b * b - 4 * a * c;
        if (std::fabs(disc) <= 1e-14L * (b * b + std::fabs(4 * a * c))) {
            roots.push_back(numeric::roundSig(Complex(-b / (2 * a), 0), 13));
        } else if (disc > 0) {
            Num q = -(b + (b >= 0 ? 1 : -1) * std::sqrt(disc)) / 2;
            Num r1 = q / a;
            Num r2 = q != 0 ? c / q : r1;
            if (r1 > r2) std::swap(r1, r2);
            roots.push_back(numeric::roundSig(Complex(r1, 0), 13));
            roots.push_back(numeric::roundSig(Complex(r2, 0), 13));
        } else {
            Num re = -b / (2 * a);
            Num im = std::sqrt(-disc) / (2 * std::fabs(a));
            roots.push_back(numeric::roundSig(Complex(re, -im), 13));
            roots.push_back(numeric::roundSig(Complex(re, im), 13));
        }
    }

    static std::vector<Complex> polynomialRoots(const std::vector<Num>& c) {
        std::vector<Complex> raw = numeric::polyRoots(c);
        size_t d = c.size() - 1;

        std::vector<std::vector<Complex>> deriv(1, std::vector<Complex>(c.begin(), c.end()));
        for (size_t k = 1; k <= d; ++k) {
            const auto& prev = deriv.back();
            std::vector<Complex> next(prev.size() - 1);
            for (size_t i = 1; i < prev.size(); ++i) next[i - 1] = prev[i] * (Num)i;
            deriv.push_back(next);
        }

        auto magnitude = [&](Complex z) {
            Num m = 1;
            for (size_t i = 0; i < c.size(); ++i) m += std::fabs(c[i]) * std::pow(std::abs(z), (Num)i);
            return m;
        };

        std::vector<bool> used(raw.size(), false);
        std::vector<Complex> roots;
        for (size_t i = 0; i < raw.size(); ++i) {
            if (used[i]) continue;
            std::vector<size_t> cluster;
            for (size_t j = i; j < raw.size(); ++j)
                if (!used[j] && std::abs(raw[i] - raw[j]) < 1e-3L * (1 + std::abs(raw[i]))) cluster.push_back(j);

            Complex mean = 0;
            for (size_t j : cluster) mean += raw[j];
            mean /= (Num)cluster.size();

            size_t m = cluster.size();
            Complex z = mean;
            for (int it = 0; it < 50; ++it) {
                Complex dp = numeric::horner(deriv[m], z);
                if (std::abs(dp) == 0) break;
                Complex step = numeric::horner(deriv[m - 1], z) / dp;
                z -= step;
                if (std::abs(step) < 1e-18L * (1 + std::abs(z))) break;
            }
            bool good = std::abs(numeric::horner(deriv[0], z)) <= 1e-8L * magnitude(z);
            if (m > 1 && !good) {
                for (size_t j : cluster) {
                    roots.push_back(raw[j]);
                    used[j] = true;
                }
                continue;
            }
            for (size_t j : cluster) used[j] = true;
            roots.push_back(z);
        }

        for (auto& r : roots) {
            if (std::fabs(r.imag()) <= 1e-9L * (1 + std::fabs(r.real()))) r = Complex(r.real(), 0);
            r = numeric::roundSig(r, 13);
        }
        std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) {
            if (std::fabs(a.real() - b.real()) > 1e-9L) return a.real() < b.real();
            return a.imag() < b.imag();
        });
        return roots;
    }

    template <typename F>
    static bool fitPolynomial(F& f, std::vector<Num>& coeffs) {
        const int maxDeg = 6;
        const int n = maxDeg + 1;
        try {
            std::vector<std::vector<Num>> v(n, std::vector<Num>(n));
            std::vector<Num> y(n);
            for (int k = 0; k < n; ++k) {
                Num x = k - maxDeg / 2;
                Num p = 1;
                for (int j = 0; j < n; ++j) {
                    v[k][j] = p;
                    p *= x;
                }
                y[k] = f(x);
                if (!std::isfinite(y[k])) return false;
            }
            numeric::LinearResult res = numeric::solveLinear(v, y);
            if (res.status != numeric::LinearResult::Status::Unique) return false;
            std::vector<Num> a = res.x;

            auto eval = [&](Num x) {
                Num r = 0, mag = 1;
                for (int j = n - 1; j >= 0; --j) r = r * x + a[j];
                for (int j = 0; j < n; ++j) mag += std::fabs(a[j]) * std::pow(std::fabs(x), (Num)j);
                return std::make_pair(r, mag);
            };
            for (Num x : {0.5L, -4.5L, 9.5L, 5.5L, -7.25L}) {
                auto [expected, mag] = eval(x);
                Num actual = f(x);
                if (!std::isfinite(actual) || std::fabs(actual - expected) > 1e-9L * mag) return false;
            }

            Num biggest = 0;
            for (Num c : a) biggest = std::max(biggest, std::fabs(c));
            Num threshold = 1e-11L * (1 + biggest);
            for (Num& c : a)
                if (std::fabs(c) <= threshold) c = 0;
            while (a.size() > 1 && a.back() == 0) a.pop_back();
            coeffs = a;
            return true;
        } catch (const CalcError&) {
            return false;
        }
    }

    template <typename F>
    static std::vector<Complex> scanRoots(F& f) {
        const Num lo = -100, hi = 100, step = 0.01L;
        const int count = (int)((hi - lo) / step) + 1;
        std::vector<Num> xs(count), ys(count);
        for (int i = 0; i < count; ++i) {
            xs[i] = lo + i * step;
            try {
                ys[i] = f(xs[i]);
            } catch (const CalcError&) {
                ys[i] = std::nan("");
            }
        }
        auto safe = [&](Num x) -> Num {
            try {
                return f(x);
            } catch (const CalcError&) {
                return std::nan("");
            }
        };

        std::vector<Num> found;
        auto add = [&](Num r) {
            for (Num v : found)
                if (std::fabs(v - r) < 1e-6L) return;
            found.push_back(r);
        };

        for (int i = 0; i + 1 < count; ++i) {
            Num y0 = ys[i], y1 = ys[i + 1];
            if (!std::isfinite(y0) || !std::isfinite(y1)) continue;
            if (y0 == 0) {
                add(xs[i]);
                continue;
            }
            if (y0 * y1 < 0) {
                Num l = xs[i], h = xs[i + 1], yl = y0;
                for (int it = 0; it < 200; ++it) {
                    Num m = (l + h) / 2;
                    Num ym = safe(m);
                    if (!std::isfinite(ym)) break;
                    if (yl * ym <= 0) h = m;
                    else {
                        l = m;
                        yl = ym;
                    }
                }
                Num r = (l + h) / 2;
                Num yr = safe(r);
                if (std::isfinite(yr) && std::fabs(yr) < 1e-6L) add(r);
            }
        }
        if (std::isfinite(ys[count - 1]) && ys[count - 1] == 0) add(xs[count - 1]);

        int touched = 0;
        for (int i = 1; i + 1 < count && touched < 200; ++i) {
            Num a = ys[i - 1], b = ys[i], c = ys[i + 1];
            if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c)) continue;
            if (a * b <= 0 || b * c <= 0) continue;
            if (!(std::fabs(b) < std::fabs(a) && std::fabs(b) <= std::fabs(c))) continue;
            ++touched;
            numeric::RealFn g = [&](Num x) {
                Num v = safe(x);
                return std::isfinite(v) ? std::fabs(v) : std::numeric_limits<Num>::infinity();
            };
            Num r = numeric::goldenSection(g, xs[i - 1], xs[i + 1], false);
            Num yr = safe(r);
            if (std::isfinite(yr) && std::fabs(yr) < 1e-10L) add(numeric::roundSig(r, 9));
        }

        std::sort(found.begin(), found.end());
        std::vector<Complex> out;
        for (Num r : found) out.push_back(Complex(r, 0));
        return out;
    }

    Solution solveSystem(const std::vector<std::string>& eqs, std::vector<std::string> vars) {
        std::vector<std::pair<std::string, std::string>> sides;
        for (const auto& e : eqs) sides.push_back(splitEquation(e));

        if (vars.empty()) {
            std::set<std::string> unknown;
            for (const auto& [l, r] : sides) {
                for (const auto& u : unknownIdents(l)) unknown.insert(u);
                for (const auto& u : unknownIdents(r)) unknown.insert(u);
            }
            vars.assign(unknown.begin(), unknown.end());
        }
        size_t n = vars.size();
        if (n == 0) throw CalcError("no unknowns found");
        if (n != sides.size())
            throw CalcError(std::to_string(sides.size()) + " equations for " + std::to_string(n) + " unknowns; a square system is required");

        auto F = [&](const std::vector<Num>& v) {
            std::map<std::string, Value> bind;
            for (size_t i = 0; i < n; ++i) bind[vars[i]] = Value(v[i]);
            std::vector<Num> out;
            for (const auto& [l, r] : sides)
                out.push_back(realOf(evalText(l, bind), "equation") - realOf(evalText(r, bind), "equation"));
            return out;
        };

        std::vector<Num> zero(n, 0);
        std::vector<Num> f0 = F(zero);
        std::vector<std::vector<Num>> a(n, std::vector<Num>(n));
        for (size_t j = 0; j < n; ++j) {
            std::vector<Num> e(n, 0);
            e[j] = 1;
            std::vector<Num> fj = F(e);
            for (size_t i = 0; i < n; ++i) a[i][j] = fj[i] - f0[i];
        }

        std::vector<Num> probe(n);
        for (size_t j = 0; j < n; ++j) probe[j] = 1.3L + 0.37L * (Num)j * (j % 2 ? -1 : 1);
        std::vector<Num> fp = F(probe);
        for (size_t i = 0; i < n; ++i) {
            Num expected = f0[i], mag = 1 + std::fabs(f0[i]);
            for (size_t j = 0; j < n; ++j) {
                expected += a[i][j] * probe[j];
                mag += std::fabs(a[i][j] * probe[j]);
            }
            if (std::fabs(fp[i] - expected) > 1e-9L * mag) throw CalcError("the system is not linear");
        }

        std::vector<Num> rhs(n);
        for (size_t i = 0; i < n; ++i) rhs[i] = -f0[i];
        numeric::LinearResult res = numeric::solveLinear(a, rhs);

        Solution s;
        s.kind = Solution::Kind::System;
        if (res.status == numeric::LinearResult::Status::None) {
            s.kind = Solution::Kind::NoSolution;
        } else if (res.status == numeric::LinearResult::Status::Infinite) {
            s.kind = Solution::Kind::Identity;
        } else {
            for (size_t i = 0; i < n; ++i) s.values.push_back({vars[i], numeric::roundSig(res.x[i], 13)});
        }
        return s;
    }

    Num toRadR(Num x) const { return degrees ? x * std::acos(-1.0L) / 180 : x; }
    Num fromRadR(Num x) const { return degrees ? x * 180 / std::acos(-1.0L) : x; }
    Complex toRadC(Complex z) const { return degrees ? z * (std::acos(-1.0L) / 180) : z; }
    Complex fromRadC(Complex z) const { return degrees ? z * (180 / std::acos(-1.0L)) : z; }

    static Value mapScalars(const Value& v, const std::function<Complex(Complex)>& f) {
        if (!v.isMatrix) return Value(f(v.scalar));
        Matrix m(v.matrix.rows, v.matrix.cols);
        for (size_t i = 0; i < m.data.size(); ++i) m.data[i] = f(v.matrix.data[i]);
        return Value(m);
    }

    static Num scalarReal(const Value& v, const std::string& name) {
        if (v.isMatrix) throw CalcError(name + " expects a scalar argument");
        if (!isReal(v.scalar)) throw CalcError(name + " requires a real argument");
        return v.scalar.real();
    }

    static std::vector<Num> flatten(const std::vector<Value>& args, const std::string& name) {
        std::vector<Num> out;
        for (const auto& v : args) {
            if (v.isMatrix) {
                for (const auto& z : v.matrix.data) {
                    if (!isReal(z)) throw CalcError(name + " requires real values");
                    out.push_back(z.real());
                }
            } else {
                out.push_back(scalarReal(v, name));
            }
        }
        if (out.empty()) throw CalcError(name + " needs at least one value");
        return out;
    }

    static std::vector<Num> vectorOf(const Value& v, const std::string& name) {
        return flatten({v}, name);
    }

    void addFn(const std::string& n, int minA, int maxA, Fn f) { funcs_[n] = {minA, maxA, std::move(f)}; }

    void addC(const std::string& n, std::function<Complex(Complex)> f) {
        addFn(n, 1, 1, [f](const std::vector<Value>& a) { return mapScalars(a[0], f); });
    }

    void addCR(const std::string& n, std::function<Complex(Complex)> cf, std::function<Num(Num)> rf,
               std::function<bool(Num)> domain) {
        addC(n, [=](Complex z) -> Complex {
            if (isReal(z) && domain(z.real())) return Complex(rf(z.real()), 0);
            return cf(z);
        });
    }

    void addR(const std::string& n, std::function<Num(Num)> f) {
        addC(n, [n, f](Complex z) -> Complex {
            if (!isReal(z)) throw CalcError(n + " requires a real argument");
            return Complex(f(z.real()), 0);
        });
    }

    void addR2(const std::string& n, std::function<Num(Num, Num)> f) {
        addFn(n, 2, 2, [n, f](const std::vector<Value>& a) {
            return Value(f(scalarReal(a[0], n), scalarReal(a[1], n)));
        });
    }

    static Num factorialNum(Num x) {
        if (x < 0 && x == std::floor(x)) throw CalcError("factorial is undefined for negative integers");
        Num r;
        if (x == std::floor(x)) {
            if (x > 100000) throw CalcError("factorial too large");
            r = 1;
            for (long long i = 2; i <= (long long)x; ++i) r *= (Num)i;
        } else {
            r = std::tgamma(x + 1);
        }
        if (!std::isfinite(r)) throw CalcError("factorial too large");
        return r;
    }

    static Num percentile(std::vector<Num> v, Num p) {
        std::sort(v.begin(), v.end());
        Num pos = p * (v.size() - 1);
        size_t lo = (size_t)std::floor(pos);
        size_t hi = std::min(lo + 1, v.size() - 1);
        return v[lo] + (v[hi] - v[lo]) * (pos - lo);
    }

    static Num variance(const std::vector<Num>& v, bool sample) {
        size_t n = v.size();
        if (sample && n < 2) throw CalcError("sample statistics need at least 2 values");
        Num mean = 0;
        for (Num x : v) mean += x;
        mean /= n;
        Num ss = 0;
        for (Num x : v) ss += (x - mean) * (x - mean);
        return ss / (sample ? n - 1 : n);
    }

    static Num sumOf(const std::vector<Num>& v) {
        Num s = 0;
        for (Num x : v) s += x;
        return s;
    }

    static size_t sizeArg(const Value& v, const std::string& name) {
        Num n = scalarReal(v, name);
        if (n < 0 || n != std::floor(n) || n > 1000) throw CalcError(name + " expects an integer between 0 and 1000");
        return (size_t)n;
    }

    static const Matrix& matrixArg(const Value& v, const std::string& name) {
        if (!v.isMatrix) throw CalcError(name + " expects a matrix");
        return v.matrix;
    }

    static Matrix filled(const std::vector<Value>& a, const std::string& name, Complex fill) {
        size_t r = sizeArg(a[0], name);
        size_t c = a.size() > 1 ? sizeArg(a[1], name) : r;
        Matrix m(r, c);
        for (auto& z : m.data) z = fill;
        return m;
    }

    void registerFunctions() {
        const Num ln10 = std::log(10.0L);
        auto always = [](Num) { return true; };

        addCR("sin", [this](Complex z) { return std::sin(toRadC(z)); }, [this](Num x) { return std::sin(toRadR(x)); }, always);
        addCR("cos", [this](Complex z) { return std::cos(toRadC(z)); }, [this](Num x) { return std::cos(toRadR(x)); }, always);
        addCR("tan", [this](Complex z) { return std::tan(toRadC(z)); }, [this](Num x) { return std::tan(toRadR(x)); }, always);
        addCR("asin", [this](Complex z) { return fromRadC(std::asin(z)); }, [this](Num x) { return fromRadR(std::asin(x)); },
              [](Num x) { return std::fabs(x) <= 1; });
        addCR("acos", [this](Complex z) { return fromRadC(std::acos(z)); }, [this](Num x) { return fromRadR(std::acos(x)); },
              [](Num x) { return std::fabs(x) <= 1; });
        addCR("atan", [this](Complex z) { return fromRadC(std::atan(z)); }, [this](Num x) { return fromRadR(std::atan(x)); }, always);
        addCR("sinh", [](Complex z) { return std::sinh(z); }, [](Num x) { return std::sinh(x); }, always);
        addCR("cosh", [](Complex z) { return std::cosh(z); }, [](Num x) { return std::cosh(x); }, always);
        addCR("tanh", [](Complex z) { return std::tanh(z); }, [](Num x) { return std::tanh(x); }, always);
        addCR("asinh", [](Complex z) { return std::asinh(z); }, [](Num x) { return std::asinh(x); }, always);
        addCR("acosh", [](Complex z) { return std::acosh(z); }, [](Num x) { return std::acosh(x); }, [](Num x) { return x >= 1; });
        addCR("atanh", [](Complex z) { return std::atanh(z); }, [](Num x) { return std::atanh(x); },
              [](Num x) { return std::fabs(x) < 1; });
        addCR("sqrt", [](Complex z) { return std::sqrt(z); }, [](Num x) { return std::sqrt(x); }, [](Num x) { return x >= 0; });
        addCR("cbrt", [](Complex z) { return std::pow(z, Num(1) / 3); }, [](Num x) { return std::cbrt(x); }, always);
        addCR("exp", [](Complex z) { return std::exp(z); }, [](Num x) { return std::exp(x); }, always);

        auto logC = [](const std::string& n, Num divisor) {
            return [n, divisor](Complex z) -> Complex {
                if (z == Complex(0)) throw CalcError(n + " is undefined at 0");
                return std::log(z) / divisor;
            };
        };
        addCR("ln", logC("ln", 1), [](Num x) { return std::log(x); }, [](Num x) { return x > 0; });
        addCR("log", logC("log", ln10), [](Num x) { return std::log10(x); }, [](Num x) { return x > 0; });
        addCR("log2", logC("log2", std::log(2.0L)), [](Num x) { return std::log2(x); }, [](Num x) { return x > 0; });

        addC("conj", [](Complex z) { return std::conj(z); });
        addC("re", [](Complex z) { return Complex(z.real(), 0); });
        addC("im", [](Complex z) { return Complex(z.imag(), 0); });
        addC("abs", [](Complex z) { return Complex(std::abs(z), 0); });
        addC("arg", [this](Complex z) { return Complex(fromRadR(std::arg(z)), 0); });

        addR("floor", [](Num x) { return std::floor(x); });
        addR("ceil", [](Num x) { return std::ceil(x); });
        addR("round", [](Num x) { return std::round(x); });
        addR("trunc", [](Num x) { return std::trunc(x); });
        addR("sign", [](Num x) { return x > 0 ? 1.0L : (x < 0 ? -1.0L : 0.0L); });
        addR("fact", [](Num x) { return factorialNum(x); });
        addR("gamma", [](Num x) {
            Num g = std::tgamma(x);
            if (!std::isfinite(g)) throw CalcError("gamma is undefined or too large here");
            return g;
        });
        addR("lgamma", [](Num x) { return std::lgamma(x); });
        addR("erf", [](Num x) { return std::erf(x); });
        addR("erfc", [](Num x) { return std::erfc(x); });
        addR("isprime", [](Num x) { return numeric::isPrime((numeric::u64)std::max<Num>(0, numeric::toInteger(x, "isprime argument"))) ? 1.0L : 0.0L; });
        addR("fib", [](Num x) {
            long long n = numeric::toInteger(x, "fib argument");
            if (n < 0 || n > 1000) throw CalcError("fib expects an integer between 0 and 1000");
            Num a = 0, b = 1;
            for (long long k = 0; k < n; ++k) {
                Num t = a + b;
                a = b;
                b = t;
            }
            return a;
        });

        addR2("atan2", [this](Num y, Num x) { return fromRadR(std::atan2(y, x)); });
        addR2("hypot", [](Num a, Num b) { return std::hypot(a, b); });
        addR2("beta", [](Num a, Num b) { return std::exp(std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b)); });
        addR2("logb", [](Num x, Num b) {
            if (x <= 0 || b <= 0 || b == 1) throw CalcError("invalid arguments for logb(x, base)");
            return std::log(x) / std::log(b);
        });
        addR2("root", [](Num x, Num n) {
            if (n == 0) throw CalcError("root index cannot be 0");
            if (x < 0 && n == std::floor(n) && std::fmod(n, 2) != 0) return -std::pow(-x, 1 / n);
            if (x < 0) throw CalcError("even root of a negative number (use x^(1/n) for complex results)");
            return std::pow(x, 1 / n);
        });
        addR2("mod", [](Num a, Num b) {
            if (b == 0) throw CalcError("modulo by zero");
            return std::fmod(a, b);
        });
        addR2("gcd", [](Num a, Num b) { return numeric::gcdNum(numeric::toInteger(a, "gcd argument"), numeric::toInteger(b, "gcd argument")); });
        addR2("lcm", [](Num a, Num b) {
            long long x = numeric::toInteger(a, "lcm argument"), y = numeric::toInteger(b, "lcm argument");
            if (x == 0 || y == 0) return 0.0L;
            return std::fabs((Num)x / numeric::gcdNum(x, y) * (Num)y);
        });
        addR2("ncr", [](Num n, Num r) { return numeric::combinations(numeric::toInteger(n, "nCr n"), numeric::toInteger(r, "nCr r")); });
        addR2("nCr", [](Num n, Num r) { return numeric::combinations(numeric::toInteger(n, "nCr n"), numeric::toInteger(r, "nCr r")); });
        addR2("nPr", [](Num n, Num r) { return numeric::permutations(numeric::toInteger(n, "nPr n"), numeric::toInteger(r, "nPr r")); });
        addR2("npr", [](Num n, Num r) { return numeric::permutations(numeric::toInteger(n, "nPr n"), numeric::toInteger(r, "nPr r")); });

        addFn("pow", 2, 2, [](const std::vector<Value>& a) { return ops::pow(a[0], a[1]); });

        addFn("min", 1, -1, [](const std::vector<Value>& a) {
            auto v = flatten(a, "min");
            return Value(*std::min_element(v.begin(), v.end()));
        });
        addFn("max", 1, -1, [](const std::vector<Value>& a) {
            auto v = flatten(a, "max");
            return Value(*std::max_element(v.begin(), v.end()));
        });
        addFn("count", 1, -1, [](const std::vector<Value>& a) { return Value((Num)flatten(a, "count").size()); });
        addFn("mean", 1, -1, [](const std::vector<Value>& a) {
            auto v = flatten(a, "mean");
            return Value(sumOf(v) / v.size());
        });
        addFn("median", 1, -1, [](const std::vector<Value>& a) { return Value(percentile(flatten(a, "median"), 0.5L)); });
        addFn("var", 1, -1, [](const std::vector<Value>& a) { return Value(variance(flatten(a, "var"), true)); });
        addFn("varp", 1, -1, [](const std::vector<Value>& a) { return Value(variance(flatten(a, "varp"), false)); });
        addFn("std", 1, -1, [](const std::vector<Value>& a) { return Value(std::sqrt(variance(flatten(a, "std"), true))); });
        addFn("stdp", 1, -1, [](const std::vector<Value>& a) { return Value(std::sqrt(variance(flatten(a, "stdp"), false))); });

        addFn("linreg", 2, 2, [](const std::vector<Value>& a) {
            auto x = vectorOf(a[0], "linreg"), y = vectorOf(a[1], "linreg");
            if (x.size() != y.size() || x.size() < 2) throw CalcError("linreg needs two lists of equal length (at least 2)");
            Num n = (Num)x.size(), mx = sumOf(x) / n, my = sumOf(y) / n;
            Num sxx = 0, sxy = 0, syy = 0;
            for (size_t i = 0; i < x.size(); ++i) {
                sxx += (x[i] - mx) * (x[i] - mx);
                sxy += (x[i] - mx) * (y[i] - my);
                syy += (y[i] - my) * (y[i] - my);
            }
            if (sxx == 0) throw CalcError("linreg: all x values are equal");
            Num slope = sxy / sxx;
            Matrix m(1, 3);
            m.at(0, 0) = slope;
            m.at(0, 1) = my - slope * mx;
            m.at(0, 2) = syy == 0 ? 1 : sxy * sxy / (sxx * syy);
            return Value(m);
        });
        addFn("corr", 2, 2, [](const std::vector<Value>& a) {
            auto x = vectorOf(a[0], "corr"), y = vectorOf(a[1], "corr");
            if (x.size() != y.size() || x.size() < 2) throw CalcError("corr needs two lists of equal length (at least 2)");
            Num n = (Num)x.size(), mx = sumOf(x) / n, my = sumOf(y) / n;
            Num sxx = 0, sxy = 0, syy = 0;
            for (size_t i = 0; i < x.size(); ++i) {
                sxx += (x[i] - mx) * (x[i] - mx);
                sxy += (x[i] - mx) * (y[i] - my);
                syy += (y[i] - my) * (y[i] - my);
            }
            if (sxx == 0 || syy == 0) throw CalcError("corr: a list has zero variance");
            return Value(sxy / std::sqrt(sxx * syy));
        });

        addFn("normpdf", 1, 3, [](const std::vector<Value>& a) {
            Num x = scalarReal(a[0], "normpdf"), mu = a.size() > 1 ? scalarReal(a[1], "normpdf") : 0;
            Num sigma = a.size() > 2 ? scalarReal(a[2], "normpdf") : 1;
            if (sigma <= 0) throw CalcError("sigma must be positive");
            Num z = (x - mu) / sigma;
            return Value(std::exp(-z * z / 2) / (sigma * std::sqrt(2 * std::acos(-1.0L))));
        });
        addFn("normcdf", 1, 3, [](const std::vector<Value>& a) {
            Num x = scalarReal(a[0], "normcdf"), mu = a.size() > 1 ? scalarReal(a[1], "normcdf") : 0;
            Num sigma = a.size() > 2 ? scalarReal(a[2], "normcdf") : 1;
            if (sigma <= 0) throw CalcError("sigma must be positive");
            return Value(0.5L * std::erfc(-(x - mu) / (sigma * std::sqrt(2.0L))));
        });

        addFn("det", 1, 1, [](const std::vector<Value>& a) { return Value(mat::determinant(matrixArg(a[0], "det"))); });
        addFn("inv", 1, 1, [](const std::vector<Value>& a) {
            if (!a[0].isMatrix) {
                if (a[0].scalar == Complex(0)) throw CalcError("division by zero");
                return Value(Complex(1) / a[0].scalar);
            }
            return Value(mat::inverse(a[0].matrix));
        });
        addFn("transpose", 1, 1, [](const std::vector<Value>& a) { return Value(mat::transpose(matrixArg(a[0], "transpose"))); });
        addFn("trace", 1, 1, [](const std::vector<Value>& a) {
            const Matrix& m = matrixArg(a[0], "trace");
            if (m.rows != m.cols) throw CalcError("trace requires a square matrix");
            Complex t = 0;
            for (size_t i = 0; i < m.rows; ++i) t += m.at(i, i);
            return Value(t);
        });
        addFn("rank", 1, 1, [](const std::vector<Value>& a) { return Value((Num)mat::rank(matrixArg(a[0], "rank"))); });
        addFn("rows", 1, 1, [](const std::vector<Value>& a) { return Value((Num)(a[0].isMatrix ? a[0].matrix.rows : 1)); });
        addFn("cols", 1, 1, [](const std::vector<Value>& a) { return Value((Num)(a[0].isMatrix ? a[0].matrix.cols : 1)); });
        addFn("eye", 1, 1, [](const std::vector<Value>& a) { return Value(mat::identity(sizeArg(a[0], "eye"))); });
        addFn("zeros", 1, 2, [](const std::vector<Value>& a) { return Value(filled(a, "zeros", 0)); });
        addFn("ones", 1, 2, [](const std::vector<Value>& a) { return Value(filled(a, "ones", 1)); });
        addFn("norm", 1, 1, [](const std::vector<Value>& a) {
            Num s = 0;
            if (a[0].isMatrix) {
                for (const auto& z : a[0].matrix.data) s += std::norm(z);
            } else {
                s = std::norm(a[0].scalar);
            }
            return Value(std::sqrt(s));
        });
        addFn("dot", 2, 2, [](const std::vector<Value>& a) {
            if (!a[0].isMatrix || !a[1].isMatrix || a[0].matrix.data.size() != a[1].matrix.data.size())
                throw CalcError("dot expects two vectors of equal length");
            Complex s = 0;
            for (size_t i = 0; i < a[0].matrix.data.size(); ++i) s += a[0].matrix.data[i] * a[1].matrix.data[i];
            return Value(s);
        });
        addFn("cross", 2, 2, [](const std::vector<Value>& a) {
            if (!a[0].isMatrix || !a[1].isMatrix || a[0].matrix.data.size() != 3 || a[1].matrix.data.size() != 3)
                throw CalcError("cross expects two 3-element vectors");
            const auto& u = a[0].matrix.data;
            const auto& v = a[1].matrix.data;
            Matrix m(1, 3);
            m.at(0, 0) = u[1] * v[2] - u[2] * v[1];
            m.at(0, 1) = u[2] * v[0] - u[0] * v[2];
            m.at(0, 2) = u[0] * v[1] - u[1] * v[0];
            return Value(m);
        });
    }

    void skipWs() {
        while (pos_ < src_.size() && std::isspace((unsigned char)src_[pos_])) ++pos_;
    }

    bool eat(char c) {
        skipWs();
        if (pos_ < src_.size() && src_[pos_] == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool peekIs(char c) {
        skipWs();
        return pos_ < src_.size() && src_[pos_] == c;
    }

    std::string readIdent() {
        size_t s = pos_;
        while (pos_ < src_.size() && (std::isalnum((unsigned char)src_[pos_]) || src_[pos_] == '_')) ++pos_;
        return src_.substr(s, pos_ - s);
    }

    bool matchAssignment(std::string& name) {
        size_t save = pos_;
        skipWs();
        if (pos_ < src_.size() && (std::isalpha((unsigned char)src_[pos_]) || src_[pos_] == '_')) {
            std::string id = readIdent();
            if (eat('=')) {
                name = id;
                return true;
            }
        }
        pos_ = save;
        return false;
    }

    bool matchFunctionDef(std::string& name, std::vector<std::string>& params, std::string& body) {
        size_t save = pos_;
        skipWs();
        if (pos_ < src_.size() && (std::isalpha((unsigned char)src_[pos_]) || src_[pos_] == '_')) {
            std::string id = readIdent();
            if (eat('(')) {
                std::vector<std::string> ps;
                bool ok = true;
                if (!eat(')')) {
                    for (;;) {
                        skipWs();
                        std::string p = readIdent();
                        if (!isIdent(p)) {
                            ok = false;
                            break;
                        }
                        ps.push_back(p);
                        if (eat(',')) continue;
                        if (eat(')')) break;
                        ok = false;
                        break;
                    }
                }
                if (ok && ps.empty()) ok = false;
                if (ok && eat('=')) {
                    name = id;
                    params = ps;
                    body = trim(src_.substr(pos_));
                    pos_ = src_.size();
                    return true;
                }
            }
        }
        pos_ = save;
        return false;
    }

    bool startsImplicitFactor() {
        skipWs();
        if (pos_ >= src_.size()) return false;
        unsigned char c = src_[pos_];
        return std::isdigit(c) || c == '.' || c == '(' || c == '[' || std::isalpha(c) || c == '_';
    }

    Value expr() {
        Value v = term();
        for (;;) {
            if (eat('+')) v = ops::add(v, term());
            else if (eat('-')) v = ops::sub(v, term());
            else return v;
        }
    }

    Value term() {
        Value v = unary();
        for (;;) {
            if (eat('*')) v = ops::mul(v, unary());
            else if (eat('/')) v = ops::div(v, unary());
            else if (eat('%')) v = modulo(v, unary());
            else if (startsImplicitFactor()) v = ops::mul(v, unary());
            else return v;
        }
    }

    static Value modulo(const Value& a, const Value& b) {
        Num x = scalarReal(a, "%"), y = scalarReal(b, "%");
        if (y == 0) throw CalcError("modulo by zero");
        return Value(std::fmod(x, y));
    }

    Value unary() {
        DepthGuard guard(depth_);
        if (eat('-')) return ops::neg(unary());
        if (eat('+')) return unary();
        return power();
    }

    Value power() {
        Value base = postfix();
        if (eat('^')) return ops::pow(base, unary());
        return base;
    }

    Value postfix() {
        Value v = primary();
        while (peekIs('!') && !(pos_ + 1 < src_.size() && src_[pos_ + 1] == '=')) {
            ++pos_;
            v = Value(factorialNum(scalarReal(v, "factorial")));
        }
        return v;
    }

    Value primary() {
        skipWs();
        if (pos_ >= src_.size()) throw CalcError("incomplete expression");
        char c = src_[pos_];

        if (std::isdigit((unsigned char)c) || c == '.') return number();

        if (c == '(') {
            ++pos_;
            Value v = expr();
            if (!eat(')')) throw CalcError("missing ')'");
            return v;
        }

        if (c == '[') {
            ++pos_;
            return matrixLiteral();
        }

        if (std::isalpha((unsigned char)c) || c == '_') {
            std::string id = readIdent();
            bool paren = peekIs('(');
            if (paren && lazyNames().count(id)) return lazyCall(id);
            if (paren && (funcs_.count(id) || userFuncs_.count(id))) {
                ++pos_;
                return call(id);
            }
            auto it = vars_.find(id);
            if (it != vars_.end()) return it->second;
            if (funcs_.count(id) || userFuncs_.count(id) || lazyNames().count(id))
                throw CalcError("'" + id + "' is a function; use " + id + "(...)");
            if (paren) throw CalcError("unknown function '" + id + "'");
            throw CalcError("unknown variable '" + id + "'");
        }
        throw CalcError(std::string("unexpected character '") + c + "'");
    }

    Value matrixLiteral() {
        std::vector<std::vector<Complex>> rows(1);
        if (peekIs(']')) throw CalcError("empty matrix");
        for (;;) {
            Value v = expr();
            if (v.isMatrix) throw CalcError("nested matrices are not supported");
            rows.back().push_back(v.scalar);
            if (eat(',')) continue;
            if (eat(';')) {
                rows.emplace_back();
                continue;
            }
            if (eat(']')) break;
            throw CalcError("missing ']'");
        }
        size_t cols = rows[0].size();
        for (const auto& r : rows)
            if (r.size() != cols || r.empty()) throw CalcError("matrix rows must have the same length");
        Matrix m(rows.size(), cols);
        for (size_t i = 0; i < rows.size(); ++i)
            for (size_t j = 0; j < cols; ++j) m.at(i, j) = rows[i][j];
        return Value(m);
    }

    Value number() {
        size_t s = pos_;
        if (src_[pos_] == '0' && pos_ + 1 < src_.size()) {
            char k = (char)std::tolower((unsigned char)src_[pos_ + 1]);
            int base = k == 'x' ? 16 : k == 'b' ? 2 : k == 'o' ? 8 : 0;
            if (base) {
                size_t q = pos_ + 2;
                while (q < src_.size() && std::isalnum((unsigned char)src_[q])) ++q;
                if (q > pos_ + 2) {
                    std::string digits = src_.substr(pos_ + 2, q - pos_ - 2);
                    char* end = nullptr;
                    errno = 0;
                    unsigned long long v = std::strtoull(digits.c_str(), &end, base);
                    if (*end != '\0' || errno != 0) throw CalcError("invalid number '" + src_.substr(s, q - s) + "'");
                    pos_ = q;
                    return Value((Num)v);
                }
            }
        }
        while (pos_ < src_.size() && (std::isdigit((unsigned char)src_[pos_]) || src_[pos_] == '.')) ++pos_;
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            size_t save = pos_++;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
            if (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) {
                while (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) ++pos_;
            } else pos_ = save;
        }
        std::string text = src_.substr(s, pos_ - s);
        try {
            size_t used = 0;
            Num v = std::stold(text, &used);
            if (used != text.size()) throw CalcError("invalid number '" + text + "'");
            return Value(v);
        } catch (const CalcError&) {
            throw;
        } catch (...) {
            throw CalcError("invalid number '" + text + "'");
        }
    }

    Value call(const std::string& name) {
        std::vector<Value> args;
        if (!eat(')')) {
            do args.push_back(expr());
            while (eat(','));
            if (!eat(')')) throw CalcError("missing ')' in call to " + name);
        }

        auto user = userFuncs_.find(name);
        if (user != userFuncs_.end()) {
            const UserFunction& f = user->second;
            if (args.size() != f.params.size())
                throw CalcError(name + " expects " + std::to_string(f.params.size()) + " argument(s)");
            std::map<std::string, Value> bind;
            for (size_t i = 0; i < args.size(); ++i) bind[f.params[i]] = args[i];
            UserFunction copy = f;
            return evalText(copy.body, bind);
        }

        const FnInfo& f = funcs_.at(name);
        int n = (int)args.size();
        if (n < f.minArgs || (f.maxArgs >= 0 && n > f.maxArgs)) {
            std::string expected = f.maxArgs < 0   ? "at least " + std::to_string(f.minArgs)
                                   : f.minArgs == f.maxArgs ? std::to_string(f.minArgs)
                                                            : std::to_string(f.minArgs) + " to " + std::to_string(f.maxArgs);
            throw CalcError(name + " expects " + expected + " argument(s)");
        }
        return f.fn(args);
    }

    std::vector<std::string> rawArgs() {
        ++pos_;
        int depth = 1;
        size_t start = pos_;
        std::vector<std::string> args;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (c == '(' || c == '[') ++depth;
            else if (c == ')' || c == ']') {
                if (--depth == 0) {
                    args.push_back(src_.substr(start, pos_ - start));
                    ++pos_;
                    if (args.size() == 1 && trim(args[0]).empty()) args.clear();
                    return args;
                }
            } else if (c == ',' && depth == 1) {
                args.push_back(src_.substr(start, pos_ - start));
                start = pos_ + 1;
            }
            ++pos_;
        }
        throw CalcError("missing ')'");
    }

    Value lazyCall(const std::string& name) {
        std::vector<std::string> a = rawArgs();
        size_t n = a.size();

        auto bad = [&](const char* usage) { return CalcError(std::string("usage: ") + usage); };
        auto real = [&](const std::string& s) { return realOf(evalText(s), name.c_str()); };
        auto varName = [&](const std::string& s) {
            std::string v = trim(s);
            if (!isIdent(v) || isConstant(v)) throw CalcError("invalid variable name '" + v + "'");
            return v;
        };
        auto fnOf = [this](const std::string& expression, const std::string& var) {
            return numeric::RealFn([this, expression, var](Num x) {
                return realOf(evalText(expression, {{var, Value(x)}}), "expression");
            });
        };

        if (name == "diff") {
            if (n != 2 && n != 3) throw bad("diff(expr, at)  or  diff(expr, var, at)");
            std::string var = n == 3 ? varName(a[1]) : "x";
            Num at = real(a[n - 1]);
            return Value(numeric::roundSig(numeric::derivative(fnOf(a[0], var), at), 11));
        }
        if (name == "integral") {
            if (n != 3 && n != 4) throw bad("integral(expr, a, b)  or  integral(expr, var, a, b)");
            std::string var = n == 4 ? varName(a[1]) : "x";
            Num lo = real(a[n - 2]), hi = real(a[n - 1]);
            if (std::isinf(lo) || std::isinf(hi)) throw CalcError("infinite bounds are not supported");
            return Value(numeric::roundSig(numeric::integrate(fnOf(a[0], var), lo, hi), 11));
        }
        if (name == "limit") {
            if (n != 2 && n != 3) throw bad("limit(expr, at)  or  limit(expr, var, at)");
            std::string var = n == 3 ? varName(a[1]) : "x";
            return Value(numeric::limit(fnOf(a[0], var), real(a[n - 1])));
        }
        if (name == "findroot") {
            if (n != 2 && n != 3) throw bad("findroot(expr, guess)  or  findroot(expr, var, guess)");
            std::string var = n == 3 ? varName(a[1]) : "x";
            return Value(numeric::roundSig(numeric::findRoot(fnOf(a[0], var), real(a[n - 1])), 13));
        }
        if (name == "argmin" || name == "argmax") {
            if (n != 3 && n != 4) throw bad((name + "(expr, a, b)  or  " + name + "(expr, var, a, b)").c_str());
            std::string var = n == 4 ? varName(a[1]) : "x";
            Num lo = real(a[n - 2]), hi = real(a[n - 1]);
            if (!(lo < hi)) throw CalcError("the interval must satisfy a < b");
            return Value(numeric::roundSig(numeric::goldenSection(fnOf(a[0], var), lo, hi, name == "argmax"), 9));
        }

        bool series = n == 4 && isIdent(trim(a[1])) && !isConstant(trim(a[1]));
        if (!series) {
            std::vector<Value> values;
            for (const auto& s : a) values.push_back(evalText(s));
            std::vector<Num> v = flatten(values, name);
            if (name == "sum") return Value(sumOf(v));
            Num p = 1;
            for (Num x : v) p *= x;
            return Value(p);
        }

        std::string var = trim(a[1]);
        long long lo = numeric::toInteger(real(a[2]), "lower bound");
        long long hi = numeric::toInteger(real(a[3]), "upper bound");
        if (hi - lo > 2000000) throw CalcError("too many terms (limit 2000000)");
        Value acc = name == "sum" ? Value(Num(0)) : Value(Num(1));
        for (long long k = lo; k <= hi; ++k) {
            Value term = evalText(a[0], {{var, Value((Num)k)}});
            acc = name == "sum" ? ops::add(acc, term) : ops::mul(acc, term);
        }
        return acc;
    }
};
