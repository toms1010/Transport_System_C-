#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

class QApplication;
class QWidget;

namespace gui {

enum class ThemeKind { Light, Dark };

// Central source of colour truth.
//
// Widgets never hardcode a colour: they read from app.qss tokens, and this class
// supplies the token values for the active palette. Switching between Light and
// Dark swaps the token map and re-applies the same stylesheet.
struct Palette {
    QColor background;
    QColor surface;
    QColor surfaceAlt;
    QColor inputBackground;
    QColor headerBackground;
    QColor border;
    QColor borderStrong;
    QColor text;
    QColor muted;
    QColor primary;
    QColor primaryHover;
    QColor primarySoft;
    QColor primaryBorder;
    QColor success;
    QColor successSoft;
    QColor successBorder;
    QColor warning;
    QColor warningSoft;
    QColor danger;
    QColor dangerSoft;
    QColor sidebar;
    QColor sidebarHover;
    QColor sidebarText;
    QColor sidebarTitle;
    QColor sidebarMuted;
    QColor sidebarAccent;
    QColor seatFree;
    QColor seatTaken;
    QColor seatMine;
    QColor scrollHandle;
    QColor scrollHandleHover;
    QColor tooltipBackground;
    QColor tooltipText;
    QColor toastBackground;
    QColor toastText;
    QColor toastBorder;
    QColor busyOverlay;
    int fontSize;
};

namespace color {
// Live accessors resolve against the active palette so widget code can keep
// using color::success() etc. without threading a palette through every call.
QColor background();
QColor surface();
QColor surfaceAlt();
QColor border();
QColor borderStrong();
QColor text();
QColor textMuted();
QColor primary();
QColor primaryHover();
QColor primarySoft();
QColor success();
QColor successSoft();
QColor warning();
QColor warningSoft();
QColor danger();
QColor dangerSoft();
QColor tooltipText();
QColor sidebarAccent();
QColor sidebarPanel();
}  // namespace color

void applyTheme(QApplication& app);

ThemeKind activeTheme();
Palette activePalette();

// Switches palette, persists the choice and restyles the running application.
void setTheme(ThemeKind kind);

QIcon icon(const QString& name);

}  // namespace gui