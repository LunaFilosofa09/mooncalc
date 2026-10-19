#pragma once

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

class LineEditor {
public:
    bool readLine(const std::string& prompt, size_t promptWidth, std::string& out) {
        if (!isatty(STDIN_FILENO)) {
            std::cout << prompt << std::flush;
            return static_cast<bool>(std::getline(std::cin, out));
        }

        termios original{};
        tcgetattr(STDIN_FILENO, &original);
        termios raw = original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        std::string buf, draft;
        size_t cursor = 0;
        size_t histIndex = history_.size();
        bool eof = false;

        auto redraw = [&] {
            std::cout << "\r" << prompt << buf << "\033[K\r";
            size_t cols = promptWidth + codepoints(buf.substr(0, cursor));
            if (cols > 0) std::cout << "\033[" << cols << "C";
            std::cout << std::flush;
        };
        redraw();

        for (;;) {
            unsigned char c;
            if (read(STDIN_FILENO, &c, 1) <= 0) {
                eof = true;
                break;
            }
            if (c == '\n' || c == '\r') {
                cursor = buf.size();
                redraw();
                std::cout << "\n" << std::flush;
                break;
            }
            if (c == 4) {
                if (buf.empty()) {
                    eof = true;
                    std::cout << "\n" << std::flush;
                    break;
                }
                if (cursor < buf.size()) buf.erase(cursor, nextLength(buf, cursor));
            } else if (c == 127 || c == 8) {
                if (cursor > 0) {
                    size_t start = prevIndex(buf, cursor);
                    buf.erase(start, cursor - start);
                    cursor = start;
                }
            } else if (c == 1) {
                cursor = 0;
            } else if (c == 5) {
                cursor = buf.size();
            } else if (c == 21) {
                buf.erase(0, cursor);
                cursor = 0;
            } else if (c == 11) {
                buf.erase(cursor);
            } else if (c == 27) {
                handleEscape(buf, cursor, histIndex, draft);
            } else if (c >= 32) {
                buf.insert(cursor, 1, (char)c);
                ++cursor;
            }
            redraw();
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &original);
        if (eof) return false;
        out = buf;
        if (!buf.empty() && (history_.empty() || history_.back() != buf)) history_.push_back(buf);
        return true;
    }

private:
    std::vector<std::string> history_;

    static size_t codepoints(const std::string& s) {
        size_t n = 0;
        for (unsigned char ch : s)
            if ((ch & 0xC0) != 0x80) ++n;
        return n;
    }

    static size_t prevIndex(const std::string& s, size_t i) {
        if (i == 0) return 0;
        --i;
        while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) --i;
        return i;
    }

    static size_t nextLength(const std::string& s, size_t i) {
        size_t j = i + 1;
        while (j < s.size() && ((unsigned char)s[j] & 0xC0) == 0x80) ++j;
        return j - i;
    }

    void handleEscape(std::string& buf, size_t& cursor, size_t& histIndex, std::string& draft) {
        unsigned char a, b;
        if (read(STDIN_FILENO, &a, 1) <= 0) return;
        if (a != '[' && a != 'O') return;
        if (read(STDIN_FILENO, &b, 1) <= 0) return;

        if (b == 'A') {
            if (histIndex > 0) {
                if (histIndex == history_.size()) draft = buf;
                --histIndex;
                buf = history_[histIndex];
                cursor = buf.size();
            }
        } else if (b == 'B') {
            if (histIndex < history_.size()) {
                ++histIndex;
                buf = histIndex == history_.size() ? draft : history_[histIndex];
                cursor = buf.size();
            }
        } else if (b == 'C') {
            if (cursor < buf.size()) cursor += nextLength(buf, cursor);
        } else if (b == 'D') {
            cursor = prevIndex(buf, cursor);
        } else if (b == 'H') {
            cursor = 0;
        } else if (b == 'F') {
            cursor = buf.size();
        } else if (b >= '0' && b <= '9') {
            unsigned char tilde;
            if (read(STDIN_FILENO, &tilde, 1) <= 0 || tilde != '~') return;
            if (b == '3' && cursor < buf.size()) buf.erase(cursor, nextLength(buf, cursor));
            else if (b == '1' || b == '7') cursor = 0;
            else if (b == '4' || b == '8') cursor = buf.size();
        }
    }
};
