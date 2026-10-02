#include "controllers/PaymentController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

#include "controllers/RouteController.hpp"
#include "models/Payment.hpp"
#include "models/Student.hpp"
#include "utils/CsvUtils.hpp"

#include <QFile>
#include <QTextStream>

namespace gui {
namespace {

int percentOf(long long paid, long long charge) {
    if (charge <= 0) return 0;
    const int value = static_cast<int>(paid * 100 / charge);
    return value < 0 ? 0 : (value > 100 ? 100 : value);
}

}  // namespace

PaymentController::PaymentController(AppContext& context) : context_(context) {}

FeeRow PaymentController::toRow(const QString& label, long long charge, long long paid) const {
    FeeRow row;
    row.label = label;
    row.charge = charge;
    row.paid = paid;
    row.due = charge > paid ? charge - paid : 0;
    row.percent = percentOf(paid, charge);
    return row;
}

StatementView PaymentController::statement(const QString& studentId) const {
    StatementView view;
    if (studentId.isEmpty()) return view;

    const st::Student* student = context_.auth().findStudent(toStd(studentId));
    if (!student) return view;

    const st::FeeBreakdown fees = context_.payments().statementFor(toStd(studentId));

    view.valid = true;
    view.studentId = studentId;
    view.studentName = qs(student->fullName());
    view.username = qs(student->username());
    view.routeName = context_.routeName(student->routeId());
    view.seatLabel = student->seatAssigned()
                         ? QStringLiteral("Seat %1").arg(student->seatNumber())
                         : QStringLiteral("No seat allotted");
    view.registration = toRow(QStringLiteral("Registration fee"), fees.registrationFee,
                              fees.registrationPaid);
    view.transport = toRow(QStringLiteral("Transport fee"), fees.transportFee, fees.transportPaid);
    view.totalCharge = fees.totalAmount();
    view.totalPaid = fees.totalPaid();
    view.outstanding = fees.outstanding();
    view.settled = fees.isSettled();
    view.awaitingSeat = fees.totalAmount() == 0;

    if (view.settled && !view.awaitingSeat) {
        view.statusLabel = QStringLiteral("PAID IN FULL");
        view.statusKind = QStringLiteral("success");
    } else if (view.outstanding > 0) {
        view.statusLabel = QStringLiteral("DUES PENDING");
        view.statusKind = QStringLiteral("danger");
    } else {
        view.statusLabel = QStringLiteral("AWAITING SEAT");
        view.statusKind = QStringLiteral("warning");
    }
    return view;
}

QVector<PaymentRow> PaymentController::history(const QString& studentId) const {
    QVector<PaymentRow> result;
    if (studentId.isEmpty()) return result;

    for (const st::Payment& payment : context_.payments().historyFor(toStd(studentId))) {
        PaymentRow row;
        row.id = qs(payment.paymentId());
        row.date = qs(payment.paymentDate());
        row.kind = qs(payment.kindLabel());
        row.reference = qs(payment.reference());
        row.amount = payment.amount();

        const st::User* recorder = context_.auth().findUser(payment.recordedBy());
        row.recordedBy = recorder ? qs(recorder->displayName()) : qs(payment.recordedBy());
        result.push_back(row);
    }
    return result;
}

AggregateView PaymentController::aggregates() const {
    AggregateView view;
    view.collected = context_.payments().collectedTotal();
    view.outstanding = context_.payments().outstandingTotal();
    view.studentsWithDues = context_.payments().studentsWithDues();
    view.totalStudents = context_.auth().studentCount();
    view.routes = static_cast<int>(context_.routes().cache().size());

    for (const st::Route& route : context_.routes().cache()) {
        const st::RouteLoad load = context_.routes().loadFor(route.routeId());
        view.totalCapacity += route.capacity();
        view.occupiedSeats += load.occupiedCount;
        view.freeSeats += load.freeSeats();
    }
    return view;
}

ActionResult PaymentController::record(const QString& studentId, st::FeeKind bucket,
                                       long long amount, const QString& recordedBy,
                                       ReceiptView& receipt) {
    const st::PaymentReceipt result =
        context_.payments().recordPayment(toStd(studentId), bucket, amount, toStd(recordedBy));

    receipt.ok = result.ok;
    receipt.paymentId = qs(result.paymentId);
    receipt.reference = qs(result.reference);
    receipt.kindLabel = qs(st::feeKindName(bucket));
    receipt.applied = result.appliedAmount;
    receipt.outstandingAfter = result.outstandingAfter;
    receipt.message = qs(result.message);

    if (!result.ok) {
        return ActionResult::failure(QStringLiteral("Payment not recorded"), qs(result.message));
    }

    return ActionResult::success(QStringLiteral("Receipt %1 · %2 applied · %3 still outstanding")
                                     .arg(receipt.reference, money(receipt.applied),
                                          money(receipt.outstandingAfter)));
}

ActionResult PaymentController::settleBucket(const QString& studentId, st::FeeKind bucket,
                                             const QString& recordedBy, ReceiptView& receipt) {
    const st::FeeBreakdown fees = context_.payments().statementFor(toStd(studentId));
    const long long due = bucket == st::FeeKind::Registration ? fees.registrationOutstanding()
                                                             : fees.transportOutstanding();
    if (due <= 0) {
        return ActionResult::failure(QStringLiteral("Nothing to settle"),
                                     QStringLiteral("That fee is already fully paid."));
    }
    return record(studentId, bucket, due, recordedBy, receipt);
}

bool PaymentController::exportHistory(const QString& studentId, const QString& filePath,
                                      QString& message) const {
    const QVector<PaymentRow> rows = history(studentId);
    if (rows.isEmpty()) {
        message = QStringLiteral("There are no payments to export for this student.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        message = QStringLiteral("Could not open %1 for writing.").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out << "Receipt,Date,Fee type,Amount,Recorded by\n";
    for (const PaymentRow& row : rows) {
        out << row.reference << ',' << row.date << ',' << row.kind << ',' << row.amount << ','
            << QStringLiteral("\"%1\"").arg(row.recordedBy) << '\n';
    }
    out.flush();
    file.close();

    message = QStringLiteral("Exported %1 payment(s) to %2.").arg(rows.size()).arg(filePath);
    return true;
}

bool PaymentController::exportLedger(const QString& filePath, QString& message) const {
    const auto& journal = context_.auth().journal();
    if (journal.empty()) {
        message = QStringLiteral("There are no payments to export.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        message = QStringLiteral("Could not open %1 for writing.").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out << "Receipt,Date,Student,Student name,Fee type,Amount,Recorded by\n";
    for (const st::Payment& payment : journal) {
        out << qs(payment.reference()) << ',' << qs(payment.paymentDate()) << ','
            << qs(payment.studentId()) << ','
            << QStringLiteral("\"%1\"").arg(context_.studentName(payment.studentId())) << ','
            << qs(payment.kindLabel()) << ',' << payment.amount() << ','
            << QStringLiteral("\"%1\"").arg(payment.recordedBy()) << '\n';
    }
    out.flush();
    file.close();

    message = QStringLiteral("Exported %1 payment(s) to %2.").arg(journal.size()).arg(filePath);
    return true;
}

}  // namespace gui