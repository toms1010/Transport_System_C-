#pragma once

#include <QString>

namespace gui {

// Shared result shape for every controller operation, so the GUI never has to
// interpret a raw service enum or an out-parameter.
struct ActionResult {
    bool ok = false;
    QString message;
    QString title;

    static ActionResult success(const QString& message = QString());
    static ActionResult failure(const QString& title, const QString& message);
};

}  // namespace gui
