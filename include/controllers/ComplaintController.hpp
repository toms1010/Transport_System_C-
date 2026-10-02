#pragma once

#include "controllers/ActionResult.hpp"

#include "models/Complaint.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

enum class ComplaintFilter { All, Open, Resolved };

struct ComplaintRow {
    QString id;
    QString subject;
    QString description;
    QString createdAt;
    QString statusLabel;
    QString statusKind;
    bool resolved = false;
    QString response;
    QString resolvedBy;
    QString resolvedAt;
    QString studentId;
    QString studentName;
};

class ComplaintController {
public:
    explicit ComplaintController(AppContext& context);

    QVector<ComplaintRow> forStudent(const QString& studentId) const;
    QVector<ComplaintRow> list(ComplaintFilter filter, const QString& search) const;

    ActionResult submit(const QString& studentId, const QString& subject,
                        const QString& description, QString& createdId);
    ActionResult resolve(const QString& complaintId, const QString& resolver,
                         const QString& response);
    ActionResult reopen(const QString& complaintId, const QString& resolver);

    bool findRow(const QString& complaintId, ComplaintRow& row) const;
    int openCount() const;

private:
    ComplaintRow toRow(const st::Complaint& complaint) const;

    AppContext& context_;
};

}  // namespace gui
