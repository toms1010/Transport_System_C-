#include "controllers/ComplaintController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

namespace gui {
namespace {

bool matches(const QString& haystack, const QString& needle) {
    return needle.isEmpty() ||
           haystack.contains(needle, Qt::CaseInsensitive);
}

}  // namespace

ComplaintController::ComplaintController(AppContext& context) : context_(context) {}

ComplaintRow ComplaintController::toRow(const st::Complaint& complaint) const {
    ComplaintRow row;
    row.id = qs(complaint.complaintId());
    row.subject = qs(complaint.subject());
    row.description = qs(complaint.description());
    row.createdAt = qs(complaint.createdAt());
    row.resolved = complaint.status() == st::ComplaintStatus::Resolved;
    row.statusLabel = row.resolved ? QStringLiteral("Resolved") : QStringLiteral("Open");
    row.statusKind = row.resolved ? QStringLiteral("success") : QStringLiteral("warning");
    row.response = qs(complaint.response());
    row.resolvedBy = qs(complaint.resolvedBy());
    row.resolvedAt = qs(complaint.resolvedAt());
    row.studentId = qs(complaint.studentId());
    row.studentName = context_.studentName(complaint.studentId());
    return row;
}

QVector<ComplaintRow> ComplaintController::forStudent(const QString& studentId) const {
    QVector<ComplaintRow> result;
    if (studentId.isEmpty()) return result;
    for (const st::Complaint& complaint : context_.complaints().forStudent(toStd(studentId))) {
        result.push_back(toRow(complaint));
    }
    return result;
}

QVector<ComplaintRow> ComplaintController::list(ComplaintFilter filter,
                                                 const QString& search) const {
    QVector<ComplaintRow> result;
    for (const st::Complaint& complaint : context_.complaints().cache()) {
        const bool resolved = complaint.status() == st::ComplaintStatus::Resolved;
        if (filter == ComplaintFilter::Open && resolved) continue;
        if (filter == ComplaintFilter::Resolved && !resolved) continue;

        ComplaintRow row = toRow(complaint);
        if (!matches(row.subject, search) && !matches(row.studentName, search) &&
            !matches(row.id, search)) {
            continue;
        }
        result.push_back(row);
    }
    return result;
}

ActionResult ComplaintController::submit(const QString& studentId, const QString& subject,
                                         const QString& description, QString& createdId) {
    std::string id;
    const st::ComplaintResult result = context_.complaints().submit(
        toStd(studentId), toStd(subject.trimmed()), toStd(description.trimmed()), id);

    if (result != st::ComplaintResult::Success) {
        return ActionResult::failure(QStringLiteral("Complaint not submitted"),
                                     qs(st::complaintResultMessage(result)));
    }

    createdId = qs(id);
    return ActionResult::success(QStringLiteral("Complaint %1 submitted.").arg(createdId));
}

ActionResult ComplaintController::resolve(const QString& complaintId, const QString& resolver,
                                          const QString& response) {
    std::string message;
    const st::ComplaintResult result =
        context_.complaints().resolve(toStd(complaintId), toStd(resolver),
                                      toStd(response.trimmed()), message);

    if (result != st::ComplaintResult::Success) {
        return ActionResult::failure(QStringLiteral("Could not resolve the complaint"),
                                     qs(st::complaintResultMessage(result)));
    }
    return ActionResult::success(QStringLiteral("Complaint %1 marked resolved.").arg(complaintId));
}

ActionResult ComplaintController::reopen(const QString& complaintId, const QString& resolver) {
    std::string message;
    const st::ComplaintResult result =
        context_.complaints().reopen(toStd(complaintId), toStd(resolver), message);

    if (result != st::ComplaintResult::Success) {
        return ActionResult::failure(QStringLiteral("Could not reopen the complaint"),
                                     qs(st::complaintResultMessage(result)));
    }
    return ActionResult::success(QStringLiteral("Complaint %1 reopened.").arg(complaintId));
}

bool ComplaintController::findRow(const QString& complaintId, ComplaintRow& row) const {
    const st::Complaint* complaint = context_.complaints().find(toStd(complaintId));
    if (!complaint) return false;
    row = toRow(*complaint);
    return true;
}

int ComplaintController::openCount() const { return context_.complaints().openCount(); }

}  // namespace gui