#include "services/PaymentService.hpp"

#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>

namespace st {

const char* paymentResultMessage(PaymentResult result) {
    switch (result) {
        case PaymentResult::Success: return "Payment recorded.";
        case PaymentResult::StudentNotFound: return "Student record not found.";
        case PaymentResult::InvalidAmount: return "Amount must be greater than zero.";
        case PaymentResult::NoOutstanding: return "This account has no outstanding balance.";
        case PaymentResult::NothingLeftInBucket:
            return "The selected bucket is already fully paid. Choose the other one.";
        case PaymentResult::StorageFailure: return "Could not save the payment.";
        default: return "Payment failed.";
    }
}

void PaymentService::load() {
    paymentCache_ = paymentRepo_.loadAll();
}

FeeBreakdown PaymentService::statementFor(const std::string& studentId) const {
    FeeBreakdown breakdown;
    const Student* student = auth_.findStudent(studentId);
    if (!student) return breakdown;
    return student->breakdown();
}

std::vector<Payment> PaymentService::historyFor(const std::string& studentId) const {
    std::vector<Payment> history = paymentRepo_.forStudent(paymentCache_, studentId);
    std::sort(history.begin(), history.end(),
              [](const Payment& a, const Payment& b) { return a.paymentDate() < b.paymentDate(); });
    return history;
}

PaymentReceipt PaymentService::recordPayment(const std::string& studentId, FeeKind bucket,
                                            long long amount, const std::string& recordedBy) {
    PaymentReceipt receipt;

    std::string reason;
    if (!validate::positiveAmount(amount, reason)) {
        receipt.message = reason;
        return receipt;
    }

    Student* student = auth_.findStudent(studentId);
    if (!student) {
        receipt.message = paymentResultMessage(PaymentResult::StudentNotFound);
        return receipt;
    }

    const FeeBreakdown before = student->breakdown();
    if (before.outstanding() == 0) {
        receipt.message = paymentResultMessage(PaymentResult::NoOutstanding);
        return receipt;
    }

    const long long bucketOutstanding =
        (bucket == FeeKind::Registration) ? before.registrationOutstanding()
                                          : before.transportOutstanding();
    if (bucketOutstanding <= 0) {
        receipt.message = paymentResultMessage(PaymentResult::NothingLeftInBucket);
        return receipt;
    }

    const long long applied = std::min(amount, bucketOutstanding);

    Payment payment(paymentRepo_.nextId(paymentCache_), studentId, applied, bucket, util::nowStamp(),
                    Security::makeReference("RCPT"), recordedBy);
    if (!paymentRepo_.append(payment, paymentCache_)) {
        receipt.message = paymentResultMessage(PaymentResult::StorageFailure);
        return receipt;
    }

    // AuthService rebalances students from its own journal snapshot, so it must
    // be told the moment the journal grows. Without this, findStudent() would
    // keep rehydrating the pre-payment balances and every total would read 0.
    auth_.onJournalChanged();

    if (bucket == FeeKind::Registration) {
        student->addRegistrationPaid(applied);
    } else {
        student->addTransportPaid(applied);
    }
    if (!userRepo_.update(*student, auth_.userCache())) {
        paymentRepo_.removeById(payment.paymentId(), paymentCache_);
        if (Student* restored = auth_.findStudent(studentId)) {
            if (bucket == FeeKind::Registration) {
                restored->setRegistrationPaid(before.registrationPaid);
            } else {
                restored->setTransportPaid(before.transportPaid);
            }
            userRepo_.update(*restored, auth_.userCache());
        }
        receipt.message = paymentResultMessage(PaymentResult::StorageFailure);
        return receipt;
    }

    if (applied < amount) {
        receipt.message = "Recorded Rs " + util::money(applied) + " (capped at the outstanding " +
                          std::string(bucket == FeeKind::Registration ? "registration" : "transport") +
                          " balance of Rs " + util::money(bucketOutstanding) + ").";
    }

    receipt.ok = true;
    receipt.paymentId = payment.paymentId();
    receipt.reference = payment.reference();
    receipt.appliedAmount = applied;
    receipt.bucket = bucket;
    const Student* reloaded = auth_.findStudent(studentId);
    receipt.outstandingAfter = reloaded ? reloaded->outstanding() : before.outstanding() - applied;
    return receipt;
}

long long PaymentService::collectedTotal() const {
    long long total = 0;
    for (const Payment& p : paymentCache_) total += p.amount();
    return total;
}

long long PaymentService::outstandingTotal() const {
    long long total = 0;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (!s) continue;
        total += s->outstanding();
    }
    return total;
}

int PaymentService::studentsWithDues() const {
    int count = 0;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (s && s->outstanding() > 0) ++count;
    }
    return count;
}

}  // namespace st