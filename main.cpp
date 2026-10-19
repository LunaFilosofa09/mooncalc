#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bigint.hpp"
#include "calculator.hpp"
#include "format.hpp"
#include "lineedit.hpp"
#include "numeric.hpp"
#include "units.hpp"

namespace ui {

struct Rgb {
    int r, g, b;
};

const Rgb kBlack{14, 11, 22};
const Rgb kBorder{139, 92, 246};
const Rgb kAccent{167, 139, 250};
const Rgb kText{221, 214, 254};
const Rgb kDim{113, 113, 138};
const Rgb kError{244, 114, 182};

bool colorEnabled = false;

std::string fg(Rgb c) {
    return colorEnabled ? "\033[38;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m" : "";
}
std::string bg(Rgb c) {
    return colorEnabled ? "\033[48;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" + std::to_string(c.b) + "m" : "";
}
std::string bold() { return colorEnabled ? "\033[1m" : ""; }
std::string reset() { return colorEnabled ? "\033[0m" : ""; }

size_t visibleLength(const std::string& s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = s[i];
        if (ch == 0x1B) {
            while (i < s.size() && s[i] != 'm') ++i;
        } else if ((ch & 0xC0) != 0x80) {
            ++n;
        }
    }
    return n;
}

std::string repeat(const std::string& s, size_t n) {
    std::string out;
    for (size_t i = 0; i < n; ++i) out += s;
    return out;
}

void panel(const std::string& title, const std::vector<std::string>& lines, size_t minWidth = 46) {
    size_t inner = std::max(minWidth, visibleLength(title) + 6);
    for (const auto& l : lines) inner = std::max(inner, visibleLength(l) + 4);

    const std::string base = bg(kBlack);
    const std::string edge = base + fg(kBorder);

    size_t titleSpace = title.empty() ? 0 : visibleLength(title) + 2;
    std::cout << edge << "╭─";
    if (!title.empty()) std::cout << " " << bold() << fg(kAccent) << title << reset() << edge << " ";
    std::cout << repeat("─", inner - 1 - titleSpace) << "╮" << reset() << "\n";

    for (const auto& l : lines) {
        std::cout << edge << "│" << base << fg(kText) << "  " << l << std::string(inner - 2 - visibleLength(l), ' ') << edge
                  << "│" << reset() << "\n";
    }
    std::cout << edge << "╰" << repeat("─", inner) << "╯" << reset() << "\n";
}

void banner() {
    panel("MoonCalc", {"scientific calculator in C++17", "", "help  ·  solve x^2-4=0  ·  plot sin(x)  ·  quit"});
}

void help(const std::string& topic) {
    if (topic == "functions") {
        panel("Functions",
              {"Trigonometry  sin cos tan asin acos atan atan2",
               "Hyperbolic    sinh cosh tanh asinh acosh atanh",
               "Powers/logs   sqrt cbrt exp ln log log2 logb(x,b)",
               "              root(x,n) pow(a,b) mod(a,b) hypot",
               "Complex       re im conj arg abs   (i is the imaginary unit)",
               "Rounding      floor ceil round trunc sign",
               "Special       fact gamma lgamma beta erf erfc fib",
               "Number theory gcd lcm ncr npr isprime",
               "Statistics    mean median var varp std stdp count",
               "              min max sum prod  (lists or matrices)",
               "              linreg(xs,ys) corr(xs,ys)",
               "              normpdf(x,mu,sigma) normcdf(x,mu,sigma)",
               "",
               "Matrices      [1,2;3,4]  det inv transpose trace rank",
               "              eye zeros ones rows cols norm dot cross",
               "",
               "Calculus      diff(expr, at)           diff(sin(x), 1)",
               "              integral(expr, a, b)     integral(x^2, 0, 3)",
               "              limit(expr, at)          limit(sin(x)/x, 0)",
               "              sum(expr, k, 1, 10)      prod(expr, k, 1, 5)",
               "              findroot(expr, guess)    argmin/argmax(expr, a, b)",
               "              (an optional variable name goes after expr)"});
        return;
    }
    panel("Help",
          {"Operators    + - * / %  ^ (power)  ! (factorial)",
           "Implicit     2pi  3(4+1)  2sin(x)  (same precedence as *)",
           "Constants    pi  e  i  inf  ans (last result)",
           "Variables    x = 5*2        Functions   f(x,y) = x^2+y",
           "Numbers      1e-3  0xFF  0b1010  0o17",
           "Complex      (2+3i)*(1-i)   sqrt(-4)",
           "",
           "Equations    solve 2*x+3=11",
           "             solve x^3-6*x^2+11*x-6=0",
           "             solve x+y=3, x-y=1",
           "             solve y: 3*y-1=y+7   (explicit unknown)",
           "",
           "Tools        plot sin(x) from -6.28 to 6.28",
           "             factor 360     hex 255     bin 10     oct 8",
           "             base 255 7     convert 100 km to mi     units",
           "             bigfact 100    bigpow 2 200    bigfib 300",
           "",
           "Commands     deg | rad   vars   funcs   help functions   quit"});
}

inline std::string fmt(Num v) { return fmtx::number(v); }
inline std::string fmtComplex(const Complex& z) { return fmtx::complex(z); }

void result(const std::string& text) {
    std::cout << "  " << fg(kBorder) << "▸ " << bold() << fg(kText) << text << reset() << "\n";
}

void note(const std::string& text) { std::cout << "  " << fg(kDim) << text << reset() << "\n"; }

void error(const std::string& text) { std::cout << "  " << fg(kError) << "✗ " << text << reset() << "\n"; }

void printValue(const Value& v) {
    if (!v.isMatrix) {
        result(fmtComplex(v.scalar));
        return;
    }
    const Matrix& m = v.matrix;
    std::vector<std::string> cells(m.data.size());
    std::vector<size_t> widths(m.cols, 0);
    for (size_t i = 0; i < m.rows; ++i)
        for (size_t j = 0; j < m.cols; ++j) {
            cells[i * m.cols + j] = fmtComplex(m.at(i, j));
            widths[j] = std::max(widths[j], visibleLength(cells[i * m.cols + j]));
        }
    for (size_t i = 0; i < m.rows; ++i) {
        std::string line = "[ ";
        for (size_t j = 0; j < m.cols; ++j) {
            const std::string& c = cells[i * m.cols + j];
            line += std::string(widths[j] - visibleLength(c), ' ') + c + (j + 1 < m.cols ? "  " : "");
        }
        line += " ]";
        if (i == 0) result(line);
        else std::cout << "    " << bold() << fg(kText) << line << reset() << "\n";
    }
}

void printSolution(const Solution& s) {
    using K = Solution::Kind;
    switch (s.kind) {
        case K::Identity:
            note("identity or underdetermined");
            result("infinitely many solutions");
            return;
        case K::NoSolution:
            note("inconsistent");
            result("no solution");
            return;
        case K::Numeric:
            note("numeric search in [-100, 100] (real roots)");
            if (s.roots.empty()) {
                result("no real solutions found");
                return;
            }
            break;
        case K::Linear:
            note("linear equation");
            break;
        case K::Quadratic:
            note("quadratic equation");
            break;
        case K::Polynomial:
            note("polynomial equation, degree " + std::to_string(s.degree));
            break;
        case K::System:
            note("linear system");
            for (const auto& [name, value] : s.values) result(name + " = " + fmt(value));
            return;
    }
    std::vector<Complex> roots = s.roots;
    size_t total = roots.size();
    if (total > 20) {
        std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) { return std::abs(a) < std::abs(b); });
        roots.resize(20);
        std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) { return a.real() < b.real(); });
        note(std::to_string(total) + " solutions in range; showing the 20 closest to 0");
    }
    for (size_t i = 0; i < roots.size(); ++i) {
        std::string label = s.variable;
        if (roots.size() > 1) label += std::to_string(i + 1);
        result(label + " = " + fmtComplex(roots[i]));
    }
}

std::string shortNumber(Num v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.4g", (double)v);
    return buf;
}

std::string brailleChar(int mask) {
    std::string s;
    s += (char)0xE2;
    s += (char)(0xA0 | (mask >> 6));
    s += (char)(0x80 | (mask & 0x3F));
    return s;
}

void plot(Calculator& calc, const std::string& rest) {
    std::string exprText = rest;
    Num a = -10, b = 10;
    size_t from = rest.find(" from ");
    if (from != std::string::npos) {
        exprText = rest.substr(0, from);
        std::string range = rest.substr(from + 6);
        size_t to = range.find(" to ");
        if (to == std::string::npos) throw CalcError("usage: plot <expr> from <a> to <b>");
        a = calc.evaluateReal(range.substr(0, to));
        b = calc.evaluateReal(range.substr(to + 4));
    }
    if (!(a < b)) throw CalcError("the range must satisfy a < b");

    const int cellsW = 64, cellsH = 16;
    const int W = cellsW * 2, H = cellsH * 4;
    std::vector<Num> ys = calc.sample(exprText, "x", a, b, W);

    std::vector<Num> finite;
    for (Num y : ys)
        if (std::isfinite(y)) finite.push_back(y);
    if (finite.empty()) throw CalcError("nothing to plot: no real values in that range");
    std::sort(finite.begin(), finite.end());
    auto q = [&](Num p) { return finite[(size_t)(p * (finite.size() - 1))]; };
    Num q1 = q(0.25L), q3 = q(0.75L), iqr = q3 - q1;
    Num lo = finite.front(), hi = finite.back();
    if (iqr > 0) {
        lo = std::max(lo, q1 - 3 * iqr);
        hi = std::min(hi, q3 + 3 * iqr);
    }
    if (hi - lo < 1e-12L) {
        lo -= 1;
        hi += 1;
    }
    Num pad = (hi - lo) * 0.05L;
    lo -= pad;
    hi += pad;

    std::vector<std::vector<int>> curve(cellsH, std::vector<int>(cellsW, 0)), axis = curve;
    static const int bitsLeft[4] = {0x01, 0x02, 0x04, 0x40};
    static const int bitsRight[4] = {0x08, 0x10, 0x20, 0x80};
    auto setDot = [&](std::vector<std::vector<int>>& grid, int px, int py) {
        if (px < 0 || px >= W || py < 0 || py >= H) return;
        grid[py / 4][px / 2] |= (px % 2 ? bitsRight : bitsLeft)[py % 4];
    };

    if (lo < 0 && hi > 0) {
        int row = (int)std::lround((hi - 0) / (hi - lo) * (H - 1));
        for (int px = 0; px < W; px += 2) setDot(axis, px, row);
    }
    if (a < 0 && b > 0) {
        int col = (int)std::lround((0 - a) / (b - a) * (W - 1));
        for (int py = 0; py < H; py += 2) setDot(axis, col, py);
    }

    int prevRow = -1;
    for (int px = 0; px < W; ++px) {
        Num y = ys[px];
        if (!std::isfinite(y) || y < lo || y > hi) {
            prevRow = -1;
            continue;
        }
        int row = (int)std::lround((hi - y) / (hi - lo) * (H - 1));
        if (prevRow >= 0 && std::abs(row - prevRow) <= H * 6 / 10) {
            int from = std::min(row, prevRow), to = std::max(row, prevRow);
            for (int r = from; r <= to; ++r) setDot(curve, px, r);
        } else {
            setDot(curve, px, row);
        }
        prevRow = row;
    }

    std::vector<std::string> lines;
    for (int r = 0; r < cellsH; ++r) {
        std::string label;
        if (r == 0) label = shortNumber(hi);
        else if (r == cellsH - 1) label = shortNumber(lo);
        else if (r == cellsH / 2) label = shortNumber((hi + lo) / 2);
        std::string line = std::string(label.size() < 9 ? 9 - label.size() : 0, ' ') + label + " ";
        int lastColor = 0;
        for (int c = 0; c < cellsW; ++c) {
            int mask = curve[r][c] | axis[r][c];
            if (!mask) {
                line += " ";
                continue;
            }
            int color = curve[r][c] ? 1 : 2;
            if (color != lastColor) line += fg(color == 1 ? kAccent : kDim);
            lastColor = color;
            line += brailleChar(mask);
        }
        lines.push_back(line);
    }
    std::string left = shortNumber(a), right = shortNumber(b);
    size_t gap = cellsW > (int)(left.size() + right.size()) ? cellsW - left.size() - right.size() : 1;
    lines.push_back(std::string(10, ' ') + left + std::string(gap, ' ') + right);
    panel("plot  " + exprText, lines, 0);
}

std::string prompt(bool degrees) {
    return fg(kDim) + (degrees ? "deg " : "rad ") + fg(kBorder) + bold() + "❯ " + reset() + fg(kText);
}

}

namespace {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

bool startsWithWord(const std::string& line, const std::string& word, std::string& rest) {
    if (line.compare(0, word.size(), word) != 0) return false;
    if (line.size() == word.size()) {
        rest.clear();
        return true;
    }
    if (line[word.size()] != ' ' && line[word.size()] != '\t') return false;
    rest = trim(line.substr(word.size()));
    return true;
}

std::vector<std::string> words(const std::string& s) {
    std::istringstream in(s);
    std::vector<std::string> out;
    std::string w;
    while (in >> w) out.push_back(w);
    return out;
}

long long integerArg(Calculator& calc, const std::string& text, const char* what) {
    return numeric::toInteger(calc.evaluateReal(text), what);
}

void showBig(const std::string& digits) {
    ui::note(std::to_string(digits.size()) + " digits");
    if (digits.size() <= 120) {
        ui::result(digits);
    } else {
        ui::result(digits.substr(0, 60) + " … " + digits.substr(digits.size() - 30));
    }
}

void runFactor(Calculator& calc, const std::string& rest) {
    if (rest.empty()) throw CalcError("usage: factor <integer>");
    long long n = integerArg(calc, rest, "factor argument");
    if (n < 2) throw CalcError("factor needs an integer >= 2");
    auto parts = numeric::factorize((numeric::u64)n);
    if (parts.size() == 1 && parts[0].second == 1) {
        ui::result(std::to_string(n) + " is prime");
        return;
    }
    std::string text = std::to_string(n) + " = ";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) text += " × ";
        text += std::to_string(parts[i].first);
        if (parts[i].second > 1) text += "^" + std::to_string(parts[i].second);
    }
    ui::result(text);
}

void runBase(Calculator& calc, const std::string& command, const std::string& rest) {
    if (rest.empty()) throw CalcError("usage: " + command + " <integer expression>");
    int base = command == "hex" ? 16 : command == "bin" ? 2 : 8;
    std::string expr = rest;
    if (command == "base") {
        size_t sp = rest.find_last_of(" \t");
        if (sp == std::string::npos) throw CalcError("usage: base <integer> <base 2-36>");
        expr = trim(rest.substr(0, sp));
        base = (int)integerArg(calc, rest.substr(sp + 1), "base");
    }
    long long v = integerArg(calc, expr, "value");
    std::string digits = numeric::toBase(v, base);
    std::string prefix = command == "hex" ? "0x" : command == "bin" ? "0b" : command == "oct" ? "0o" : "";
    if (!prefix.empty() && !digits.empty() && digits[0] == '-') digits = "-" + prefix + digits.substr(1);
    else digits = prefix + digits;
    ui::result(digits + (command == "base" ? "  (base " + std::to_string(base) + ")" : ""));
}

void runConvert(Calculator& calc, const std::string& rest) {
    size_t to = rest.find(" to ");
    if (to == std::string::npos) throw CalcError("usage: convert <value> <unit> to <unit>");
    std::string left = trim(rest.substr(0, to));
    std::string target = trim(rest.substr(to + 4));
    size_t sp = left.find_last_of(" \t");
    if (sp == std::string::npos || target.empty()) throw CalcError("usage: convert <value> <unit> to <unit>");
    std::string from = trim(left.substr(sp + 1));
    Num value = calc.evaluateReal(left.substr(0, sp));
    ui::result(ui::fmt(units::convert(value, from, target)) + " " + target);
}

void runBig(Calculator& calc, const std::string& command, const std::string& rest) {
    std::vector<std::string> args = words(rest);
    if (command == "bigfact") {
        if (args.size() != 1) throw CalcError("usage: bigfact <n>");
        long long n = integerArg(calc, args[0], "n");
        if (n < 0 || n > 20000) throw CalcError("n must be between 0 and 20000");
        showBig(BigInt::factorial((unsigned)n).str());
    } else if (command == "bigfib") {
        if (args.size() != 1) throw CalcError("usage: bigfib <n>");
        long long n = integerArg(calc, args[0], "n");
        if (n < 0 || n > 50000) throw CalcError("n must be between 0 and 50000");
        showBig(BigInt::fibonacci((unsigned)n).str());
    } else {
        if (args.size() != 2) throw CalcError("usage: bigpow <base> <exponent>");
        long long b = integerArg(calc, args[0], "base"), e = integerArg(calc, args[1], "exponent");
        if (b < 0 || b > 4294967295LL) throw CalcError("base must be between 0 and 4294967295");
        if (e < 0 || e > 100000) throw CalcError("exponent must be between 0 and 100000");
        showBig(BigInt::power((unsigned)b, (unsigned)e).str());
    }
}

void listVariables(const Calculator& calc) {
    for (const auto& [name, value] : calc.variables()) {
        if (value.isMatrix)
            ui::result(name + " = matrix " + std::to_string(value.matrix.rows) + "x" + std::to_string(value.matrix.cols));
        else
            ui::result(name + " = " + ui::fmtComplex(value.scalar));
    }
}

void listFunctions(const Calculator& calc) {
    if (calc.userFunctions().empty()) {
        ui::note("no user functions defined; try f(x) = x^2 + 1");
        return;
    }
    for (const auto& [name, f] : calc.userFunctions()) {
        std::string sig = name + "(";
        for (size_t i = 0; i < f.params.size(); ++i) sig += (i ? ", " : "") + f.params[i];
        ui::result(sig + ") = " + f.body);
    }
}

bool dispatch(Calculator& calc, const std::string& line) {
    std::string rest;
    if (line == "quit" || line == "exit") return false;
    if (line == "help") { ui::help(""); return true; }
    if (startsWithWord(line, "help", rest)) { ui::help(rest); return true; }
    if (line == "deg") { calc.degrees = true; return true; }
    if (line == "rad") { calc.degrees = false; return true; }
    if (line == "vars") { listVariables(calc); return true; }
    if (line == "funcs") { listFunctions(calc); return true; }
    if (line == "units") {
        for (const auto& l : units::listing()) ui::note(l);
        return true;
    }
    if (startsWithWord(line, "solve", rest)) {
        if (rest.empty()) throw CalcError("usage: solve <equation>   e.g. solve x^2-4=0");
        ui::printSolution(calc.solve(rest));
        return true;
    }
    if (startsWithWord(line, "plot", rest)) {
        if (rest.empty()) throw CalcError("usage: plot <expr> [from <a> to <b>]");
        ui::plot(calc, rest);
        return true;
    }
    if (startsWithWord(line, "factor", rest)) { runFactor(calc, rest); return true; }
    for (const char* cmd : {"hex", "bin", "oct", "base"})
        if (startsWithWord(line, cmd, rest)) { runBase(calc, cmd, rest); return true; }
    if (startsWithWord(line, "convert", rest)) { runConvert(calc, rest); return true; }
    for (const char* cmd : {"bigfact", "bigpow", "bigfib"})
        if (startsWithWord(line, cmd, rest)) { runBig(calc, cmd, rest); return true; }

    Outcome out = calc.execute(line);
    if (!out.defined.empty()) ui::note("defined " + out.defined);
    else ui::printValue(out.value);
    return true;
}

}

int main() {
    ui::colorEnabled = isatty(STDOUT_FILENO) && std::getenv("NO_COLOR") == nullptr;

    Calculator calc;
    LineEditor editor;
    ui::banner();

    std::string line;
    while (editor.readLine(ui::prompt(calc.degrees), 6, line)) {
        std::cout << ui::reset();
        line = trim(line);
        if (line.empty()) continue;
        try {
            if (!dispatch(calc, line)) break;
        } catch (const CalcError& e) {
            ui::error(e.what());
        }
    }
    std::cout << ui::reset();
    return 0;
}
