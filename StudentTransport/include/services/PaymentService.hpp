#pragma once

#include "models/FeeBreakdown.hpp"
#include "models/Payment.hpp"
#include "models/Student.hpp"
#include "repositories/PaymentRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/UserService.hpp"

#include <string>
#include <vector>

namespace st {

enum class PaymentResult {
    Success,
    StudentNotFound,
    InvalidAmount,
    NoOutstanding,
    NothingLeftInBucket,
    StorageFailure,
};

const char* paymentResultMessage(PaymentResult result);

struct PaymentReceipt {
    bool ok = false;
    std::string message;
    std::string paymentId;
    std::string reference;
    long long appliedAmount = 0;
    FeeKind bucket = FeeKind::Transport;
    long long outstandingAfter = 0;
};

class PaymentService {
public:
    PaymentService(CsvPaymentRepository& paymentRepo, CsvUserRepository& userRepo,
                   AuthService& authService, ConfigService& configService)
        : paymentRepo_(paymentRepo),
          userRepo_(userRepo),
          auth_(authService),
          config_(configService) {}

    void load();

    FeeBreakdown statementFor(const std::string& studentId) const;
    std::vector<Payment> historyFor(const std::string& studentId) const;

    PaymentReceipt recordPayment(const std::string& studentId, FeeKind bucket, long long amount,
                                 const std::string& recordedBy);

    long long collectedTotal() const;
    long long outstandingTotal() const;
    int studentsWithDues() const;

private:
    CsvPaymentRepository& paymentRepo_;
    CsvUserRepository& userRepo_;
    AuthService& auth_;
    ConfigService& config_;
    std::vector<Payment> paymentCache_;
};

}  // namespace st