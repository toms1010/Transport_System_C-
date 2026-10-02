#pragma once

#include "controllers/ActionResult.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

enum class DuesFilter { Any, WithDues, FullyPaid, NoSeat };

struct StudentRow {
    QString id;
    QString name;
    QString username;
    QString fatherName;
    QString phone;
    QString address;
    QString routeId;
    QString routeName;
    QString seatLabel;
    bool hasSeat = false;
    long long total = 0;
    long long paid = 0;
    long long outstanding = 0;
    QString statusLabel;
    QString statusKind;
};

struct RosterFilter {
    QString search;
    QString routeId;
    DuesFilter dues = DuesFilter::Any;
};

class StudentController {
public:
    explicit StudentController(AppContext& context);

    QVector<StudentRow> roster(const RosterFilter& filter) const;
    bool findRow(const QString& studentId, StudentRow& row) const;

    QStringList namesWithIds() const;
    QStringList idList() const;
    int count() const;

private:
    StudentRow toRow(const QString& studentId) const;

    AppContext& context_;
};

}  // namespace gui
