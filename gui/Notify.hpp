#pragma once

#include "controllers/ActionResult.hpp"

#include <QCoreApplication>
#include <QFrame>
#include <QString>

class QLabel;
class QWidget;

namespace gui {

enum class ToastKind { Success, Error, Warning, Info };

// Non-blocking confirmation, auto-dismissed. Spec section 31: toasts for routine
// outcomes, modal dialogs reserved for anything destructive or critical.
class Toast {
public:
    static void show(QWidget* parent, const QString& message,
                     ToastKind kind = ToastKind::Success);
    static void show(QWidget* parent, const ActionResult& result);
};

// Wraps a long operation so the window is not frozen and the user gets told
// something is happening (spec section 32).
class BusyOverlay : public QFrame {
    Q_OBJECT
public:
    explicit BusyOverlay(QWidget* parent, const QString& message = QString());

    void setMessage(const QString& message);

private:
    QLabel* message_ = nullptr;
};

// Runs a callable behind a busy overlay, then always clears the overlay.
template <typename Work>
void runBusy(QWidget* parent, const QString& message, Work&& work) {
    BusyOverlay* overlay = new BusyOverlay(parent, message);
    overlay->setGeometry(parent->rect());
    overlay->show();
    QCoreApplication::processEvents();
    work();
    overlay->hide();
    overlay->deleteLater();
}

}  // namespace gui