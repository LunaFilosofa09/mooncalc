#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "calculator.hpp"
#include "format.hpp"
#include "imgui.h"

namespace gui {

namespace pal {
constexpr unsigned kBackground = 0x05030D;
constexpr unsigned kSurface = 0x0B0820;
constexpr unsigned kPanel = 0x130D30;
constexpr unsigned kLine = 0xAD9FFF;
constexpr unsigned kLineDim = 0x4A3D8F;
constexpr unsigned kInk = 0xF1EDFF;
constexpr unsigned kInkMuted = 0x9A8FD0;
constexpr unsigned kPrimary = 0xB9A8FF;
constexpr unsigned kOnPrimary = 0x05030D;
constexpr unsigned kAccent = 0x5A6BFF;
constexpr unsigned kAberration = 0xFF2FA0;
constexpr unsigned kSignal = 0xFF3B8A;
constexpr unsigned kGlow = 0x9678FF;
constexpr unsigned kGrid = 0xAD9FFF;
}

inline ImVec4 col(unsigned rgb, float a = 1.0f) {
    return ImVec4(((rgb >> 16) & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f, (rgb & 255) / 255.0f, a);
}

inline ImU32 col32(unsigned rgb, float a = 1.0f) { return ImGui::ColorConvertFloat4ToU32(col(rgb, a)); }

inline std::string upper(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

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
    int tab = 0;

    char equation[512] = "";
    char unknown[64] = "";
    bool hasSolution = false;
    Solution solution;
    std::string equationError;
    float toast = 0.0f;

    struct MatrixEditor {
        int rows = 3;
        int cols = 3;
        char cell[6][6][24] = {};
    };
    MatrixEditor matA, matB;
    std::string matOp;
    bool matHasResult = false;
    Value matResult;
    std::string matResultText;
    std::string matError;

    ImFont* small = nullptr;
    ImFont* regular = nullptr;
    ImFont* medium = nullptr;
    ImFont* large = nullptr;
    ImFont* title = nullptr;
    bool customFonts = false;
    std::string hexTexture;
};

inline State S;

inline std::string trimmed(const char* s) {
    std::string t = s;
    size_t a = t.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    return t.substr(a, t.find_last_not_of(" \t") - a + 1);
}

inline bool fileExists(const char* path) {
    if (FILE* f = std::fopen(path, "rb")) {
        std::fclose(f);
        return true;
    }
    return false;
}

inline void applyTheme() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 0;
    st.ChildRounding = 0;
    st.FrameRounding = 0;
    st.PopupRounding = 0;
    st.ScrollbarRounding = 0;
    st.GrabRounding = 0;
    st.TabRounding = 0;
    st.WindowBorderSize = 0;
    st.ChildBorderSize = 0;
    st.FrameBorderSize = 1;
    st.WindowPadding = ImVec2(28, 20);
    st.FramePadding = ImVec2(14, 11);
    st.ItemSpacing = ImVec2(8, 8);
    st.ScrollbarSize = 8;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = col(pal::kInk);
    c[ImGuiCol_TextDisabled] = col(pal::kInkMuted);
    c[ImGuiCol_WindowBg] = col(pal::kBackground);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = col(pal::kSurface);
    c[ImGuiCol_Border] = col(pal::kLine);
    c[ImGuiCol_FrameBg] = col(pal::kSurface);
    c[ImGuiCol_FrameBgHovered] = col(pal::kPanel);
    c[ImGuiCol_FrameBgActive] = col(pal::kPanel);
    c[ImGuiCol_Header] = col(pal::kPrimary, 0.14f);
    c[ImGuiCol_HeaderHovered] = col(pal::kPrimary, 0.28f);
    c[ImGuiCol_HeaderActive] = col(pal::kPrimary, 0.42f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = col(pal::kLineDim);
    c[ImGuiCol_ScrollbarGrabHovered] = col(pal::kLine, 0.8f);
    c[ImGuiCol_ScrollbarGrabActive] = col(pal::kPrimary);
    c[ImGuiCol_Separator] = col(pal::kLineDim);
    c[ImGuiCol_TextSelectedBg] = col(pal::kPrimary, 0.35f);
    c[ImGuiCol_NavCursor] = col(pal::kPrimary, 0.0f);
}

inline void init(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    const char* monoCandidates[] = {"/System/Library/Fonts/Menlo.ttc", "/System/Library/Fonts/Monaco.ttf",
                                    "/System/Library/Fonts/SFNSMono.ttf", "/System/Library/Fonts/Courier.ttc"};
    const char* mono = nullptr;
    for (const char* path : monoCandidates)
        if (fileExists(path)) {
            mono = path;
            break;
        }

    const char* hudCustom = fileExists("fonts/VT323-Regular.ttf") ? "fonts/VT323-Regular.ttf" : nullptr;
    const char* titleCustom = fileExists("fonts/Amanojaku.ttf")   ? "fonts/Amanojaku.ttf"
                              : fileExists("fonts/Amanojaku.otf") ? "fonts/Amanojaku.otf"
                                                                   : nullptr;
    S.customFonts = hudCustom || titleCustom;

    auto load = [&](const char* path, float px, int fontNo) -> ImFont* {
        if (!path) return io.Fonts->AddFontDefault();
        ImFontConfig cfg;
        cfg.FontNo = fontNo;
        cfg.OversampleH = 2;
        ImFont* f = io.Fonts->AddFontFromFileTTF(path, px * scale, &cfg);
        return f ? f : io.Fonts->AddFontDefault();
    };

    const char* hud = hudCustom ? hudCustom : mono;
    float k = hudCustom ? 1.35f : 1.0f;
    S.small = load(hud, 13 * k, 0);
    S.regular = load(hud, 16 * k, 0);
    S.medium = load(hud, 22 * k, 0);
    S.large = load(hud, 44 * k, 0);
    S.title = titleCustom ? load(titleCustom, 28, 0) : load(mono, 26, mono && std::strstr(mono, "Menlo") ? 1 : 0);
    io.FontGlobalScale = 1.0f / scale;

    unsigned seed = 0x23DB;
    for (int line = 0; line < 3; ++line) {
        for (int i = 0; i < 18; ++i) {
            seed = seed * 1664525u + 1013904223u;
            char buf[16];
            std::snprintf(buf, sizeof buf, "0x%04X ", (seed >> 8) & 0xFFFF);
            S.hexTexture += buf;
        }
        S.hexTexture += "\n";
    }

    applyTheme();
}

inline void glowText(ImDrawList* dl, ImVec2 pos, ImU32 color, const char* text, int level, bool aberration) {
    float g = level >= 2 ? 2.0f : 1.0f;
    if (level >= 1) {
        const ImVec2 offs[] = {{-g, 0}, {g, 0}, {0, -g}, {0, g}};
        for (const ImVec2& o : offs) dl->AddText(ImVec2(pos.x + o.x, pos.y + o.y), col32(pal::kGlow, level >= 2 ? 0.16f : 0.10f), text);
        if (level >= 2) {
            const ImVec2 wide[] = {{-4, 0}, {4, 0}, {0, -3}, {0, 3}};
            for (const ImVec2& o : wide) dl->AddText(ImVec2(pos.x + o.x, pos.y + o.y), col32(pal::kGlow, 0.07f), text);
        }
    }
    if (aberration) {
        dl->AddText(ImVec2(pos.x - 1, pos.y), col32(pal::kAberration, 0.85f), text);
        dl->AddText(ImVec2(pos.x + 1, pos.y), col32(pal::kAccent, 0.85f), text);
    }
    dl->AddText(pos, color, text);
}

inline void bevelPoints(ImVec2 a, ImVec2 b, float c, ImVec2* p) {
    p[0] = ImVec2(a.x + c, a.y);
    p[1] = ImVec2(b.x, a.y);
    p[2] = ImVec2(b.x, b.y - c);
    p[3] = ImVec2(b.x - c, b.y);
    p[4] = ImVec2(a.x, b.y);
    p[5] = ImVec2(a.x, a.y + c);
}

inline void bevelFrame(ImDrawList* dl, ImVec2 a, ImVec2 b, float c, float fillAlpha, bool glow, bool grid) {
    ImVec2 p[6];
    bevelPoints(a, b, c, p);
    dl->AddConvexPolyFilled(p, 6, col32(pal::kSurface, fillAlpha));

    if (grid) {
        ImU32 gc = col32(pal::kGrid, 0.10f);
        for (float x = a.x + 16; x < b.x; x += 16) {
            float y0 = a.y + std::max(0.0f, c - (x - a.x));
            float y1 = b.y - std::max(0.0f, c - (b.x - x));
            dl->AddLine(ImVec2(x, y0), ImVec2(x, y1), gc);
        }
        for (float y = a.y + 16; y < b.y; y += 16) {
            float x0 = a.x + std::max(0.0f, c - (y - a.y));
            float x1 = b.x - std::max(0.0f, c - (b.y - y));
            dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), gc);
        }
    }
    if (glow) {
        dl->AddPolyline(p, 6, col32(pal::kGlow, 0.10f), ImDrawFlags_Closed, 6.0f);
        dl->AddPolyline(p, 6, col32(pal::kGlow, 0.22f), ImDrawFlags_Closed, 3.0f);
    }
    dl->AddPolyline(p, 6, col32(pal::kLine), ImDrawFlags_Closed, 1.0f);
}

inline void brackets(ImDrawList* dl, ImVec2 a, ImVec2 b, float len, ImU32 color) {
    dl->AddLine(a, ImVec2(a.x + len, a.y), color);
    dl->AddLine(a, ImVec2(a.x, a.y + len), color);
    dl->AddLine(ImVec2(b.x, a.y), ImVec2(b.x - len, a.y), color);
    dl->AddLine(ImVec2(b.x, a.y), ImVec2(b.x, a.y + len), color);
    dl->AddLine(ImVec2(a.x, b.y), ImVec2(a.x + len, b.y), color);
    dl->AddLine(ImVec2(a.x, b.y), ImVec2(a.x, b.y - len), color);
    dl->AddLine(b, ImVec2(b.x - len, b.y), color);
    dl->AddLine(b, ImVec2(b.x, b.y - len), color);
}

inline void beginPanel(const char* id, ImVec2 size, float bevel, bool grid, ImVec2 padding) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (size.x <= 0) size.x += avail.x;
    if (size.y <= 0) size.y += avail.y;
    bevelFrame(ImGui::GetWindowDrawList(), p, ImVec2(p.x + size.x, p.y + size.y), bevel, 0.88f, true, grid);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding);
}

inline void endPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

inline void textRight(const std::string& text, ImU32 color) {
    float w = ImGui::CalcTextSize(text.c_str()).x;
    float avail = ImGui::GetContentRegionAvail().x;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (w < avail) pos.x += avail - w;
    ImGui::GetWindowDrawList()->AddText(pos, color, text.c_str());
    ImGui::Dummy(ImVec2(avail, ImGui::GetTextLineHeight()));
}

inline void label(const char* text, ImU32 color) {
    ImGui::PushFont(S.small);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(upper(text).c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

inline void evaluate() {
    std::string text = trimmed(S.input);
    if (text.empty()) return;

    HistoryEntry entry{text, "", false};
    if (text == "deg" || text == "rad") {
        S.calc.degrees = text == "deg";
        entry.result = S.calc.degrees ? "DEGREES MODE" : "RADIANS MODE";
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

inline bool ghostButton(const char* text, ImVec2 size, Kind kind) {
    if (size.y <= 0) size.y = ImGui::GetFontSize() + 22;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(text, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    ImVec2 q(p.x + size.x, p.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    unsigned lineColor = pal::kLineDim, textColor = pal::kInk;
    float fillAlpha = 0.0f;
    unsigned fill = pal::kPanel;
    switch (kind) {
        case Kind::Number: break;
        case Kind::Operator: lineColor = pal::kLine; textColor = pal::kPrimary; break;
        case Kind::Function: textColor = pal::kInkMuted; break;
        case Kind::Action: lineColor = pal::kLine; textColor = pal::kInkMuted; break;
        case Kind::Mode: textColor = pal::kAccent; lineColor = pal::kAccent; break;
        case Kind::Equals:
            fill = pal::kPrimary;
            fillAlpha = 1.0f;
            lineColor = pal::kPrimary;
            textColor = pal::kOnPrimary;
            break;
    }
    if (hovered) {
        if (kind != Kind::Equals) {
            lineColor = pal::kLine;
            textColor = pal::kInk;
            fillAlpha = 0.9f;
        } else {
            fill = pal::kInk;
        }
    }
    if (held) {
        fill = kind == Kind::Equals ? pal::kLine : pal::kLineDim;
        fillAlpha = 1.0f;
    }

    if (kind == Kind::Equals || hovered) {
        dl->AddRect(ImVec2(p.x - 2, p.y - 2), ImVec2(q.x + 2, q.y + 2), col32(pal::kGlow, kind == Kind::Equals ? 0.30f : 0.20f), 0, 0, 3.0f);
    }
    if (fillAlpha > 0) dl->AddRectFilled(p, q, col32(fill, fillAlpha));
    dl->AddRect(p, q, col32(lineColor), 0, 0, 1.0f);

    std::string shown = upper(text);
    ImVec2 ts = ImGui::CalcTextSize(shown.c_str());
    ImVec2 tp(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f);
    if (kind != Kind::Equals && (hovered || kind == Kind::Operator)) {
        dl->AddText(ImVec2(tp.x, tp.y), col32(pal::kGlow, 0.35f), shown.c_str());
        dl->AddText(ImVec2(tp.x - 0.5f, tp.y), col32(textColor), shown.c_str());
    }
    dl->AddText(tp, col32(textColor), shown.c_str());
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
    beginPanel("display", ImVec2(0, 150), 16, true, ImVec2(24, 16));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::PushFont(S.small);
    ImVec2 top = ImGui::GetCursorScreenPos();
    float avail = ImGui::GetContentRegionAvail().x;
    dl->AddText(top, col32(pal::kInk), "SUBJECT A-01");
    std::string mode = std::string("MODE: ") + (S.calc.degrees ? "DEG" : "RAD");
    float mw = ImGui::CalcTextSize(mode.c_str()).x;
    dl->AddText(ImVec2(top.x + avail - mw, top.y), col32(pal::kPrimary), mode.c_str());
    std::string sub = S.subtitle.empty() ? "AWAITING INPUT" : upper(S.subtitle);
    float sw = ImGui::CalcTextSize(sub.c_str()).x;
    ImVec2 subPos(top.x + std::max(0.0f, (avail - sw) * 0.5f), top.y);
    if (sw < avail - mw - 140) dl->AddText(subPos, col32(pal::kInkMuted), sub.c_str());
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(avail, 14));

    ImFont* fonts[] = {S.large, S.medium, S.regular};
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
    ImVec2 ts = ImGui::CalcTextSize(S.display.c_str());
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 color = col32(S.displayError ? pal::kSignal : pal::kPrimary);
    if (ts.x <= avail) {
        pos.x += avail - ts.x;
        glowText(dl, pos, color, S.display.c_str(), 2, !S.displayError);
        ImGui::Dummy(ImVec2(avail, ts.y));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(S.display.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
    ImGui::PopFont();
    endPanel();
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
         {"> EXEC", "=", Kind::Equals}, {"", "", Kind::Number}, {"", "", Kind::Number}, {"", "", Kind::Number}},
    };

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float avail = ImGui::GetContentRegionAvail().x;
    float bw = (avail - spacing * 6) / 7.0f;
    float bh = 48;

    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            const Key& key = rows[r][c];
            if (r == 5 && c >= 4) break;
            if (c > 0) ImGui::SameLine();
            ImGui::PushID(r * 10 + c);
            ImVec2 size(bw, bh);
            std::string modeLabel;
            const char* shown = key.label;
            if (key.kind == Kind::Mode) {
                modeLabel = S.calc.degrees ? "DEG" : "RAD";
                shown = modeLabel.c_str();
            }
            if (key.kind == Kind::Equals) size.x = bw * 4 + spacing * 3;
            if (ghostButton(shown, size, key.kind)) handleKey(key);
            ImGui::PopID();
        }
    }
}

inline void historyPanel() {
    beginPanel("side", ImVec2(0, 0), 16, false, ImVec2(20, 16));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    label("LOG", col32(pal::kInk));
    ImGui::SameLine();
    char count[24];
    std::snprintf(count, sizeof count, "%04zu", S.history.size());
    ImGui::PushFont(S.small);
    ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kPrimary));
    float cw = ImGui::CalcTextSize(count).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - cw);
    ImGui::TextUnformatted(count);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImVec2 lp = ImGui::GetCursorScreenPos();
    dl->AddLine(lp, ImVec2(lp.x + ImGui::GetContentRegionAvail().x, lp.y), col32(pal::kLineDim));
    ImGui::Dummy(ImVec2(0, 4));

    if (S.history.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
        ImGui::TextUnformatted("----");
        ImGui::PopStyleColor();
    }

    for (int i = (int)S.history.size() - 1; i >= 0; --i) {
        const HistoryEntry& e = S.history[i];
        ImGui::PushID(i);
        char idx[16];
        std::snprintf(idx, sizeof idx, "%04d", i + 1);
        ImGui::PushFont(S.small);
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
        ImGui::Text("%s  %s", idx, upper(e.expression).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PushStyleColor(ImGuiCol_Text, col(e.error ? pal::kInkMuted : pal::kPrimary));
        std::string line = (e.error ? "ERR " : "= ") + (e.error ? upper(e.result) : e.result);
        if (ImGui::Selectable(line.c_str(), false, 0)) {
            if (!e.error) appendInput(e.result.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0, 8));
    ImVec2 rp = ImGui::GetCursorScreenPos();
    dl->AddLine(rp, ImVec2(rp.x + ImGui::GetContentRegionAvail().x, rp.y), col32(pal::kLineDim));
    ImGui::Dummy(ImVec2(0, 4));
    label("REGISTERS", col32(pal::kInk));
    int shown = 0;
    for (const auto& [name, value] : S.calc.variables()) {
        if (name == "pi" || name == "e" || name == "i" || name == "inf") continue;
        std::string text = upper(name) + " = " + fmtx::inlineValue(value);
        ImGui::PushID(name.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kPrimary));
        if (ImGui::Selectable(text.c_str(), false, 0)) appendInput(name.c_str());
        ImGui::PopStyleColor();
        ImGui::PopID();
        ++shown;
    }
    if (!shown) {
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
        ImGui::TextUnformatted("----");
        ImGui::PopStyleColor();
    }
    endPanel();
}

inline void inputField(const char* id, const char* hint, char* buffer, size_t size, float width, bool* enter) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushFont(S.medium);
    ImGui::SetNextItemWidth(width);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 12));
    ImGui::PushStyleColor(ImGuiCol_Border, col(pal::kLine));
    bool e = ImGui::InputTextWithHint(id, hint, buffer, size, ImGuiInputTextFlags_EnterReturnsTrue);
    ImVec2 q = ImGui::GetItemRectMax();
    bool active = ImGui::IsItemActive();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopFont();
    if (enter) *enter = e;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRect(ImVec2(p.x - 2, p.y - 2), ImVec2(q.x + 2, q.y + 2), col32(pal::kGlow, active ? 0.30f : 0.14f), 0, 0, 3.0f);
}

inline void calculatorTab() {
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float total = ImGui::GetContentRegionAvail().x;
    float side = std::clamp(total * 0.28f, 240.0f, 340.0f);
    bool showSide = total > 900;
    float mainWidth = showSide ? total - side - spacing - 8 : total;

    ImGui::BeginChild("main", ImVec2(mainWidth, 0), ImGuiChildFlags_None);
    displayCard();
    ImGui::Dummy(ImVec2(0, 2));

    ImGui::PushFont(S.medium);
    ImVec2 pp = ImGui::GetCursorScreenPos();
    float promptH = ImGui::GetFontSize() + 24;
    ImGui::GetWindowDrawList()->AddText(ImVec2(pp.x + 2, pp.y + (promptH - ImGui::GetFontSize()) * 0.5f),
                                        col32(pal::kPrimary), ">");
    ImGui::Dummy(ImVec2(22, promptH));
    ImGui::PopFont();
    ImGui::SameLine(0, 4);

    if (S.focusInput) {
        ImGui::SetKeyboardFocusHere();
        S.focusInput = false;
    }
    bool enter = false;
    inputField("##input", "ENTER EXPRESSION_", S.input, sizeof(S.input), -1, &enter);
    if (enter) evaluate();

    ImGui::Dummy(ImVec2(0, 2));
    ImGui::PushFont(S.regular);
    keypad();
    ImGui::PopFont();
    ImGui::EndChild();

    if (showSide) {
        ImGui::SameLine(0, spacing + 8);
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

inline void tag(const std::string& text) {
    ImGui::PushFont(S.small);
    std::string shown = "[ " + upper(text) + " ]";
    ImVec2 ts = ImGui::CalcTextSize(shown.c_str());
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText(pos, col32(pal::kPrimary), shown.c_str());
    ImGui::Dummy(ImVec2(ts.x, ts.y + 4));
    ImGui::PopFont();
}

inline void resultCard(const char* id, const std::string& name, const std::string& value, ImVec2 size) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        ImGui::SetClipboardText(value.c_str());
        S.toast = 1.6f;
    }
    ImVec2 q(pos.x + size.x, pos.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, q, col32(hovered ? pal::kPanel : pal::kSurface, 0.92f));
    dl->AddRect(pos, q, col32(hovered ? pal::kLine : pal::kLineDim));
    brackets(dl, ImVec2(pos.x - 3, pos.y - 3), ImVec2(q.x + 3, q.y + 3), 10, col32(pal::kLine));
    if (hovered) dl->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(q.x + 1, q.y + 1), col32(pal::kGlow, 0.25f), 0, 0, 3.0f);

    ImGui::PushFont(S.small);
    std::string head = upper(name);
    dl->AddText(ImVec2(pos.x + 16, pos.y + 12), col32(pal::kInk), head.c_str());
    ImGui::PopFont();
    ImGui::PushFont(S.medium);
    ImVec2 ts = ImGui::CalcTextSize(value.c_str());
    if (ts.x > size.x - 32) {
        ImGui::PopFont();
        ImGui::PushFont(S.regular);
        ts = ImGui::CalcTextSize(value.c_str());
    }
    glowText(dl, ImVec2(pos.x + 16, q.y - ts.y - 14), col32(pal::kPrimary), value.c_str(), 1, false);
    ImGui::PopFont();
}

inline void solutionView() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!S.equationError.empty()) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        std::string msg = "ERR  " + upper(S.equationError);
        float h = ImGui::CalcTextSize(msg.c_str(), nullptr, false, w - 36).y + 28;
        dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), col32(pal::kSignal, 0.08f));
        dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), col32(pal::kSignal));
        dl->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(pos.x + w + 1, pos.y + h + 1), col32(pal::kSignal, 0.22f), 0, 0, 3.0f);
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 18, pos.y + 14));
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kSignal));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 36);
        ImGui::TextUnformatted(msg.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + h + 10));
        return;
    }
    if (!S.hasSolution) {
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
        ImGui::TextUnformatted("----  AWAITING INPUT. CLICK A CARD TO COPY ITS VALUE.");
        ImGui::PopStyleColor();
        return;
    }

    using K = Solution::Kind;
    const Solution& s = S.solution;
    std::string kind;
    switch (s.kind) {
        case K::Linear: kind = "linear equation"; break;
        case K::Quadratic: kind = "quadratic equation"; break;
        case K::Polynomial: kind = "polynomial // deg " + std::to_string(s.degree); break;
        case K::Numeric: kind = "numeric search // real roots in [-100, 100]"; break;
        case K::System: kind = "linear system"; break;
        case K::Identity: kind = "identity or underdetermined"; break;
        case K::NoSolution: kind = "inconsistent"; break;
    }
    tag(kind);
    ImGui::Dummy(ImVec2(0, 6));

    std::vector<std::pair<std::string, std::string>> cards;
    if (s.kind == K::Identity) cards.push_back({"result", "infinitely many solutions"});
    else if (s.kind == K::NoSolution) cards.push_back({"result", "no solution"});
    else if (s.kind == K::System) {
        for (const auto& [name, value] : s.values) cards.push_back({"var " + name, fmtx::number(value)});
    } else {
        std::vector<Complex> roots = s.roots;
        size_t total = roots.size();
        if (total > 24) {
            std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) { return std::abs(a) < std::abs(b); });
            roots.resize(24);
            std::sort(roots.begin(), roots.end(), [](const Complex& a, const Complex& b) { return a.real() < b.real(); });
        }
        if (roots.empty()) cards.push_back({"result", "no real solutions found"});
        for (size_t i = 0; i < roots.size(); ++i) {
            char buf[32];
            std::snprintf(buf, sizeof buf, "root %02zu", i + 1);
            cards.push_back({buf, s.variable + " = " + fmtx::complex(roots[i])});
        }
        if (total > 24) {
            ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
            ImGui::Text("%zu SOLUTIONS IN RANGE. SHOWING THE 24 CLOSEST TO 0.", total);
            ImGui::PopStyleColor();
        }
    }

    float spacing = ImGui::GetStyle().ItemSpacing.x + 6;
    float avail = ImGui::GetContentRegionAvail().x - 6;
    float width = 250;
    int perRow = std::max(1, (int)((avail + spacing) / (width + spacing)));
    width = (avail - spacing * (perRow - 1)) / perRow;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i % perRow != 0) ImGui::SameLine(0, spacing);
        else if (i > 0) ImGui::Dummy(ImVec2(0, 2));
        resultCard(("card" + std::to_string(i)).c_str(), cards[i].first, cards[i].second, ImVec2(width, 92));
    }
}

inline void equationsTab() {
    beginPanel("equations", ImVec2(0, 0), 18, true, ImVec2(30, 24));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::PushFont(S.title);
    ImVec2 tp = ImGui::GetCursorScreenPos();
    glowText(dl, tp, col32(pal::kPrimary), "EQUATION SOLVER", 1, true);
    ImGui::Dummy(ImVec2(10, ImGui::GetTextLineHeight()));
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
    ImGui::TextUnformatted("POLYNOMIALS UP TO DEGREE 6 // LINEAR SYSTEMS // ANY EQUATION WITH REAL ROOTS");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));

    bool enter = false;
    inputField("##equation", "x^2-5x+6=0     or     x+y=3, x-y=1", S.equation, sizeof(S.equation), -1, &enter);

    ImGui::PushFont(S.regular);
    ImGui::SetNextItemWidth(210);
    ImGui::InputTextWithHint("##unknown", "UNKNOWN (OPTIONAL)", S.unknown, sizeof(S.unknown));
    ImGui::SameLine();
    if (ghostButton("> SOLVE", ImVec2(150, 0), Kind::Equals) || enter) solveNow();
    ImGui::SameLine();
    if (ghostButton("> RESET", ImVec2(130, 0), Kind::Action)) {
        S.equation[0] = '\0';
        S.unknown[0] = '\0';
        S.hasSolution = false;
        S.equationError.clear();
    }

    ImGui::Dummy(ImVec2(0, 4));
    ImGui::PopFont();
    label("PRESETS", col32(pal::kInk));
    ImGui::PushFont(S.regular);
    static const char* examples[] = {"x^2-5x+6=0", "x^3-6x^2+11x-6=0", "x^2+1=0", "x+y=3, x-y=1", "cos(x)=x", "2x+3=11"};
    float line = ImGui::GetContentRegionAvail().x;
    float used = 0;
    for (const char* ex : examples) {
        float w = ImGui::CalcTextSize(ex).x + 36;
        if (used > 0 && used + w + 8 > line) used = 0;
        else if (used > 0) ImGui::SameLine();
        if (ghostButton(ex, ImVec2(w, 0), Kind::Function)) {
            std::snprintf(S.equation, sizeof(S.equation), "%s", ex);
            solveNow();
        }
        used += w + 8;
    }
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::PushFont(S.regular);
    solutionView();
    ImGui::PopFont();

    if (S.toast > 0) {
        S.toast -= ImGui::GetIO().DeltaTime;
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kPrimary));
        ImGui::TextUnformatted("STATUS: COPIED TO CLIPBOARD");
        ImGui::PopStyleColor();
    }
    endPanel();
}

inline std::string matrixLiteral(const State::MatrixEditor& m) {
    std::string s = "[";
    for (int i = 0; i < m.rows; ++i) {
        if (i) s += ";";
        for (int j = 0; j < m.cols; ++j) {
            std::string cell = trimmed(m.cell[i][j]);
            s += (j ? "," : "") + (cell.empty() ? std::string("0") : cell);
        }
    }
    return s + "]";
}

inline void runMatrixOp(const std::string& expression) {
    S.matHasResult = false;
    S.matError.clear();
    S.matOp = expression;
    try {
        S.calc.execute("A = " + matrixLiteral(S.matA));
        S.calc.execute("B = " + matrixLiteral(S.matB));
        Outcome out = S.calc.execute(expression);
        S.matResult = out.value;
        S.matResultText = fmtx::inlineValue(out.value);
        S.matHasResult = true;
    } catch (const CalcError& e) {
        S.matError = e.what();
    }
}

inline void useResultAs(State::MatrixEditor& target) {
    if (!S.matHasResult) return;
    std::memset(target.cell, 0, sizeof target.cell);
    if (!S.matResult.isMatrix) {
        target.rows = target.cols = 1;
        std::snprintf(target.cell[0][0], sizeof target.cell[0][0], "%s", fmtx::complex(S.matResult.scalar).c_str());
        return;
    }
    const Matrix& m = S.matResult.matrix;
    target.rows = (int)std::min<size_t>(m.rows, 6);
    target.cols = (int)std::min<size_t>(m.cols, 6);
    for (int i = 0; i < target.rows; ++i)
        for (int j = 0; j < target.cols; ++j)
            std::snprintf(target.cell[i][j], sizeof target.cell[i][j], "%s", fmtx::complex(m.at(i, j)).c_str());
}

inline void matrixEditor(const char* id, const char* title, State::MatrixEditor& m, float width, float height) {
    beginPanel(id, ImVec2(width, height), 14, false, ImVec2(20, 16));
    char head[48];
    std::snprintf(head, sizeof head, "%s  [%dx%d]", title, m.rows, m.cols);
    label(head, col32(pal::kInk));
    ImGui::Dummy(ImVec2(0, 2));

    ImGui::PushFont(S.small);
    float bh = 30;
    struct Control {
        const char* text;
        int kind;
    } controls[] = {{"ROW -", 0}, {"ROW +", 1}, {"COL -", 2}, {"COL +", 3}, {"IDENT", 4}, {"CLEAR", 5}};
    for (int k = 0; k < 6; ++k) {
        if (k) ImGui::SameLine(0, 6);
        ImGui::PushID(k);
        if (ghostButton(controls[k].text, ImVec2(64, bh), Kind::Function)) {
            switch (controls[k].kind) {
                case 0: m.rows = std::max(1, m.rows - 1); break;
                case 1: m.rows = std::min(6, m.rows + 1); break;
                case 2: m.cols = std::max(1, m.cols - 1); break;
                case 3: m.cols = std::min(6, m.cols + 1); break;
                case 4:
                    std::memset(m.cell, 0, sizeof m.cell);
                    for (int i = 0; i < std::min(m.rows, m.cols); ++i) std::snprintf(m.cell[i][i], sizeof m.cell[i][i], "1");
                    break;
                default: std::memset(m.cell, 0, sizeof m.cell); break;
            }
        }
        ImGui::PopID();
    }
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 4));

    ImGui::PushFont(S.regular);
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float cell = std::min(86.0f, (ImGui::GetContentRegionAvail().x - spacing * (m.cols - 1)) / m.cols);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 9));
    for (int i = 0; i < m.rows; ++i) {
        for (int j = 0; j < m.cols; ++j) {
            if (j) ImGui::SameLine();
            ImGui::PushID(i * 10 + j);
            ImGui::SetNextItemWidth(cell);
            ImGui::InputText("##c", m.cell[i][j], sizeof m.cell[i][j]);
            ImGui::PopID();
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopFont();
    endPanel();
}

inline void resultGrid() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!S.matError.empty()) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        std::string msg = "ERR  " + upper(S.matError);
        float h = ImGui::CalcTextSize(msg.c_str(), nullptr, false, w - 36).y + 24;
        dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), col32(pal::kSignal, 0.08f));
        dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), col32(pal::kSignal));
        dl->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(pos.x + w + 1, pos.y + h + 1), col32(pal::kSignal, 0.22f), 0, 0, 3.0f);
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 16, pos.y + 12));
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kSignal));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 32);
        ImGui::TextUnformatted(msg.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + h + 8));
        return;
    }
    if (!S.matHasResult) {
        ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
        ImGui::TextUnformatted("----  PICK AN OPERATION. A AND B ARE STORED AS VARIABLES.");
        ImGui::PopStyleColor();
        return;
    }

    if (!S.matResult.isMatrix) {
        std::string text = fmtx::complex(S.matResult.scalar);
        ImGui::PushFont(S.large);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        glowText(dl, pos, col32(pal::kPrimary), text.c_str(), 2, true);
        ImGui::Dummy(ImVec2(10, ImGui::GetTextLineHeight()));
        ImGui::PopFont();
        return;
    }

    const Matrix& m = S.matResult.matrix;
    ImGui::PushFont(S.regular);
    float avail = ImGui::GetContentRegionAvail().x;
    float cell = std::min(130.0f, (avail - 6.0f * (m.cols - 1)) / (float)m.cols);
    float cellH = ImGui::GetFontSize() + 18;
    ImVec2 origin = ImGui::GetCursorScreenPos();
    for (size_t i = 0; i < m.rows; ++i) {
        for (size_t j = 0; j < m.cols; ++j) {
            ImVec2 p(origin.x + (float)j * (cell + 6), origin.y + (float)i * (cellH + 6));
            ImVec2 q(p.x + cell, p.y + cellH);
            std::string text = fmtx::complex(numeric::roundSig(m.at(i, j), 8));
            dl->AddRectFilled(p, q, col32(pal::kSurface, 0.92f));
            dl->AddRect(p, q, col32(pal::kLineDim));
            ImVec2 ts = ImGui::CalcTextSize(text.c_str());
            ImVec2 tp(p.x + std::max(6.0f, (cell - ts.x) * 0.5f), p.y + (cellH - ts.y) * 0.5f);
            dl->PushClipRect(p, q, true);
            glowText(dl, tp, col32(pal::kPrimary), text.c_str(), 1, false);
            dl->PopClipRect();
        }
    }
    brackets(dl, ImVec2(origin.x - 6, origin.y - 4),
             ImVec2(origin.x + (float)m.cols * (cell + 6) - 6 + 6, origin.y + (float)m.rows * (cellH + 6) - 6 + 4), 12,
             col32(pal::kLine));
    ImGui::Dummy(ImVec2(avail, (float)m.rows * (cellH + 6) + 6));
    ImGui::PopFont();
}

inline void matricesTab() {
    ImGui::BeginChild("matrices", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::PushFont(S.title);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    glowText(dl, ImGui::GetCursorScreenPos(), col32(pal::kPrimary), "MATRIX LAB", 1, true);
    ImGui::Dummy(ImVec2(10, ImGui::GetTextLineHeight()));
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, col(pal::kInkMuted));
    ImGui::TextUnformatted("EDIT A AND B // CELLS ACCEPT EXPRESSIONS (PI, SQRT(2), 1/3)");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 2));

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float width = (ImGui::GetContentRegionAvail().x - spacing) / 2;
    float height = 112 + (float)std::max(S.matA.rows, S.matB.rows) * 46;
    matrixEditor("matrixA", "MATRIX A", S.matA, width, height);
    ImGui::SameLine();
    matrixEditor("matrixB", "MATRIX B", S.matB, width, height);

    beginPanel("ops", ImVec2(0, 130), 14, false, ImVec2(20, 14));
    ImGui::PushFont(S.small);
    ImGui::PopFont();
    label("OPERATIONS", col32(pal::kInk));
    ImGui::PushFont(S.regular);
    struct Op {
        const char* text;
        const char* expr;
    } ops[] = {{"A+B", "A+B"}, {"A-B", "A-B"}, {"A*B", "A*B"}, {"A.*B", "A.*B"}, {"A\\B", "A\\B"}, {"DET A", "det(A)"},
               {"INV A", "inv(A)"}, {"TRANSPOSE A", "transpose(A)"}, {"RANK A", "rank(A)"}, {"TRACE A", "trace(A)"},
               {"RREF A", "rref(A)"}, {"NULLSPACE A", "nullspace(A)"}, {"EXPM A", "expm(A)"}, {"COND A", "cond(A)"},
               {"NORM A", "norm(A)"}, {"A^2", "A^2"}};
    float line = ImGui::GetContentRegionAvail().x;
    float used = 0;
    for (const Op& op : ops) {
        float w = ImGui::CalcTextSize(op.text).x + 30;
        if (used > 0 && used + w + 8 > line) used = 0;
        else if (used > 0) ImGui::SameLine();
        if (ghostButton(op.text, ImVec2(w, 36), Kind::Operator)) runMatrixOp(op.expr);
        used += w + 8;
    }
    ImGui::PopFont();
    endPanel();

    beginPanel("matrixResult", ImVec2(0, std::max(250.0f, ImGui::GetContentRegionAvail().y)), 14, true, ImVec2(22, 16));
    std::string head = S.matOp.empty() ? "RESULT" : "RESULT // " + upper(S.matOp);
    label(head.c_str(), col32(pal::kInk));
    ImGui::Dummy(ImVec2(0, 4));
    resultGrid();
    if (S.matHasResult) {
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(S.small);
        if (ghostButton("> USE AS A", ImVec2(120, 30), Kind::Function)) useResultAs(S.matA);
        ImGui::SameLine();
        if (ghostButton("> USE AS B", ImVec2(120, 30), Kind::Function)) useResultAs(S.matB);
        ImGui::SameLine();
        if (ghostButton("> COPY", ImVec2(100, 30), Kind::Function)) ImGui::SetClipboardText(S.matResultText.c_str());
        ImGui::PopFont();
    }
    endPanel();
    ImGui::EndChild();
}

inline bool tabButton(const char* text, bool active, float width) {
    float height = 38;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(text, ImVec2(width, height));
    bool hovered = ImGui::IsItemHovered();
    ImVec2 q(p.x + width, p.y + height);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (active) {
        dl->AddRect(ImVec2(p.x - 2, p.y - 2), ImVec2(q.x + 2, q.y + 2), col32(pal::kGlow, 0.30f), 0, 0, 3.0f);
        dl->AddRectFilled(p, q, col32(pal::kPrimary));
    } else {
        if (hovered) dl->AddRectFilled(p, q, col32(pal::kPanel, 0.9f));
        dl->AddRect(p, q, col32(hovered ? pal::kLine : pal::kLineDim));
    }
    ImVec2 ts = ImGui::CalcTextSize(text);
    ImU32 tc = col32(active ? pal::kOnPrimary : (hovered ? pal::kInk : pal::kInkMuted));
    dl->AddText(ImVec2(p.x + (width - ts.x) * 0.5f, p.y + (height - ts.y) * 0.5f), tc, text);
    return pressed;
}

inline void header() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;

    brackets(dl, ImVec2(p.x, p.y + 2), ImVec2(p.x + 40, p.y + 42), 9, col32(pal::kLine));
    dl->AddRectFilled(ImVec2(p.x + 17, p.y + 19), ImVec2(p.x + 23, p.y + 25), col32(pal::kPrimary));
    dl->AddRect(ImVec2(p.x + 14, p.y + 16), ImVec2(p.x + 26, p.y + 28), col32(pal::kGlow, 0.30f), 0, 0, 2.0f);

    ImGui::PushFont(S.title);
    glowText(dl, ImVec2(p.x + 56, p.y - 2), col32(pal::kPrimary), "MOONCALC", 1, true);
    ImGui::PopFont();

    ImGui::PushFont(S.small);
    bool blink = std::fmod(ImGui::GetTime(), 1.0) < 0.5;
    dl->AddText(ImVec2(p.x + 56, p.y + 30), col32(pal::kInkMuted), "SIGNAL // SCIENTIFIC CALCULATOR");
    float tw = ImGui::CalcTextSize("SIGNAL // SCIENTIFIC CALCULATOR ").x;
    if (blink) dl->AddRectFilled(ImVec2(p.x + 56 + tw, p.y + 31), ImVec2(p.x + 56 + tw + 7, p.y + 42), col32(pal::kPrimary));

    if (width > 860) {
        ImVec2 hp(p.x + width * 0.42f, p.y);
        dl->PushClipRect(hp, ImVec2(p.x + width - 150, p.y + 48), true);
        dl->AddText(hp, col32(pal::kInkMuted, 0.30f), S.hexTexture.c_str());
        dl->PopClipRect();
    }

    char logCount[16];
    std::snprintf(logCount, sizeof logCount, "%04zu", S.history.size());
    struct Row {
        const char* name;
        std::string value;
    } rows[] = {{"STATUS", "OK"}, {"MODE", S.calc.degrees ? "DEG" : "RAD"}, {"LOG", logCount}};
    float rowH = 14;
    for (int i = 0; i < 3; ++i) {
        std::string left = std::string(rows[i].name) + ":";
        float vw = ImGui::CalcTextSize(rows[i].value.c_str()).x;
        float y = p.y + 2 + i * rowH;
        dl->AddText(ImVec2(p.x + width - 110, y), col32(pal::kInk), left.c_str());
        glowText(dl, ImVec2(p.x + width - vw, y), col32(pal::kPrimary), rows[i].value.c_str(), 1, false);
    }
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(width, 54));
}

inline void overlay(ImGuiViewport* vp) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 a = vp->Pos, b(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y);

    ImU32 scan = col32(0x000000, 0.28f);
    for (float y = a.y; y < b.y; y += 3.0f) dl->AddRectFilled(ImVec2(a.x, y + 2), ImVec2(b.x, y + 3), scan);

    ImU32 dark = IM_COL32(0, 0, 0, 120), clear = IM_COL32(0, 0, 0, 0);
    float v = 150;
    dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + v), dark, dark, clear, clear);
    dl->AddRectFilledMultiColor(ImVec2(a.x, b.y - v), b, clear, clear, dark, dark);
    dl->AddRectFilledMultiColor(a, ImVec2(a.x + v, b.y), dark, clear, clear, dark);
    dl->AddRectFilledMultiColor(ImVec2(b.x - v, a.y), b, clear, dark, dark, clear);
}

inline void frame() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushFont(S.regular);
    header();

    float tabW = 190;
    ImVec2 rowStart = ImGui::GetCursorScreenPos();
    float full = ImGui::GetContentRegionAvail().x;
    if (tabButton("01  CALCULATOR", S.tab == 0, tabW)) S.tab = 0;
    ImGui::SameLine(0, 6);
    if (tabButton("02  EQUATIONS", S.tab == 1, tabW)) S.tab = 1;
    ImGui::SameLine(0, 6);
    if (tabButton("03  MATRICES", S.tab == 2, tabW)) S.tab = 2;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(rowStart.x + 3 * tabW + 12, rowStart.y + 38), ImVec2(rowStart.x + full, rowStart.y + 38),
                                        col32(pal::kLineDim));
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::PopFont();

    if (S.tab == 0) calculatorTab();
    else if (S.tab == 1) equationsTab();
    else matricesTab();

    ImGui::End();
    overlay(vp);
}

}
