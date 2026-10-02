#pragma once

#include <string>

namespace st {

enum class FeeKind {
    Registration,
    Transport,
};

const char* feeKindName(FeeKind kind);
FeeKind feeKindFromName(const std::string& name);

// Per-bucket balance for a student.
//
// totalAmount() = registrationFee + transportFee
// outstanding() = totalAmount() - (registrationPaid + transportPaid)
//
// Keeping the two buckets separate is what makes partial payments correct: a
// payment can settle the registration fee in full without silently zeroing the
// transport balance (the bug in the original single-`due` field).
struct FeeBreakdown {
    long long registrationFee = 0;
    long long registrationPaid = 0;
    long long transportFee = 0;
    long long transportPaid = 0;

    long long totalAmount() const { return registrationFee + transportFee; }
    long long totalPaid() const { return registrationPaid + transportPaid; }

    long long outstanding() const {
        const long long left = totalAmount() - totalPaid();
        return left > 0 ? left : 0;
    }
    long long registrationOutstanding() const {
        const long long left = registrationFee - registrationPaid;
        return left > 0 ? left : 0;
    }
    long long transportOutstanding() const {
        const long long left = transportFee - transportPaid;
        return left > 0 ? left : 0;
    }
    bool isSettled() const { return outstanding() == 0; }

    std::string statusLabel() const;
};

}  // namespace st