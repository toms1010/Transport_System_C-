#pragma once

#include "controllers/ActionResult.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

struct NoticeRow {
    QString id;
    QString title;
    QString content;
    QString authorId;
    QString authorName;
    QString createdAt;
    QString excerpt;
};

class NoticeController {
public:
    explicit NoticeController(AppContext& context);

    QVector<NoticeRow> rows() const;

    ActionResult publish(const QString& title, const QString& content, const QString& authorId);
    ActionResult edit(const QString& noticeId, const QString& title, const QString& content);
    ActionResult remove(const QString& noticeId);

    bool findRow(const QString& noticeId, NoticeRow& row) const;

private:
    AppContext& context_;
};

}  // namespace gui
