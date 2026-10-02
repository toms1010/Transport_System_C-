#include "Theme.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QHash>
#include <QFile>
#include <QPalette>
#include <QRegularExpression>
#include <QSettings>
#include <QStyle>
#include <QWidget>

namespace gui {
namespace {

QColor c(const char* hex) { return QColor(QString::fromLatin1(hex)); }

Palette lightPalette() {
    Palette p;
    p.background = c("#f1f5f9");
    p.surface = c("#ffffff");
    p.surfaceAlt = c("#f8fafc");
    p.inputBackground = c("#ffffff");
    p.headerBackground = c("#f8fafc");
    p.border = c("#e2e8f0");
    p.borderStrong = c("#cbd5e1");
    p.text = c("#0f172a");
    p.muted = c("#64748b");
    p.primary = c("#4f46e5");
    p.primaryHover = c("#4338ca");
    p.primarySoft = c("#eef2ff");
    p.primaryBorder = c("#c7d2fe");
    p.success = c("#047857");
    p.successSoft = c("#ecfdf5");
    p.successBorder = c("#86efac");
    p.warning = c("#b45309");
    p.warningSoft = c("#fffbeb");
    p.danger = c("#dc2626");
    p.dangerSoft = c("#fef2f2");
    p.sidebar = c("#1e1b4b");
    p.sidebarHover = c("#312e81");
    p.sidebarText = c("#e0e7ff");
    p.sidebarTitle = c("#ffffff");
    p.sidebarMuted = c("#a5b4fc");
    p.sidebarAccent = c("#818cf8");
    p.seatFree = c("#ffffff");
    p.seatTaken = c("#f1f5f9");
    p.seatMine = c("#dcfce7");
    p.scrollHandle = c("#cbd5e1");
    p.scrollHandleHover = c("#94a3b8");
    p.tooltipBackground = c("#0f172a");
    p.tooltipText = c("#ffffff");
    p.toastBackground = c("#ffffff");
    p.toastText = c("#0f172a");
    p.toastBorder = c("#e2e8f0");
    p.busyOverlay = c("#f1f5f9");
    p.fontSize = 13;
    return p;
}

Palette darkPalette() {
    Palette p;
    p.background = c("#0b1120");
    p.surface = c("#131c2e");
    p.surfaceAlt = c("#1b2537");
    p.inputBackground = c("#0f1724");
    p.headerBackground = c("#1b2537");
    p.border = c("#24334a");
    p.borderStrong = c("#33455f");
    p.text = c("#e8eef7");
    p.muted = c("#94a3b8");
    p.primary = c("#818cf8");
    p.primaryHover = c("#a5b4fc");
    p.primarySoft = c("#1e2450");
    p.primaryBorder = c("#3b3f7a");
    p.success = c("#34d399");
    p.successSoft = c("#0d2f26");
    p.successBorder = c("#16634a");
    p.warning = c("#fbbf24");
    p.warningSoft = c("#33270a");
    p.danger = c("#f87171");
    p.dangerSoft = c("#3a1414");
    p.sidebar = c("#0f172a");
    p.sidebarHover = c("#1e293b");
    p.sidebarText = c("#cbd5e1");
    p.sidebarTitle = c("#ffffff");
    p.sidebarMuted = c("#7c8db5");
    p.sidebarAccent = c("#818cf8");
    p.seatFree = c("#1b2537");
    p.seatTaken = c("#111827");
    p.seatMine = c("#0d3b2e");
    p.scrollHandle = c("#33455f");
    p.scrollHandleHover = c("#47597a");
    p.tooltipBackground = c("#1e293b");
    p.tooltipText = c("#e8eef7");
    p.toastBackground = c("#1b2537");
    p.toastText = c("#e8eef7");
    p.toastBorder = c("#33455f");
    p.busyOverlay = c("#0b1120");
    p.fontSize = 13;
    return p;
}

ThemeKind currentKind = ThemeKind::Light;

QString qssTemplate() {
    QFile file(QStringLiteral(":/styles/app"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    file.setFileName(QStringLiteral(":/styles/app.qss"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return QString();
}

QString substitute(const Palette& p) {
    QString sheet = qssTemplate();
    if (sheet.isEmpty()) return QString();

    const QHash<QString, QColor> tokens = {
        {QStringLiteral("BG"), p.background},
        {QStringLiteral("SURFACE"), p.surface},
        {QStringLiteral("SURFACE_ALT"), p.surfaceAlt},
        {QStringLiteral("INPUT_BG"), p.inputBackground},
        {QStringLiteral("HEADER_BG"), p.headerBackground},
        {QStringLiteral("BORDER"), p.border},
        {QStringLiteral("BORDER_STRONG"), p.borderStrong},
        {QStringLiteral("TEXT"), p.text},
        {QStringLiteral("MUTED"), p.muted},
        {QStringLiteral("PRIMARY"), p.primary},
        {QStringLiteral("PRIMARY_HOVER"), p.primaryHover},
        {QStringLiteral("PRIMARY_SOFT"), p.primarySoft},
        {QStringLiteral("PRIMARY_BORDER"), p.primaryBorder},
        {QStringLiteral("SUCCESS"), p.success},
        {QStringLiteral("SUCCESS_SOFT"), p.successSoft},
        {QStringLiteral("SUCCESS_BORDER"), p.successBorder},
        {QStringLiteral("WARNING"), p.warning},
        {QStringLiteral("WARNING_SOFT"), p.warningSoft},
        {QStringLiteral("DANGER"), p.danger},
        {QStringLiteral("DANGER_SOFT"), p.dangerSoft},
        {QStringLiteral("SIDEBAR"), p.sidebar},
        {QStringLiteral("SIDEBAR_HOVER"), p.sidebarHover},
        {QStringLiteral("SIDEBAR_TEXT"), p.sidebarText},
        {QStringLiteral("SIDEBAR_TITLE"), p.sidebarTitle},
        {QStringLiteral("SIDEBAR_MUTED"), p.sidebarMuted},
        {QStringLiteral("SIDEBAR_ACCENT"), p.sidebarAccent},
        {QStringLiteral("SEAT_FREE"), p.seatFree},
        {QStringLiteral("SEAT_TAKEN"), p.seatTaken},
        {QStringLiteral("SEAT_MINE"), p.seatMine},
        {QStringLiteral("SCROLL_HANDLE"), p.scrollHandle},
        {QStringLiteral("SCROLL_HANDLE_HOVER"), p.scrollHandleHover},
        {QStringLiteral("TOOLTIP_BG"), p.tooltipBackground},
        {QStringLiteral("TOOLTIP_TEXT"), p.tooltipText},
        {QStringLiteral("TOAST_BG"), p.toastBackground},
        {QStringLiteral("TOAST_TEXT"), p.toastText},
        {QStringLiteral("TOAST_BORDER"), p.toastBorder},
        {QStringLiteral("BUSY_BG"), p.busyOverlay},
    };

    for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it) {
        sheet.replace(QStringLiteral("{{%1}}").arg(it.key()), it.value().name());
    }
    sheet.replace(QStringLiteral("{{FONT_SIZE}}"), QString::number(p.fontSize));
    return sheet;
}

void restyle(QApplication& app, const Palette& palette) {
    QPalette qt;
    qt.setColor(QPalette::Window, palette.background);
    qt.setColor(QPalette::Base, palette.inputBackground);
    qt.setColor(QPalette::AlternateBase, palette.surfaceAlt);
    qt.setColor(QPalette::Text, palette.text);
    qt.setColor(QPalette::WindowText, palette.text);
    qt.setColor(QPalette::Button, palette.surface);
    qt.setColor(QPalette::ButtonText, palette.text);
    qt.setColor(QPalette::Highlight, palette.primary);
    qt.setColor(QPalette::HighlightedText, Qt::white);
    qt.setColor(QPalette::ToolTipBase, palette.tooltipBackground);
    qt.setColor(QPalette::ToolTipText, palette.tooltipText);
    qt.setColor(QPalette::Link, palette.primary);
    qt.setColor(QPalette::Disabled, QPalette::Text, palette.muted);
    qt.setColor(QPalette::Disabled, QPalette::ButtonText, palette.muted);
    qt.setColor(QPalette::Disabled, QPalette::WindowText, palette.muted);
    app.setPalette(qt);

    const QString sheet = substitute(palette);
    if (!sheet.isEmpty()) app.setStyleSheet(sheet);
}

}  // namespace

namespace color {
QColor background() { return activePalette().background; }
QColor surface() { return activePalette().surface; }
QColor surfaceAlt() { return activePalette().surfaceAlt; }
QColor border() { return activePalette().border; }
QColor borderStrong() { return activePalette().borderStrong; }
QColor text() { return activePalette().text; }
QColor textMuted() { return activePalette().muted; }
QColor primary() { return activePalette().primary; }
QColor primaryHover() { return activePalette().primaryHover; }
QColor primarySoft() { return activePalette().primarySoft; }
QColor success() { return activePalette().success; }
QColor successSoft() { return activePalette().successSoft; }
QColor warning() { return activePalette().warning; }
QColor warningSoft() { return activePalette().warningSoft; }
QColor danger() { return activePalette().danger; }
QColor dangerSoft() { return activePalette().dangerSoft; }
QColor tooltipText() { return activePalette().tooltipText; }
}  // namespace color

Palette activePalette() {
    return currentKind == ThemeKind::Dark ? darkPalette() : lightPalette();
}

ThemeKind activeTheme() { return currentKind; }

void applyTheme(QApplication& app) {
    QSettings settings;
    const int stored = settings.value(QStringLiteral("appearance/theme"),
                                      static_cast<int>(ThemeKind::Light))
                            .toInt();
    currentKind = stored == static_cast<int>(ThemeKind::Dark) ? ThemeKind::Dark
                                                              : ThemeKind::Light;

    QFont font = app.font();
    font.setPointSize(10);
    app.setFont(font);

    restyle(app, activePalette());
}

void setTheme(ThemeKind kind) {
    if (kind == currentKind) return;
    currentKind = kind;

    QSettings settings;
    settings.setValue(QStringLiteral("appearance/theme"), static_cast<int>(kind));

    if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        restyle(*app, activePalette());
    }
}

QIcon icon(const QString& name) {
    QString path = QStringLiteral(":/icons/%1.svg").arg(name);
    if (!QFile::exists(path)) path = QStringLiteral(":/icons/%1").arg(name);
    return QIcon::fromTheme(path, QIcon(path));
}

}  // namespace gui