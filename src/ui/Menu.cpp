#include "ui/Menu.hpp"

#include "utils/TextUtils.hpp"

#include <algorithm>
#include <iomanip>
#include <algorithm>
#include <iostream>

namespace st {
namespace ui {
namespace {

// U+2500 BOX DRAWINGS LIGHT HORIZONTAL, as a raw byte so the source stays ASCII.
const char* const kRuleRun = "\xE2\x94\x80";

std::string statusLabel(StatusKind kind) {
    switch (kind) {
        case StatusKind::Ok: return "OK";
        case StatusKind::Info: return "INFO";
        case StatusKind::Warning: return "WARNING";
        case StatusKind::Error: return "ERROR";
        case StatusKind::Open: return "OPEN";
        case StatusKind::Resolved: return "RESOLVED";
        case StatusKind::Paid: return "PAID";
        case StatusKind::Pending: return "PENDING";
        case StatusKind::Neutral:
        default: return "INFO";
    }
}

Color statusColor(StatusKind kind) {
    switch (kind) {
        case StatusKind::Ok:
        case StatusKind::Paid:
        case StatusKind::Resolved: return Color::Green;
        case StatusKind::Warning:
        case StatusKind::Pending:
        case StatusKind::Open: return Color::Yellow;
        case StatusKind::Error: return Color::Red;
        case StatusKind::Info: return Color::Cyan;
        case StatusKind::Neutral:
        default: return Color::Dim;
    }
}

std::string fitTo(const std::string& text, int width) {
    if (static_cast<int>(text.size()) <= width) return text;
    if (width <= 1) return text.substr(0, static_cast<std::size_t>(width));
    return text.substr(0, static_cast<std::size_t>(width - 1)) + "~";
}

// Centres inside the banner box: 2 columns of margin, 1 for the border glyph,
// then the same interior width boxLine() draws.
std::string boxCentered(const std::string& text, Color color = Color::Reset) {
    const int inner = std::max(1, contentWidth() - 2);
    const std::string fitted = fitTo(text, inner);
    const int pad = std::max(0, (inner - static_cast<int>(fitted.size())) / 2);
    std::string out = "   ";
    out.append(static_cast<std::size_t>(pad), ' ');
    out += paint(color, fitted);
    for (int i = pad + static_cast<int>(fitted.size()); i < inner; ++i) out.push_back(' ');
    return out;
}

void boxLine(const std::string& left, const std::string& fill, const std::string& right) {
    // The two corner glyphs count towards the width, otherwise the line is two
    // columns too wide and the terminal soft-wraps the box.
    const int inner = std::max(1, contentWidth() - 2);
    std::cout << "  " << paint(Color::Cyan, left);
    for (int i = 0; i < inner; ++i) std::cout << paint(Color::Cyan, fill);
    std::cout << paint(Color::Cyan, right) << "\n";
}

}

std::string statusTag(StatusKind kind) { return "[" + statusLabel(kind) + "]"; }

std::string statusText(StatusKind kind, const std::string& text) {
    return paint(statusColor(kind), statusTag(kind)) + " " + text;
}

void banner(const std::string& title, const std::string& subtitle) {
    clearScreen();
    std::cout << "\n";
    boxLine("\u2554", "\u2550", "\u2557");
    std::cout << boxCentered(title, Color::Bold) << "\n";
    if (!subtitle.empty()) std::cout << boxCentered(subtitle, Color::Dim) << "\n";
    boxLine("\u255a", "\u2550", "\u255d");
    std::cout << "\n";
}

void pageHeader(const std::string& title, const std::string& subtitle) {
    std::cout << "  " << paint(Color::Bold, title);
    if (!subtitle.empty()) std::cout << "  " << paint(Color::Dim, subtitle);
    std::cout << "\n";
    rule();
}

void section(const std::string& title) {
    std::cout << "\n  " << paint(Color::Cyan, title) << "\n";
}

void rule() {
    std::string line;
    for (int i = 0; i < contentWidth(); ++i) line += kRuleRun;
    std::cout << "  " << paint(Color::Dim, line) << "\n";
}

void field(const std::string& label, const std::string& value, int labelWidth) {
    std::cout << "  " << std::left << std::setw(labelWidth) << label << value << "\n";
}

void note(const std::string& message, StatusKind kind) {
    std::cout << "  " << statusText(kind, message) << "\n";
}

void table(const std::vector<std::string>& headers, const std::vector<int>& widths,
           const std::vector<std::vector<std::string>>& rows) {
    std::cout << "  ";
    for (std::size_t i = 0; i < headers.size() && i < widths.size(); ++i) {
        std::cout << paint(Color::Bold, util::pad(headers[i], widths[i]));
    }
    std::cout << "\n";
    rule();

    for (const std::vector<std::string>& row : rows) {
        std::cout << "  ";
        for (std::size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            std::string cell = row[i];
            // Never let a long value push the rest of the table out of alignment.
            const int width = widths[i];
            if (static_cast<int>(cell.size()) > width) {
                cell = width <= 1 ? cell.substr(0, static_cast<std::size_t>(width))
                                  : cell.substr(0, static_cast<std::size_t>(width - 1)) + "~";
            }
            std::cout << util::pad(cell, width);
        }
        std::cout << "\n";
    }
    if (!rows.empty()) rule();
}

void pause(const std::string& prompt) {
    std::cout << "\n  " << paint(Color::Dim, prompt) << " ";
    std::cout.flush();
    waitForAnyKey();
    std::cout << "\n";
}

void footer(const std::string& left, const std::string& right) {
    std::cout << "\n";
    rule();
    const std::size_t used = left.size() + right.size();
    const std::size_t space = static_cast<std::size_t>(contentWidth());
    const std::size_t gap = space > used ? space - used : 1;
    std::cout << "  " << paint(Color::Dim, left) << std::string(gap, ' ')
              << paint(Color::Dim, right) << "\n";
}

int menu(const std::string& title, const std::vector<std::string>& items, int startIndex,
         bool backable) {
    MenuOptions options;
    options.startIndex = startIndex;
    options.backable = backable;
    return menu(title, items, options);
}

int menu(const std::string& title, const std::vector<std::string>& items,
         const MenuOptions& options) {
    if (items.empty()) return kMenuCancelled;

    const int startIndex = options.startIndex;
    const bool backable = options.backable;
    const std::string& footerText = options.footerText;
    const std::vector<std::string>& context = options.context;

    int selected = std::min(std::max(startIndex, 0), static_cast<int>(items.size()) - 1);

    rawModeBegin();
    struct Guard {
        ~Guard() { rawModeEnd(); showCursor(); }
    } guard;

    const int itemLines = static_cast<int>(items.size());

    // The list is positioned by absolute row rather than drawn in flow.
    //
    // The previous version moved the cursor back up with "CSI <n> A" and cleared
    // to end of screen. That only works while the drawn block has not scrolled
    // off the top: on a short terminal the terminal auto-scrolls, the cursor no
    // longer sits below the block, and every redraw left a duplicate copy of the
    // whole menu behind. Absolute addressing is immune to scrolling.
    const int rows = terminalHeight();
    const bool hasFooter = !footerText.empty() && rows >= 8;
    const int footerRow = hasFooter ? rows : 0;

    const int contextTop = 3;
    int listTop = contextTop + static_cast<int>(context.size()) + 1;
    const int reserved = itemLines + 1 + (hasFooter ? 2 : 0);
    if (rows - listTop < reserved) {
        listTop = rows - reserved;
        if (listTop < 1) listTop = 1;
    }
    const int hintRow = listTop - 1;
    // The context block is dropped rather than squeezed off the top of a short screen.
    const int contextLines = std::max(0, std::min<int>(
                             static_cast<int>(context.size()), listTop - contextTop - 1));

    // Keep the hint on one line: drop the extras rather than being truncated.
    std::string hint = "\u2191/\u2193 navigate    ENTER select";
    const std::size_t hintWidth = static_cast<std::size_t>(contentWidth() - 2);
    if (hint.size() + 12 <= hintWidth) hint += "    1-9 quick select";
    if (backable && hint.size() + 9 <= hintWidth) hint += "    ESC back";

    for (;;) {
        // Repaint the static header every pass so a scroll can never orphan it.
        clearScreen();
        gotoXY(1, 1);
        std::cout << "  " << paint(Color::Bold, title) << "\n";
        gotoXY(1, 2);
        rule();

        for (int i = 0; i < contextLines; ++i) {
            gotoXY(1, contextTop + i);
            std::cout << fitTo(context[static_cast<std::size_t>(i)], contentWidth()) << "\n";
        }

        if (hintRow >= 1) {
            gotoXY(1, hintRow);
            std::cout << "  " << paint(Color::Dim, fitTo(hint, contentWidth() - 2)) << "\n";
        }

        // Position at column 1 of the first item row and clear everything below,
        // so a redraw can never leave stale lines at the bottom of the screen.
        gotoXY(1, listTop);
        std::cout << "\033[J";

        for (std::size_t i = 0; i < items.size(); ++i) {
            const bool active = static_cast<int>(i) == selected;
            if (active) {
                std::cout << "  " << paint(Color::Cyan, "\u25B6") << " "
                          << paint(Color::Bold, items[i]) << "\n";
            } else {
                std::cout << "    " << paint(Color::Dim, items[i]) << "\n";
            }
        }
        if (hasFooter) {
            gotoXY(1, footerRow);
            const int w = contentWidth();
            const std::string fitted = fitTo(footerText, w);
            const std::size_t gap =
                w > static_cast<int>(fitted.size())
                    ? static_cast<std::size_t>(w - static_cast<int>(fitted.size()))
                    : 0;
            std::cout << "  " << paint(Color::Dim, fitted) << std::string(gap, ' ') << "\n";
        }

        std::cout.flush();
        hideCursor();

        const Key key = readKey();
        if (key == Key::Eof) return kMenuInputClosed;
        if (key == Key::Character) {
            const int ch = lastKeyChar();
            if (ch >= '1' && ch <= '9') {
                const int index = ch - '1';
                if (index < itemLines) return index;
            }
            continue;
        }
        if (key == Key::Up) {
            selected = (selected - 1 + itemLines) % itemLines;
        } else if (key == Key::Down) {
            selected = (selected + 1) % itemLines;
        } else if (key == Key::Enter) {
            return selected;
        } else if (key == Key::Escape) {
            if (backable) return kMenuCancelled;
        }
    }
}

}  // namespace ui
}  // namespace st