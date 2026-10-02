#pragma once

#include "ui/ConsoleUI.hpp"

#include <string>
#include <vector>

namespace st {
namespace ui {

// Sentinel returns from menu(). Non-negative values are item indices.
constexpr int kMenuCancelled = -1;   // Esc pressed
constexpr int kMenuInputClosed = -2;  // stdin closed (Ctrl-D / EOF)

enum class StatusKind { Ok, Info, Warning, Error, Open, Resolved, Paid, Pending, Neutral };

std::string statusTag(StatusKind kind);
std::string statusText(StatusKind kind, const std::string& text);

void banner(const std::string& title, const std::string& subtitle);
void pageHeader(const std::string& title, const std::string& subtitle = "");
void section(const std::string& title);
void rule();
void field(const std::string& label, const std::string& value, int labelWidth = 24);
void note(const std::string& message, StatusKind kind = StatusKind::Info);
void table(const std::vector<std::string>& headers, const std::vector<int>& widths,
           const std::vector<std::vector<std::string>>& rows);
void pause(const std::string& prompt = "Press any key to continue");
void footer(const std::string& left = "", const std::string& right = "");

struct MenuOptions {
    int startIndex = 0;
    bool backable = true;
    std::string footerText;             // drawn on the last terminal row
    std::vector<std::string> context;   // plain lines under the title, e.g. a
                                        // dashboard summary
};

// Owns the whole screen while it is open: it clears, paints the title, the
// context block, the key hint, the item list and the footer, then repaints in
// place on every keypress. Because it repaints everything, anything a caller
// wants visible on this screen must be passed in rather than printed before.
int menu(const std::string& title, const std::vector<std::string>& items,
         const MenuOptions& options);

int menu(const std::string& title, const std::vector<std::string>& items, int startIndex = 0,
         bool backable = true);

}  // namespace ui
}  // namespace st