#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "calculator.hpp"

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
    for (unsigned char ch : s)
        if ((ch & 0xC0) != 0x80) ++n;
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
        std::cout << edge << "│" << base << fg(kText) << "  " << l
                  << std::string(inner - 2 - visibleLength(l), ' ') << edge << "│" << reset() << "\n";
    }
    std::cout << edge << "╰" << repeat("─", inner) << "╯" << reset() << "\n";
}

void banner() {
    panel("MoonCalc",
          {"scientific calculator in C++17", "", "help  ·  solve x^2-4=0  ·  quit"});
}

void help() {
    panel("Help",
          {"Operators    + - * / %  ^ (power)  ! (factorial)",
           "Constants    pi  e  ans (last result)",
           "Variables    x = 5*2     y = x^2",
           "",
           "Trigonometry  sin cos tan asin acos atan atan2",
           "Hyperbolic    sinh cosh tanh",
           "Powers/logs   sqrt cbrt exp ln log log2 logb(x,b)",
           "              root(x,n) pow(a,b) mod(a,b)",
           "Other         abs floor ceil round fact min max",
           "",
           "Equations     solve 2*x+3=11",
           "              solve x^2-5*x+6=0",
           "              solve y: 3*y-1=y+7   (explicit unknown)",
           "",
           "Commands      deg | rad   vars   help   quit"});
}

std::string fmt(Num v) {
    if (std::fabs(v) < 1e-14L) v = 0;
    std::ostringstream os;
    os.precision(15);
    os << v;
    return os.str();
}

std::string fmtComplex(const Complex& z) {
    if (std::fabs(z.imag()) < 1e-14L) return fmt(z.real());
    std::string re = fmt(z.real());
    std::string im = fmt(std::fabs(z.imag()));
    bool neg = z.imag() < 0;
    if (std::fabs(z.real()) < 1e-14L) return std::string(neg ? "-" : "") + im + "i";
    return re + (neg ? " - " : " + ") + im + "i";
}

void result(const std::string& text) {
    std::cout << "  " << fg(kBorder) << "▸ " << bold() << fg(kText) << text << reset() << "\n";
}

void note(const std::string& text) {
    std::cout << "  " << fg(kDim) << text << reset() << "\n";
}

void error(const std::string& text) {
    std::cout << "  " << fg(kError) << "✗ " << text << reset() << "\n";
}

void printSolution(const Solution& s) {
    using K = Solution::Kind;
    switch (s.kind) {
        case K::Identity:
            note("identity equation");
            result("infinitely many solutions");
            return;
        case K::NoSolution:
            note("inconsistent equation");
            result("no solution");
            return;
        case K::Numeric:
            note("numeric search in [-100, 100]");
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
    }
    for (size_t i = 0; i < s.roots.size(); ++i) {
        std::string label = s.variable;
        if (s.roots.size() > 1) label += std::to_string(i + 1);
        result(label + " = " + fmtComplex(s.roots[i]));
    }
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

}

int main() {
    ui::colorEnabled = isatty(STDOUT_FILENO) && std::getenv("NO_COLOR") == nullptr;

    Calculator calc;
    ui::banner();

    std::string line;
    while (std::cout << ui::prompt(calc.degrees) << std::flush, std::getline(std::cin, line)) {
        std::cout << ui::reset();
        line = trim(line);
        if (line.empty()) continue;

        if (line == "quit" || line == "exit") break;
        if (line == "help") { ui::help(); continue; }
        if (line == "deg") { calc.degrees = true; continue; }
        if (line == "rad") { calc.degrees = false; continue; }
        if (line == "vars") {
            for (const auto& [name, value] : calc.variables()) ui::result(name + " = " + ui::fmt(value));
            continue;
        }

        try {
            std::string rest;
            if (startsWithWord(line, "solve", rest)) {
                if (rest.empty()) throw CalcError("usage: solve <equation>   e.g. solve x^2-4=0");
                ui::printSolution(calc.solve(rest));
            } else {
                ui::result(ui::fmt(calc.evaluate(line)));
            }
        } catch (const CalcError& e) {
            ui::error(e.what());
        }
    }
    std::cout << ui::reset();
    return 0;
}
