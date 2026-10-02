#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace st {
namespace ui {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Escape,
    Backspace,
    Character,
    Eof,
};

enum class Color {
    Reset,
    Bold,
    Dim,
    Red,
    Green,
    Yellow,
    Blue,
    Cyan,
    Magenta,
    White,
};

int terminalWidth();
int terminalHeight();
int contentWidth();

void init();
void clearScreen();
void gotoXY(int column, int row);
void hideCursor();
void showCursor();

void setColor(Color color);
std::string paint(Color color, const std::string& text);

void rawModeBegin();
void rawModeEnd();
Key readKey();

// Returns the literal character that produced Key::Character, otherwise -1.
int lastKeyChar();

int waitForAnyKey();

std::string readLineField(const std::string& prompt, int width = 26);
std::string readSecretField(const std::string& prompt, int width = 26);
std::string readMultiline(const std::string& prompt);
bool confirm(const std::string& question);

}  // namespace ui
}  // namespace st