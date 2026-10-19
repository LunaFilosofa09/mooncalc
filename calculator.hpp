#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using Num = long double;
using Complex = std::complex<Num>;

struct CalcError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Solution {
    enum class Kind { Linear, Quadratic, Numeric, Identity, NoSolution };
    Kind kind = Kind::NoSolution;
    std::string variable;
    std::vector<Complex> roots;
};

class Calculator {
public:
    bool degrees = false;

    Calculator() {
        vars_["pi"] = std::acos(-1.0L);
        vars_["e"] = std::exp(1.0L);
        vars_["ans"] = 0;
        registerFunctions();
    }

    Num evaluate(const std::string& line) {
        src_ = line;
        pos_ = 0;
        Num result;
        std::string name;
        if (matchAssignment(name)) {
            if (name == "pi" || name == "e") throw CalcError("cannot reassign constant '" + name + "'");
            result = expr();
            vars_[name] = result;
        } else {
            result = expr();
        }
        skipWs();
        if (pos_ < src_.size()) throw CalcError(std::string("unexpected character '") + src_[pos_] + "'");
        vars_["ans"] = result;
        return result;
    }

    Solution solve(const std::string& input) {
        std::string eq = input;
        std::string var;
        size_t colon = input.find(':');
        if (colon != std::string::npos) {
            var = trim(input.substr(0, colon));
            eq = input.substr(colon + 1);
            if (!isIdent(var)) throw CalcError("invalid unknown name '" + var + "'");
            if (var == "pi" || var == "e" || var == "ans") throw CalcError("'" + var + "' cannot be the unknown");
        }

        std::string lhs, rhs;
        size_t eqPos = eq.find('=');
        if (eqPos == std::string::npos) {
            lhs = eq;
            rhs = "0";
        } else {
            lhs = eq.substr(0, eqPos);
            rhs = eq.substr(eqPos + 1);
            if (rhs.find('=') != std::string::npos) throw CalcError("equation has more than one '='");
        }
        if (trim(lhs).empty() || trim(rhs).empty()) throw CalcError("incomplete equation");

        if (var.empty()) {
            std::set<std::string> unknown = unknownIdents(lhs);
            for (const auto& u : unknownIdents(rhs)) unknown.insert(u);
            if (unknown.empty())
                throw CalcError("no unknown found; use 'solve x: equation' to specify it");
            if (unknown.size() > 1) throw CalcError("several unknowns found; use 'solve x: equation' to pick one");
            var = *unknown.begin();
        }

        auto f = [&](Num x) { return evalExpr(lhs, var, x) - evalExpr(rhs, var, x); };

        Solution s;
        s.variable = var;

        Num a = 0, b = 0, c = 0;
        if (fitQuadratic(f, a, b, c)) {
            Num eps = 1e-12L * (1 + std::fabs(a) + std::fabs(b) + std::fabs(c));
            if (std::fabs(a) <= eps) {
                if (std::fabs(b) <= eps) {
                    s.kind = std::fabs(c) <= eps ? Solution::Kind::Identity : Solution::Kind::NoSolution;
                } else {
                    s.kind = Solution::Kind::Linear;
                    s.roots.push_back(Complex(-c / b, 0));
                }
                return s;
            }
            s.kind = Solution::Kind::Quadratic;
            Num disc = b * b - 4 * a * c;
            if (std::fabs(disc) <= eps * eps + 1e-14L * b * b) {
                s.roots.push_back(Complex(-b / (2 * a), 0));
            } else if (disc > 0) {
                Num q = -(b + (b >= 0 ? 1 : -1) * std::sqrt(disc)) / 2;
                Num r1 = q / a;
                Num r2 = q != 0 ? c / q : r1;
                if (r1 > r2) std::swap(r1, r2);
                s.roots.push_back(Complex(r1, 0));
                s.roots.push_back(Complex(r2, 0));
            } else {
                Num re = -b / (2 * a);
                Num im = std::sqrt(-disc) / (2 * std::fabs(a));
                s.roots.push_back(Complex(re, -im));
                s.roots.push_back(Complex(re, im));
            }
            return s;
        }

        s.kind = Solution::Kind::Numeric;
        s.roots = scanRoots(f);
        return s;
    }

    const std::map<std::string, Num>& variables() const { return vars_; }

private:
    using Fn = std::function<Num(const std::vector<Num>&)>;
    struct FnInfo {
        int arity;
        Fn fn;
    };

    std::string src_;
    size_t pos_ = 0;
    std::map<std::string, Num> vars_;
    std::map<std::string, FnInfo> funcs_;

    static std::string trim(const std::string& s) {
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos) return "";
        return s.substr(a, s.find_last_not_of(" \t") - a + 1);
    }

    static bool isIdent(const std::string& s) {
        if (s.empty() || !(std::isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
        return std::all_of(s.begin(), s.end(), [](char ch) { return std::isalnum((unsigned char)ch) || ch == '_'; });
    }

    Num evalExpr(const std::string& text, const std::string& var, Num value) {
        std::string savedSrc = src_;
        size_t savedPos = pos_;
        auto it = vars_.find(var);
        bool had = it != vars_.end();
        Num old = had ? it->second : 0;
        vars_[var] = value;

        auto restore = [&] {
            src_ = savedSrc;
            pos_ = savedPos;
            if (had) vars_[var] = old;
            else vars_.erase(var);
        };

        try {
            src_ = text;
            pos_ = 0;
            Num r = expr();
            skipWs();
            if (pos_ < src_.size()) throw CalcError(std::string("unexpected character '") + src_[pos_] + "'");
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

    template <typename F>
    static bool fitQuadratic(F& f, Num& a, Num& b, Num& c) {
        try {
            Num f0 = f(0), f1 = f(1), fm = f(-1);
            c = f0;
            a = (f1 + fm) / 2 - c;
            b = (f1 - fm) / 2;
            if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c)) return false;
            for (Num x : {2.0L, -3.0L, 0.5L, 7.0L}) {
                Num expected = a * x * x + b * x + c;
                Num actual = f(x);
                Num scale = 1 + std::fabs(a) * x * x + std::fabs(b) * std::fabs(x) + std::fabs(c);
                if (!std::isfinite(actual) || std::fabs(actual - expected) > 1e-9L * scale) return false;
            }
            return true;
        } catch (const CalcError&) {
            return false;
        }
    }

    template <typename F>
    static std::vector<Complex> scanRoots(F& f) {
        const Num lo = -100, hi = 100, step = 0.01L;
        std::vector<Num> found;
        auto safe = [&](Num x) -> Num {
            try {
                return f(x);
            } catch (const CalcError&) {
                return std::nan("");
            }
        };
        auto add = [&](Num r) {
            for (Num v : found)
                if (std::fabs(v - r) < 1e-7L) return;
            found.push_back(r);
        };

        Num prevX = lo;
        Num prevY = safe(prevX);
        for (int i = 1; i <= (int)((hi - lo) / step); ++i) {
            Num x = lo + i * step;
            Num y = safe(x);
            if (std::isfinite(prevY) && std::isfinite(y)) {
                if (prevY == 0) add(prevX);
                else if (prevY * y < 0) {
                    Num l = prevX, h = x, yl = prevY;
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
            prevX = x;
            prevY = y;
        }
        if (prevY == 0) add(prevX);
        std::sort(found.begin(), found.end());
        std::vector<Complex> out;
        for (Num r : found) out.push_back(Complex(r, 0));
        return out;
    }

    Num toRad(Num x) const { return degrees ? x * vars_.at("pi") / 180 : x; }
    Num fromRad(Num x) const { return degrees ? x * 180 / vars_.at("pi") : x; }

    void add1(const std::string& n, std::function<Num(Num)> f) {
        funcs_[n] = {1, [f](const std::vector<Num>& a) { return f(a[0]); }};
    }
    void add2(const std::string& n, std::function<Num(Num, Num)> f) {
        funcs_[n] = {2, [f](const std::vector<Num>& a) { return f(a[0], a[1]); }};
    }

    void registerFunctions() {
        add1("sin", [this](Num x) { return std::sin(toRad(x)); });
        add1("cos", [this](Num x) { return std::cos(toRad(x)); });
        add1("tan", [this](Num x) { return std::tan(toRad(x)); });
        add1("asin", [this](Num x) { return fromRad(std::asin(x)); });
        add1("acos", [this](Num x) { return fromRad(std::acos(x)); });
        add1("atan", [this](Num x) { return fromRad(std::atan(x)); });
        add2("atan2", [this](Num y, Num x) { return fromRad(std::atan2(y, x)); });
        add1("sinh", [](Num x) { return std::sinh(x); });
        add1("cosh", [](Num x) { return std::cosh(x); });
        add1("tanh", [](Num x) { return std::tanh(x); });
        add1("sqrt", [](Num x) {
            if (x < 0) throw CalcError("sqrt of a negative number");
            return std::sqrt(x);
        });
        add1("cbrt", [](Num x) { return std::cbrt(x); });
        add1("exp", [](Num x) { return std::exp(x); });
        add1("ln", [](Num x) {
            if (x <= 0) throw CalcError("ln is only defined for x > 0");
            return std::log(x);
        });
        add1("log", [](Num x) {
            if (x <= 0) throw CalcError("log is only defined for x > 0");
            return std::log10(x);
        });
        add1("log2", [](Num x) {
            if (x <= 0) throw CalcError("log2 is only defined for x > 0");
            return std::log2(x);
        });
        add2("logb", [](Num x, Num b) {
            if (x <= 0 || b <= 0 || b == 1) throw CalcError("invalid arguments for logb(x, base)");
            return std::log(x) / std::log(b);
        });
        add1("abs", [](Num x) { return std::fabs(x); });
        add1("floor", [](Num x) { return std::floor(x); });
        add1("ceil", [](Num x) { return std::ceil(x); });
        add1("round", [](Num x) { return std::round(x); });
        add1("fact", [](Num x) { return factorial(x); });
        add2("pow", [](Num a, Num b) { return std::pow(a, b); });
        add2("mod", [](Num a, Num b) {
            if (b == 0) throw CalcError("modulo by zero");
            return std::fmod(a, b);
        });
        add2("root", [](Num x, Num n) { return std::pow(x, 1 / n); });
        funcs_["min"] = {-1, [](const std::vector<Num>& a) {
                             Num m = a[0];
                             for (Num v : a) m = std::min(m, v);
                             return m;
                         }};
        funcs_["max"] = {-1, [](const std::vector<Num>& a) {
                             Num m = a[0];
                             for (Num v : a) m = std::max(m, v);
                             return m;
                         }};
    }

    static Num factorial(Num x) {
        if (x < 0 || x != std::floor(x)) throw CalcError("factorial requires an integer >= 0");
        if (x > 1000) throw CalcError("factorial too large");
        Num r = 1;
        for (int i = 2; i <= (int)x; ++i) r *= i;
        return r;
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

    Num expr() {
        Num v = term();
        for (;;) {
            if (eat('+')) v += term();
            else if (eat('-')) v -= term();
            else return v;
        }
    }

    Num term() {
        Num v = unary();
        for (;;) {
            if (eat('*')) v *= unary();
            else if (eat('/')) {
                Num d = unary();
                if (d == 0) throw CalcError("division by zero");
                v /= d;
            } else if (eat('%')) {
                Num d = unary();
                if (d == 0) throw CalcError("modulo by zero");
                v = std::fmod(v, d);
            } else return v;
        }
    }

    Num unary() {
        if (eat('-')) return -unary();
        if (eat('+')) return unary();
        return power();
    }

    Num power() {
        Num base = postfix();
        if (eat('^')) return std::pow(base, unary());
        return base;
    }

    Num postfix() {
        Num v = primary();
        while (eat('!')) v = factorial(v);
        return v;
    }

    Num primary() {
        skipWs();
        if (pos_ >= src_.size()) throw CalcError("incomplete expression");
        char c = src_[pos_];

        if (std::isdigit((unsigned char)c) || c == '.') return number();

        if (c == '(') {
            ++pos_;
            Num v = expr();
            if (!eat(')')) throw CalcError("missing ')'");
            return v;
        }

        if (std::isalpha((unsigned char)c) || c == '_') {
            std::string id = readIdent();
            if (eat('(')) return call(id);
            auto it = vars_.find(id);
            if (it == vars_.end()) throw CalcError("unknown variable '" + id + "'");
            return it->second;
        }
        throw CalcError(std::string("unexpected character '") + c + "'");
    }

    Num number() {
        size_t s = pos_;
        while (pos_ < src_.size() && (std::isdigit((unsigned char)src_[pos_]) || src_[pos_] == '.')) ++pos_;
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            size_t save = pos_++;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
            if (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) {
                while (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) ++pos_;
            } else pos_ = save;
        }
        try {
            return std::stold(src_.substr(s, pos_ - s));
        } catch (...) {
            throw CalcError("invalid number '" + src_.substr(s, pos_ - s) + "'");
        }
    }

    Num call(const std::string& name) {
        auto it = funcs_.find(name);
        if (it == funcs_.end()) throw CalcError("unknown function '" + name + "'");
        std::vector<Num> args;
        if (!eat(')')) {
            do args.push_back(expr());
            while (eat(','));
            if (!eat(')')) throw CalcError("missing ')' in call to " + name);
        }
        const FnInfo& f = it->second;
        if (f.arity == -1 ? args.empty() : (int)args.size() != f.arity)
            throw CalcError(name + " expects " + (f.arity == -1 ? "at least 1" : std::to_string(f.arity)) + " argument(s)");
        return f.fn(args);
    }
};
