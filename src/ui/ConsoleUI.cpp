#include "ui/ConsoleUI.hpp"

#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#endif

namespace st {
namespace ui {
namespace {

#ifdef _WIN32
HANDLE g_console = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_CURSOR_INFO g_cursorInfo{};
bool g_cursorHidden = false;
#else
termios g_saved{};
bool g_rawActive = false;
#endif

struct RawModeGuard {
    RawModeGuard() { rawModeBegin(); }
    ~RawModeGuard() { rawModeEnd(); }
};

bool g_rawReady = false;
int g_lastChar = -1;

}  // namespace

void setColor(Color color) {
    static const char* codes[] = {"\033[0m",   "\033[1m",   "\033[2m",  "\033[31m",
                                  "\033[32m", "\033[33m", "\033[34m", "\033[36m",
                                  "\033[35m", "\033[97m"};
    const std::size_t idx = static_cast<std::size_t>(color);
    if (idx < sizeof(codes) / sizeof(codes[0])) std::cout << codes[idx];
}

int terminalWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(g_console, &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
    return 80;
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
    return 80;
#endif
}

int terminalHeight() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(g_console, &info)) {
        return info.srWindow.Bottom - info.srWindow.Top + 1;
    }
    return 24;
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) return ws.ws_row;
    return 24;
#endif
}

// Usable width for one line of body text: the 2-space left margin plus at least
// one space of right margin. Everything that draws must stay within this or the
// terminal soft-wraps and the layout falls apart.
int contentWidth() {
    const int w = terminalWidth();
    if (w < 50) return std::max(20, w - 2);
    if (w > 100) return 98;
    return w - 2;
}

void init() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    if (GetConsoleCursorInfo(g_console, &g_cursorInfo)) g_cursorReady = true;
#else
    g_rawReady = true;
#endif
}

void clearScreen() {
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

void gotoXY(int column, int row) {
    std::cout << "\033[" << row << ";" << column << "H";
    std::cout.flush();
}

void hideCursor() {
    std::cout << "\033[?25l";
    std::cout.flush();
#ifdef _WIN32
    if (GetConsoleCursorInfo(g_console, &g_cursorInfo)) {
        g_cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(g_console, &g_cursorInfo);
        g_cursorHidden = true;
    }
#endif
}

void showCursor() {
    std::cout << "\033[?25h";
    std::cout.flush();
#ifdef _WIN32
    if (g_cursorHidden) {
        SetConsoleCursorInfo(g_console, &g_cursorInfo);
        g_cursorHidden = false;
    }
#endif
}


std::string paint(Color color, const std::string& text) {
    setColor(color);
    std::string out = text;
    setColor(Color::Reset);
    return out;
}

void rawModeBegin() {
#ifdef _WIN32
    (void)0;
#else
    if (g_rawActive || !g_rawReady) return;
    if (tcgetattr(STDIN_FILENO, &g_saved) != 0) return;
    termios raw = g_saved;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) g_rawActive = true;
#endif
}

void rawModeEnd() {
#ifdef _WIN32
    (void)0;
#else
    if (!g_rawActive) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
    g_rawActive = false;
#endif
}

namespace {

int rawByte() {
#ifdef _WIN32
    const int c = _getch();
    if (c == 0 || c == 224) {
        const int ext = _getch();
        switch (ext) {
            case 72: return 1001;
            case 80: return 1002;
            case 75: return 1003;
            case 77: return 1004;
            default: return -2;
        }
    }
    if (c == 3) return 4;
    return c;
#else
    unsigned char c = 0;
    const ssize_t n = ::read(STDIN_FILENO, &c, 1);
    if (n != 1) return -1;
    if (c == 3) return 4;
    return c;
#endif
}

int timedByte() {
#ifdef _WIN32
    if (_kbhit()) return rawByte();
    return -1;
#else
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeval tv{0, 50000};
    if (select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) <= 0) return -1;
    return rawByte();
#endif
}

}

int lastKeyChar() { return g_lastChar; }

Key readKey() {
    g_lastChar = -1;
    const int c = rawByte();
    if (c < 0) return Key::Eof;
    if (c > 0 && c < 256) g_lastChar = c;

    switch (c) {
        case 1001: return Key::Up;
        case 1002: return Key::Down;
        case 1003: return Key::Left;
        case 1004: return Key::Right;
        case 4: return Key::Eof;
        case 13: return Key::Enter;
        // LF is Enter. Treating '\n' as "move down" (as the original code did)
        // meant a pasted Enter silently changed the highlighted menu item.
        case 10: return Key::Enter;
        case 9: return Key::Down;
        case 127:
        case 8: return Key::Backspace;
        case 27: {
            const int first = timedByte();
            if (first == '[' || first == 'O') {
                const int code = timedByte();
                switch (code) {
                    case 'A': return Key::Up;
                    case 'B': return Key::Down;
                    case 'C': return Key::Right;
                    case 'D': return Key::Left;
                    default: return Key::Character;
                }
            }
            if (first < 0) return Key::Escape;
            return Key::Escape;
        }
        default: return Key::Character;
    }
}

int waitForAnyKey() {
    RawModeGuard guard;
    const int c = rawByte();
    if (c == 27) {
        timedByte();
        timedByte();
    }
    return c;
}

std::string readLineField(const std::string& prompt, int width) {
    std::cout << "  " << std::left << std::setw(width) << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n";
        std::exit(0);
    }
    return util::trim(line);
}

std::string readSecretField(const std::string& prompt, int width) {
    std::cout << "  " << std::left << std::setw(width) << prompt << std::flush;
    std::string secret;
    int c;
    while ((c = rawByte()) >= 0) {
        if (c == 13 || c == 10) break;
        if (c == 4) std::exit(0);
        if (c == 127 || c == 8) {
            if (!secret.empty()) {
                secret.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
            continue;
        }
        if (c < 32 || secret.size() >= 128) continue;
        secret.push_back(static_cast<char>(c));
        std::cout << '*';
        std::cout.flush();
    }
    std::cout << "\n";
    return secret;
}

std::string readMultiline(const std::string& prompt) {
    std::cout << "  " << prompt << "\n";
    std::string text;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (util::trim(line).empty()) break;
        if (!text.empty()) text += "\n";
        text += line;
    }
    return util::trim(text);
}

bool confirm(const std::string& question) {
    while (true) {
        std::cout << "  " << question << " [Y/n] " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) std::exit(0);
        const std::string answer = util::lower(util::trim(line));
        if (answer.empty() || answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        std::cout << "  " << paint(Color::Red, "[ERROR] Please type Y or N.") << "\n";
    }
}

}  // namespace ui
}  // namespace st