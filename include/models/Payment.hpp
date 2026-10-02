#pragma once

#include "models/FeeBreakdown.hpp"

#include <string>

namespace st {

// Immutable record of a single money movement.
//
// The original project stored only a `paid` flag, which made partial payment
// and payment history impossible. Every payment is now journalled here and the
// student's running balances are derived from the sum of these rows.
class Payment {
public:
    Payment() = default;

    Payment(std::string paymentId, std::string studentId, long long amount, FeeKind kind,
            std::string paymentDate, std::string reference, std::string recordedBy)
        : paymentId_(std::move(paymentId)),
          studentId_(std::move(studentId)),
          amount_(amount),
          kind_(kind),
          paymentDate_(std::move(paymentDate)),
          reference_(std::move(reference)),
          recordedBy_(std::move(recordedBy)) {}

    const std::string& paymentId() const { return paymentId_; }
    const std::string& studentId() const { return studentId_; }
    long long amount() const { return amount_; }
    FeeKind kind() const { return kind_; }
    const std::string& paymentDate() const { return paymentDate_; }
    const std::string& reference() const { return reference_; }
    const std::string& recordedBy() const { return recordedBy_; }

    void setPaymentId(std::string v) { paymentId_ = std::move(v); }
    void setStudentId(std::string v) { studentId_ = std::move(v); }
    void setAmount(long long v) { amount_ = v; }
    void setKind(FeeKind v) { kind_ = v; }
    void setPaymentDate(std::string v) { paymentDate_ = std::move(v); }
    void setReference(std::string v) { reference_ = std::move(v); }
    void setRecordedBy(std::string v) { recordedBy_ = std::move(v); }

    std::string kindLabel() const { return feeKindName(kind_); }

private:
    std::string paymentId_;
    std::string studentId_;
    long long amount_ = 0;
    FeeKind kind_ = FeeKind::Transport;
    std::string paymentDate_;
    std::string reference_;
    std::string recordedBy_;
};

}  // namespace st