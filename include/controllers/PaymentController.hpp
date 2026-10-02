#pragma once

#include "controllers/ActionResult.hpp"

#include "models/FeeBreakdown.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

struct FeeRow {
    QString label;
    long long charge = 0;
    long long paid = 0;
    long long due = 0;
    int percent = 0;
};

struct StatementView {
    bool valid = false;
    QString studentId;
    QString studentName;
    QString username;
    QString routeName;
    QString seatLabel;
    FeeRow registration;
    FeeRow transport;
    long long totalCharge = 0;
    long long totalPaid = 0;
    long long outstanding = 0;
    QString statusLabel;
    QString statusKind;
    bool settled = false;
    bool awaitingSeat = false;
};

struct PaymentRow {
    QString id;
    QString date;
    QString kind;
    QString reference;
    QString recordedBy;
    long long amount = 0;
};

struct ReceiptView {
    bool ok = false;
    QString paymentId;
    QString reference;
    QString date;
    QString kindLabel;
    long long applied = 0;
    long long outstandingAfter = 0;
    QString message;
};

struct AggregateView {
    long long collected = 0;
    long long outstanding = 0;
    int studentsWithDues = 0;
    int totalStudents = 0;
    int routes = 0;
    int totalCapacity = 0;
    int occupiedSeats = 0;
    int freeSeats = 0;
};

class PaymentController {
public:
    explicit PaymentController(AppContext& context);

    StatementView statement(const QString& studentId) const;
    QVector<PaymentRow> history(const QString& studentId) const;
    AggregateView aggregates() const;

    ActionResult record(const QString& studentId, st::FeeKind bucket, long long amount,
                        const QString& recordedBy, ReceiptView& receipt);
    ActionResult settleBucket(const QString& studentId, st::FeeKind bucket,
                               const QString& recordedBy, ReceiptView& receipt);

    bool exportHistory(const QString& studentId, const QString& filePath, QString& message) const;
    bool exportLedger(const QString& filePath, QString& message) const;

private:
    FeeRow toRow(const QString& label, long long charge, long long paid) const;

    AppContext& context_;
};

}  // namespace gui