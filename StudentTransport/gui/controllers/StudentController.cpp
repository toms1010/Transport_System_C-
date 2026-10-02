#include "controllers/StudentController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

#include "models/Student.hpp"

namespace gui {

StudentController::StudentController(AppContext& context) : context_(context) {}

StudentRow StudentController::toRow(const QString& studentId) const {
    StudentRow row;
    const st::Student* student = context_.auth().findStudent(toStd(studentId));
    if (!student) return row;

    const st::FeeBreakdown fees = student->breakdown();

    row.id = qs(student->userId());
    row.name = qs(student->fullName());
    row.username = qs(student->username());
    row.fatherName = qs(student->fatherName());
    row.phone = qs(student->phone());
    row.address = qs(student->address());
    row.routeId = qs(student->routeId());
    row.routeName = context_.routeName(student->routeId());
    row.hasSeat = student->seatAssigned();
    row.seatLabel = row.hasSeat ? QStringLiteral("Seat %1").arg(student->seatNumber())
                                : QStringLiteral("—");
    row.total = fees.totalAmount();
    row.paid = fees.totalPaid();
    row.outstanding = fees.outstanding();

    if (fees.totalAmount() == 0) {
        row.statusLabel = QStringLiteral("No seat");
        row.statusKind = QStringLiteral("warning");
    } else if (fees.outstanding() > 0) {
        row.statusLabel = QStringLiteral("Dues pending");
        row.statusKind = QStringLiteral("danger");
    } else {
        row.statusLabel = QStringLiteral("Paid");
        row.statusKind = QStringLiteral("success");
    }
    return row;
}

QVector<StudentRow> StudentController::roster(const RosterFilter& filter) const {
    QVector<StudentRow> result;

    for (const std::string& id : context_.studentIds()) {
        StudentRow row = toRow(qs(id));
        if (row.id.isEmpty()) continue;

        if (!filter.search.isEmpty() && !row.name.contains(filter.search, Qt::CaseInsensitive) &&
            !row.username.contains(filter.search, Qt::CaseInsensitive) &&
            !row.id.contains(filter.search, Qt::CaseInsensitive)) {
            continue;
        }
        if (!filter.routeId.isEmpty() && row.routeId != filter.routeId) continue;

        switch (filter.dues) {
            case DuesFilter::WithDues:
                if (row.outstanding == 0) continue;
                break;
            case DuesFilter::FullyPaid:
                if (row.outstanding > 0) continue;
                break;
            case DuesFilter::NoSeat:
                if (row.hasSeat) continue;
                break;
            case DuesFilter::Any:
                break;
        }
        result.push_back(row);
    }
    return result;
}

bool StudentController::findRow(const QString& studentId, StudentRow& row) const {
    row = toRow(studentId);
    return !row.id.isEmpty();
}

QStringList StudentController::namesWithIds() const {
    QStringList result;
    for (const std::string& id : context_.studentIds()) {
        const st::Student* student = context_.auth().findStudent(id);
        if (!student) continue;
        const QString seat = student->seatAssigned()
                                 ? QStringLiteral(" · seat %1").arg(student->seatNumber())
                                 : QStringLiteral(" · no seat");
        result << QStringLiteral("%1 (@%2)%3")
                      .arg(qs(student->fullName()), qs(student->username()), seat);
    }
    return result;
}

QStringList StudentController::idList() const {
    QStringList result;
    for (const std::string& id : context_.studentIds()) result << qs(id);
    return result;
}

int StudentController::count() const { return context_.auth().studentCount(); }

}  // namespace gui
