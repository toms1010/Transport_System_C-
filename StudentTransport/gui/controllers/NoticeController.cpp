#include "controllers/NoticeController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

#include "models/Notice.hpp"

namespace gui {

NoticeController::NoticeController(AppContext& context) : context_(context) {}

QVector<NoticeRow> NoticeController::rows() const {
    QVector<NoticeRow> result;
    for (const st::Notice& notice : context_.notices().cache()) {
        NoticeRow row;
        row.id = qs(notice.noticeId());
        row.title = qs(notice.title());
        row.content = qs(notice.content());
        row.authorId = qs(notice.authorId());
        row.authorName = context_.studentName(notice.authorId());
        row.createdAt = qs(notice.createdAt());

        QString excerpt = row.content.simplified();
        if (excerpt.size() > 140) excerpt = excerpt.left(137) + QStringLiteral("…");
        row.excerpt = excerpt;
        result.push_back(row);
    }
    return result;
}

ActionResult NoticeController::publish(const QString& title, const QString& content,
                                       const QString& authorId) {
    std::string createdId;
    const st::NoticeResult result =
        context_.notices().publish(toStd(title.trimmed()), toStd(content.trimmed()),
                                   toStd(authorId), createdId);

    if (result != st::NoticeResult::Success) {
        return ActionResult::failure(QStringLiteral("Notice not published"),
                                     qs(st::noticeResultMessage(result)));
    }
    return ActionResult::success(QStringLiteral("Notice published."));
}

ActionResult NoticeController::edit(const QString& noticeId, const QString& title,
                                    const QString& content) {
    std::string message;
    const st::NoticeResult result =
        context_.notices().edit(toStd(noticeId), toStd(title.trimmed()), toStd(content.trimmed()),
                                message);

    if (result != st::NoticeResult::Success) {
        return ActionResult::failure(QStringLiteral("Notice not saved"),
                                     qs(st::noticeResultMessage(result)));
    }
    return ActionResult::success(QStringLiteral("Notice updated."));
}

ActionResult NoticeController::remove(const QString& noticeId) {
    std::string message;
    const st::NoticeResult result = context_.notices().remove(toStd(noticeId), message);
    if (result != st::NoticeResult::Success) {
        return ActionResult::failure(QStringLiteral("Notice not deleted"),
                                     qs(st::noticeResultMessage(result)));
    }
    return ActionResult::success(QStringLiteral("Notice deleted."));
}

bool NoticeController::findRow(const QString& noticeId, NoticeRow& row) const {
    const st::Notice* notice = context_.notices().find(toStd(noticeId));
    if (!notice) return false;

    row.id = qs(notice->noticeId());
    row.title = qs(notice->title());
    row.content = qs(notice->content());
    row.authorId = qs(notice->authorId());
    row.authorName = context_.studentName(notice->authorId());
    row.createdAt = qs(notice->createdAt());
    return true;
}

}  // namespace gui
