#include "Notify.hpp"

#include "Theme.hpp"

#include <QCoreApplication>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

namespace gui {
namespace {

constexpr int kToastWidth = 340;
constexpr int kToastLifetimeMs = 3200;

QString kindName(ToastKind kind) {
    switch (kind) {
        case ToastKind::Success:
            return QStringLiteral("success");
        case ToastKind::Error:
            return QStringLiteral("error");
        case ToastKind::Warning:
            return QStringLiteral("warning");
        default:
            return QStringLiteral("info");
    }
}

QString kindHeading(ToastKind kind) {
    switch (kind) {
        case ToastKind::Success:
            return QStringLiteral("Done");
        case ToastKind::Error:
            return QStringLiteral("Something went wrong");
        case ToastKind::Warning:
            return QStringLiteral("Warning");
        default:
            return QStringLiteral("Notice");
    }
}

}  // namespace

void Toast::show(QWidget* parent, const QString& message, ToastKind kind) {
    if (!parent) return;

    // Toasts stack upwards from the bottom-right of the window they belong to.
    static int depth = 0;
    if (depth > 3) depth = 0;
    ++depth;

    auto* host = parent->window();
    auto* toast = new QFrame(host);
    toast->setObjectName("Toast");
    toast->setProperty("kind", kindName(kind));
    toast->setFixedWidth(kToastWidth);

    auto* layout = new QVBoxLayout(toast);
    layout->setContentsMargins(14, 11, 14, 11);
    layout->setSpacing(3);

    auto* heading = new QLabel(kindHeading(kind));
    heading->setObjectName("ToastTitle");

    auto* body = new QLabel(message);
    body->setObjectName("ToastBody");
    body->setWordWrap(true);

    layout->addWidget(heading);
    layout->addWidget(body);

    const int margin = 22;
    const int offset = depth * 74;
    toast->adjustSize();
    toast->move(host->width() - toast->width() - margin,
                host->height() - toast->height() - margin - offset);
    toast->raise();
    toast->show();

    auto* effect = new QGraphicsOpacityEffect(toast);
    effect->setOpacity(0.0);
    toast->setGraphicsEffect(effect);

    auto* fadeIn = new QPropertyAnimation(effect, "opacity", toast);
    fadeIn->setDuration(160);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    QPointer<QFrame> guard(toast);
    QTimer::singleShot(kToastLifetimeMs, toast, [guard] {
        if (!guard) return;
        auto* fadeOut = new QPropertyAnimation(guard->graphicsEffect(), "opacity", guard);
        fadeOut->setDuration(220);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, guard, &QObject::deleteLater);
        --depth;
    });

    // Keep the toast above its parent window if the parent is resized.
    if (auto* anchor = parent) {
        QObject::connect(anchor, &QWidget::destroyed, toast, &QObject::deleteLater);
    }
}

void Toast::show(QWidget* parent, const ActionResult& result) {
    if (!result.ok) {
        show(parent, result.message.isEmpty() ? result.title : result.message, ToastKind::Error);
        return;
    }
    if (result.message.isEmpty()) return;
    show(parent, result.message, ToastKind::Success);
}

BusyOverlay::BusyOverlay(QWidget* parent, const QString& message) : QFrame(parent) {
    setObjectName("BusyOverlay");
    setAttribute(Qt::WA_DeleteOnClose, false);
    setGeometry(parent->rect());

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(14);

    message_ = new QLabel(message.isEmpty() ? QStringLiteral("Working…") : message);
    message_->setObjectName("BusyText");
    message_->setAlignment(Qt::AlignCenter);

    layout->addWidget(message_);
}

void BusyOverlay::setMessage(const QString& message) {
    if (message_) message_->setText(message);
}

}  // namespace gui