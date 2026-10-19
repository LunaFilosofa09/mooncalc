#pragma once

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "calculator.hpp"
#include "format.hpp"
#include "imgui.h"

namespace gui {

namespace pal {
constexpr unsigned kBackground = 0x09070F;
constexpr unsigned kPanel = 0x120E20;
constexpr unsigned kCard = 0x1A1530;
constexpr unsigned kCardHover = 0x251D44;
constexpr unsigned kBorder = 0x2A2145;
constexpr unsigned kAccent = 0x8B5CF6;
constexpr unsigned kAccentHover = 0xA78BFA;
constexpr unsigned kAccentDeep = 0x6D3FE0;
constexpr unsigned kText = 0xEDE9FE;
constexpr unsigned kSoft = 0xC4B5FD;
constexpr unsigned kDim = 0x7C7799;
constexpr unsigned kError = 0xF472B6;
}

inline ImVec4 col(unsigned rgb, float a = 1.0f) {
    return ImVec4(((rgb >> 16) & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f, (rgb & 255) / 255.0f, a);
}

inline ImU32 col32(unsigned rgb, float a = 1.0f) { return ImGui::ColorConvertFloat4ToU32(col(rgb, a)); }

struct HistoryEntry {
    std::string expression;
    std::string result;
    bool error = false;
};

struct State {
    Calculator calc;
    std::vector<HistoryEntry> history;
    char input[512] = "";
    std::string display = "0";
    std::string subtitle;
    bool displayError = false;
    bool focusInput = true;

    char equation[512] = "";
    char unknown[64] = "";
    bool hasSolution = false;
    Solution solution;
    std::string equationError;
    float toast = 0.0f;

    ImFont* regular = nullptr;
    ImFont* medium = nullptr;
    ImFont* large = nullptr;
    ImFont* small = nullptr;
};

inline State S;

inline std::string trimmed(const char* s) {
    std::string t = s;
    size_t a = t.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    return t.substr(a, t.find_last_not_of(" \t") - a + 1);
}

inline void applyTheme() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 0;
    st.ChildRounding = 18;
    st.FrameRounding = 14;
    st.PopupRounding = 14;
    st.ScrollbarRounding = 12;
    st.GrabRounding = 12;
    st.TabRounding = 12;
    st.WindowBorderSize = 0;
    st.ChildBorderSize = 1;
    st.FrameBorderSize = 1;
    st.WindowPadding = ImVec2(28, 22);
    st.FramePadding = ImVec2(16, 12);
    st.ItemSpacing = ImVec2(10, 10);
    st.ScrollbarSize = 10;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = col(pal::kText);
    c[ImGuiCol_TextDisabled] = col(pal::kDim);
    c[ImGuiCol_WindowBg] = col(pal::kBackground);
    c[ImGuiCol_ChildBg] = col(pal::kPanel);
    c[ImGuiCol_PopupBg] = col(pal::kPanel);
    c[ImGuiCol_Border] = col(pal::kBorder);
    c[ImGuiCol_FrameBg] = col(pal::kCard);
    c[ImGuiCol_FrameBgHovered] = col(pal::kCardHover);
    c[ImGuiCol_FrameBgActive] = col(pal::kCardHover);
    c[ImGuiCol_Button] = col(pal::kCard);
    c[ImGuiCol_ButtonHovered] = col(pal::kCardHover);
    c[ImGuiCol_ButtonActive] = col(pal::kAccentDeep);
    c[ImGuiCol_Header] = col(pal::kAccent, 0.18f);
    c[ImGuiCol_HeaderHovered] = col(pal::kAccent, 0.32f);
    c[ImGuiCol_HeaderActive] = col(pal::kAccent, 0.5f);
    c[ImGuiCol_Tab] = col(pal::kPanel);
    c[ImGuiCol_TabHovered] = col(pal::kAccent, 0.55f);
    c[ImGuiCol_TabSelected] = col(pal::kAccentDeep, 0.85f);
    c[ImGuiCol_TabDimmed] = col(pal::kPanel);
    c[ImGuiCol_TabDimmedSelected] = col(pal::kAccentDeep, 0.6f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = col(pal::kBorder);
    c[ImGuiCol_ScrollbarGrabHovered] = col(pal::kAccent, 0.6f);
    c[ImGuiCol_ScrollbarGrabActive] = col(pal::kAccent);
    c[ImGuiCol_Separator] = col(pal::kBorder);
    c[ImGuiCol_TextSelectedBg] = col(pal::kAccent, 0.45f);
    c[ImGuiCol_NavCursor] = col(pal::kAccent, 0.0f);
}

inline void init(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    const char* candidates[] = {"/System/Library/Fonts/SFNS.ttf", "/System/Library/Fonts/Helvetica.ttc",
                                "/Library/Fonts/Arial.ttf"};
    const char* found = nullptr;
    for (const char* path : candidates) {
        if (FILE* f = std::fopen(path, "rb")) {
            std::fclose(f);
            found = path;
            break;
        }
    }
    auto load = [&](float px) -> ImFont* {
        ImFont* font = found ? io.Fonts->AddFontFromFileTTF(found, px * scale) : nullptr;
        return font ? font : io.Fonts->AddFontDefault();
    };
    S.regular = load(17);
    S.medium = load(24);
    S.large = load(46);
    S.small = load(14);
    io.FontGlobalScale = 1.0f / scale;

    applyTheme();
}

inline void textRight(const std::string& text, const ImVec4& color) {
    float w = ImGui::CalcTextSize(text.c_str()).x;
    float avail = ImGui::GetContentRegionAvail().x;
    if (w < avail) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - w);
    ImGui::TextColored(color, "%s", text.c_str());
}

inline void evaluate() {
    std::string text = trimmed(S.input);
    if (text.empty()) return;

    HistoryEntry entry{text, "", false};
    if (text == "deg" || text == "rad") {
        S.calc.degrees = text == "deg";
        entry.result = S.calc.degrees ? "Degrees mode" : "Radians mode";
    } else {
        try {
            Outcome out = S.calc.execute(text);
            entry.result = out.defined.empty() ? fmtx::inlineValue(out.value) : "defined " + out.defined;
        } catch (const CalcError& e) {
            entry.result = e.what();
            entry.error = true;
        }
    }
    S.display = entry.result;
    S.subtitle = text;
    S.displayError = entry.error;
    S.history.push_back(entry);
    if (!entry.error) S.input[0] = '\0';
    S.focusInput = true;
}

inline void appendInput(const char* text) {
    size_t len = std::strlen(S.input);
    size_t add = std::strlen(text);
    if (len + add + 1 < sizeof(S.input)) std::memcpy(S.input + len, text, add + 1);
    S.focusInput = true;
}

inline void backspace() {
    size_t len = std::strlen(S.input);
    if (len > 0) S.input[len - 1] = '\0';
    S.focusInput = true;
}

enum class Kind { Number, Operator, Function, Action, Equals, Mode };

struct Key {
    const char* label;
    const char* insert;
    Kind kind;
};

inline bool drawKey(const Key& key, ImVec2 size) {
    unsigned base = pal::kCard, hover = pal::kCardHover, active = pal::kAccentDeep, text = pal::kText;
    switch (key.kind) {
        case Kind::Number: break;
        case Kind::Operator:
            base = 0x2B1F63; hover = 0x3D2C8C; active = 0x5B3FD6; text = pal::kSoft;
            break;
        case Kind::Function:
            base = pal::kPanel; hover = 0x1F1838; active = pal::kBorder; text = pal::kAccentHover;
            break;
        case Kind::Action:
            base = 0x3B1534; hover = 0x56204D; active = 0x7A2B6B; text = pal::kError;
            break;
        case Kind::Equals:
            base = pal::kAccent; hover = pal::kAccentHover; active = pal::kAccentDeep; text = 0xFFFFFF;
            break;
        case Kind::Mode:
            base = pal::kPanel; hover = 0x1F1838; active = pal::kBorder; text = pal::kAccent;
            break;
    }
    ImGui::PushStyleColor(ImGuiCol_Button, col(base));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col(hover));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col(active));
    ImGui::PushStyleColor(ImGuiCol_Text, col(text));
    ImGui::PushStyleColor(ImGuiCol_Border, col(key.kind == Kind::Equals ? pal::kAccent : pal::kBorder));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16);
    bool pressed = ImGui::Button(key.label, size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return pressed;
}

inline void handleKey(const Key& key) {
    std::string ins = key.insert;
    if (ins == "AC") {
        S.input[0] = '\0';
        S.display = "0";
        S.subtitle.clear();
        S.displayError = false;
        S.focusInput = true;
    } else if (ins == "DEL") {
        backspace();
    } else if (ins == "=") {
        evaluate();
    } else if (ins == "MODE") {
        S.calc.degrees = !S.calc.degrees;
        S.focusInput = true;
    } else {
        appendInput(key.insert);
    }
}

inline void displayCard() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, col(pal::kPanel));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(26, 18));
    ImGui::BeginChild("display", ImVec2(0, 132), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushFont(S.small);
    std::string mode = S.calc.degrees ? "DEG" : "RAD";
    ImGui::TextColored(col(pal::kAccent), "%s", mode.c_str());
    ImGui::SameLine();
    textRight(S.subtitle.empty() ? "ready" : S.subtitle, col(pal::kDim));
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 4));
    unsigned color = S.displayError ? pal::kError : pal::kText;
    ImFont* fonts[] = {S.large, S.medium, S.regular};
    float avail = ImGui::GetContentRegionAvail().x;
    ImFont* chosen = S.regular;
    for (ImFont* f : fonts) {
        ImGui::PushFont(f);
        float w = ImGui::CalcTextSize(S.display.c_str()).x;
        ImGui::PopFont();
        if (w <= avail) {
            chosen = f;
            break;
        }
    }
    ImGui::PushFont(chosen);
    float w = ImGui::CalcTextSize(S.display.c_str()).x;
    if (w <= avail) {
        textRight(S.display, col(color));
    } else {
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(col(color), "%s", S.display.c_str());
        ImGui::PopTextWrapPos();
    }
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

inline void keypad() {
    static const Key rows[6][7] = {
        {{"sin", "sin(", Kind::Function}, {"cos", "cos(", Kind::Function}, {"tan", "tan(", Kind::Function},
         {"AC", "AC", Kind::Action}, {"DEL", "DEL", Kind::Action}, {"(", "(", Kind::Operator}, {")", ")", Kind::Operator}},
        {{"asin", "asin(", Kind::Function}, {"acos", "acos(", Kind::Function}, {"atan", "atan(", Kind::Function},
         {"7", "7", Kind::Number}, {"8", "8", Kind::Number}, {"9", "9", Kind::Number}, {"\xC3\xB7", "/", Kind::Operator}},
        {{"ln", "ln(", Kind::Function}, {"log", "log(", Kind::Function}, {"exp", "exp(", Kind::Function},
         {"4", "4", Kind::Number}, {"5", "5", Kind::Number}, {"6", "6", Kind::Number}, {"\xC3\x97", "*", Kind::Operator}},
        {{"sqrt", "sqrt(", Kind::Function}, {"x\xC2\xB2", "^2", Kind::Function}, {"x^y", "^", Kind::Function},
         {"1", "1", Kind::Number}, {"2", "2", Kind::Number}, {"3", "3", Kind::Number}, {"-", "-", Kind::Operator}},
        {{"pi", "pi", Kind::Function}, {"e", "e", Kind::Function}, {"ans", "ans", Kind::Function},
         {"0", "0", Kind::Number}, {".", ".", Kind::Number}, {",", ",", Kind::Number}, {"+", "+", Kind::Operator}},
        {{"MODE", "MODE", Kind::Mode}, {"i", "i", Kind::Function}, {"x!", "!", Kind::Function},
         {"=", "=", Kind::Equals}, {"", "", Kind::Number}, {"", "", Kind::Number}, {"", "", Kind::Number}},
    };

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float avail = ImGui::GetContentRegionAvail().x;
    float bw = (avail - spacing * 6) / 7.0f;
    float bh = 50;

    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Key& key = rows[r][c];
            if (r == 5 && c >= 4) break;
            if (c > 0) ImGui::SameLine();
            ImGui::PushID(r * 10 + c);
            ImVec2 size(bw, bh);
            Key shown = key;
            std::string modeLabel;
            if (key.kind == Kind::Mode) {
                modeLabel = S.calc.degrees ? "DEG" : "RAD";
                shown.label = modeLabel.c_str();
            }
            if (key.kind == Kind::Equals) size.x = bw * 4 + spacing * 3;
            if (drawKey(shown, size)) handleKey(key);
            ImGui::PopID();
        }
    }
}

inline void historyPanel() {
    ImGui::BeginChild("side", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImGui::PushFont(S.small);
    ImGui::TextColored(col(pal::kDim), "HISTORY");
    ImGui::SameLine();
    float cw = ImGui::CalcTextSize("clear").x + 24;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - cw);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kAccent));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 3));
    if (ImGui::Button("clear")) S.history.clear();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PopFont();

    if (S.history.empty()) {
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextColored(col(pal::kDim), "Results appear here.");
    }

    for (int i = (int)S.history.size() - 1; i >= 0; --i) {
        const HistoryEntry& e = S.history[i];
        ImGui::PushID(i);
        ImGui::PushFont(S.small);
        ImGui::TextColored(col(pal::kDim), "%s", e.expression.c_str());
        ImGui::PopFont();
        ImGui::PushStyleColor(ImGuiCol_Text, col(e.error ? pal::kError : pal::kSoft));
        if (ImGui::Selectable(("= " + e.result).c_str(), false, 0)) {
            if (!e.error) appendInput(e.result.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0, 8));
    ImGui::Separator();
    ImGui::PushFont(S.small);
    ImGui::TextColored(col(pal::kDim), "VARIABLES");
    ImGui::PopFont();
    int shown = 0;
    for (const auto& [name, value] : S.calc.variables()) {
        if (name == "pi" || name == "e" || name == "i" || name == "inf") continue;
        std::string text = name + " = " + fmtx::inlineValue(value);
        ImGui::PushID(name.c_str());
        if (ImGui::Selectable(text.c_str(), false, 0)) appendInput(name.c_str());
        ImGui::PopID();
        ++shown;
    }
    if (!shown) ImGui::TextColored(col(pal::kDim), "none yet");
    ImGui::EndChild();
}

inline void calculatorTab() {
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float total = ImGui::GetContentRegionAvail().x;
    float side = std::clamp(total * 0.28f, 240.0f, 340.0f);
    bool showSide = total > 900;
    float mainWidth = showSide ? total - side - spacing : total;

    ImGui::BeginChild("main", ImVec2(mainWidth, 0), ImGuiChildFlags_None);
    displayCard();

    ImGui::PushFont(S.medium);
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 14));
    ImGui::PushStyleColor(ImGuiCol_Border, col(pal::kAccent, 0.55f));
    if (S.focusInput) {
        ImGui::SetKeyboardFocusHere();
        S.focusInput = false;
    }
    bool enter = ImGui::InputTextWithHint("##input", "type an expression, e.g. 2pi*sin(30)", S.input, sizeof(S.input),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::PopFont();
    if (enter) evaluate();

    ImGui::Dummy(ImVec2(0, 2));
    ImGui::PushFont(S.regular);
    keypad();
    ImGui::PopFont();
    ImGui::EndChild();

    if (showSide) {
        ImGui::SameLine();
        historyPanel();
    }
}

inline void solveNow() {
    std::string eq = trimmed(S.equation);
    std::string var = trimmed(S.unknown);
    S.hasSolution = false;
    S.equationError.clear();
    if (eq.empty()) {
        S.equationError = "Type an equation first.";
        return;
    }
    try {
        S.solution = S.calc.solve(var.empty() ? eq : var + ": " + eq);
        S.hasSolution = true;
    } catch (const CalcError& e) {
        S.equationError = e.what();
    }
}

inline void pill(const std::string& text, unsigned background, unsigned foreground) {
    ImGui::PushFont(S.small);
    ImVec2 ts = ImGui::CalcTextSize(text.c_str());
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(ts.x + 28, ts.y + 14);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col32(background), size.y / 2);
    dl->AddText(ImVec2(pos.x + 14, pos.y + 7), col32(foreground), text.c_str());
    ImGui::Dummy(size);
    ImGui::PopFont();
}

inline void resultCard(const char* id, const std::string& label, const std::string& value, ImVec2 size) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        ImGui::SetClipboardText(value.c_str());
        S.toast = 1.6f;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col32(hovered ? pal::kCardHover : pal::kCard), 18);
    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), col32(hovered ? pal::kAccent : pal::kBorder), 18, 0, 1.5f);

    ImGui::PushFont(S.small);
    dl->AddText(ImVec2(pos.x + 18, pos.y + 14), col32(pal::kAccent), label.c_str());
    ImGui::PopFont();
    ImGui::PushFont(S.medium);
    ImVec2 ts = ImGui::CalcTextSize(value.c_str());
    float x = pos.x + 18;
    if (ts.x > size.x - 36) ImGui::PopFont(), ImGui::PushFont(S.regular), ts = ImGui::CalcTextSize(value.c_str());
    dl->AddText(ImVec2(x, pos.y + size.y - ts.y - 16), col32(pal::kText), value.c_str());
    ImGui::PopFont();
}

inline void solutionView() {
    if (!S.equationError.empty()) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 36);
        float h = ImGui::CalcTextSize(S.equationError.c_str(), nullptr, false, w - 36).y + 28;
        ImGui::PopTextWrapPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), col32(pal::kError, 0.12f), 16);
        dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), col32(pal::kError, 0.6f), 16);
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 18, pos.y + 14));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 36);
        ImGui::TextColored(col(pal::kError), "%s", S.equationError.c_str());
        ImGui::PopTextWrapPos();
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + h + 10));
        return;
    }
    if (!S.hasSolution) {
        ImGui::TextColored(col(pal::kDim), "Solutions appear here. Click a card to copy its value.");
        return;
    }

    using K = Solution::Kind;
    const Solution& s = S.solution;
    std::string kind;
    switch (s.kind) {
        case K::Linear: kind = "linear equation"; break;
        case K::Quadratic: kind = "quadratic equation"; break;
        case K::Polynomial: kind = "polynomial, degree " + std::to_string(s.degree); break;
        case K::Numeric: kind = "numeric search, real roots in [-100, 100]"; break;
        case K::System: kind = "linear system"; break;
        case K::Identity: kind = "identity or underdetermined"; break;
        case K::NoSolution: kind = "inconsistent"; break;
    }
    pill(kind, 0x2B1F63, pal::kSoft);
    ImGui::Dummy(ImVec2(0, 4));

    std::vector<std::pair<std::string, std::string>> cards;
    if (s.kind == K::Identity) cards.push_back({"result", "infinitely many solutions"});
    else if (s.kind == K::NoSolution) cards.push_back({"result", "no solution"});
    else if (s.kind == K::System) {
        for (const auto& [name, value] : s.values) cards.push_back({name, fmtx::number(value)});
    } else {
        std::vector<Complex> roots = s.roots;
        size_t total = roots.size();
        if (total > 24) {
            std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) { return std::abs(a) < std::abs(b); });
            roots.resize(24);
            std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) { return a.real() < b.real(); });
        }
        if (roots.empty()) cards.push_back({"result", "no real solutions found"});
        for (size_t i = 0; i < roots.size(); ++i)
            cards.push_back({s.variable + (roots.size() > 1 ? std::to_string(i + 1) : ""), fmtx::complex(roots[i])});
        if (total > 24) {
            ImGui::TextColored(col(pal::kDim), "%zu solutions in range; showing the 24 closest to 0", total);
        }
    }

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float avail = ImGui::GetContentRegionAvail().x;
    float width = 230;
    int perRow = std::max(1, (int)((avail + spacing) / (width + spacing)));
    width = (avail - spacing * (perRow - 1)) / perRow;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i % perRow != 0) ImGui::SameLine();
        resultCard(("card" + std::to_string(i)).c_str(), cards[i].first, cards[i].second, ImVec2(width, 96));
    }
}

inline void equationsTab() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 26));
    ImGui::BeginChild("equations", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImGui::PushFont(S.medium);
    ImGui::TextColored(col(pal::kText), "Solve equations");
    ImGui::PopFont();
    ImGui::TextColored(col(pal::kDim),
                       "Polynomials up to degree 6, linear systems, or any equation with real roots.");
    ImGui::Dummy(ImVec2(0, 4));

    ImGui::PushFont(S.medium);
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 14));
    ImGui::PushStyleColor(ImGuiCol_Border, col(pal::kAccent, 0.55f));
    bool enter = ImGui::InputTextWithHint("##equation", "x^2-5x+6=0     or     x+y=3, x-y=1", S.equation,
                                          sizeof(S.equation), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::PopFont();

    ImGui::SetNextItemWidth(190);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14);
    ImGui::InputTextWithHint("##unknown", "unknown (optional)", S.unknown, sizeof(S.unknown));
    ImGui::PopStyleVar();
    ImGui::SameLine();

    Key solveKey{"Solve", "", Kind::Equals};
    if (drawKey(solveKey, ImVec2(130, 0)) || enter) solveNow();
    ImGui::SameLine();
    Key clearKey{"Clear", "", Kind::Action};
    if (drawKey(clearKey, ImVec2(110, 0))) {
        S.equation[0] = '\0';
        S.unknown[0] = '\0';
        S.hasSolution = false;
        S.equationError.clear();
    }

    ImGui::Dummy(ImVec2(0, 2));
    ImGui::PushFont(S.small);
    ImGui::TextColored(col(pal::kDim), "EXAMPLES");
    ImGui::PopFont();
    static const char* examples[] = {"x^2-5x+6=0", "x^3-6x^2+11x-6=0", "x^2+1=0", "x+y=3, x-y=1", "cos(x)=x", "2x+3=11"};
    float line = ImGui::GetContentRegionAvail().x;
    float used = 0;
    for (const char* ex : examples) {
        float w = ImGui::CalcTextSize(ex).x + 36;
        if (used > 0 && used + w + 10 > line) used = 0;
        else if (used > 0) ImGui::SameLine();
        Key chip{ex, "", Kind::Function};
        if (drawKey(chip, ImVec2(w, 0))) {
            std::snprintf(S.equation, sizeof(S.equation), "%s", ex);
            solveNow();
        }
        used += w + 10;
    }

    ImGui::Dummy(ImVec2(0, 8));
    solutionView();

    if (S.toast > 0) {
        S.toast -= ImGui::GetIO().DeltaTime;
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextColored(col(pal::kAccentHover), "copied to clipboard");
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

inline void frame() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(ImVec2(p.x + 16, p.y + 18), 14, col32(pal::kAccent), 48);
    dl->AddCircleFilled(ImVec2(p.x + 23, p.y + 13), 12, col32(pal::kBackground), 48);
    ImGui::Dummy(ImVec2(40, 36));
    ImGui::SameLine();
    ImGui::PushFont(S.medium);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
    ImGui::TextColored(col(pal::kText), "MoonCalc");
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 2));

    ImGui::PushFont(S.regular);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(22, 10));
    if (ImGui::BeginTabBar("modes", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Calculator")) {
            ImGui::Dummy(ImVec2(0, 6));
            calculatorTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Equations")) {
            ImGui::Dummy(ImVec2(0, 6));
            equationsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopStyleVar();
    ImGui::PopFont();
    ImGui::End();
}

}
