#pragma once

#include "models/FeeBreakdown.hpp"
#include "models/Staff.hpp"
#include "models/Student.hpp"
#include "models/User.hpp"
#include "repositories/PaymentRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/UserService.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace st {

enum class AuthOutcome {
    Success,
    UnknownUser,
    WrongPassword,
    InactiveAccount,
    EmptyInput,
};

const char* authOutcomeMessage(AuthOutcome outcome);

struct RegistrationForm {
    std::string fullName;
    std::string fatherName;
    std::string phone;
    std::string address;
    std::string username;
    std::string password;
};

struct Session {
    std::string userId;
    std::string username;
    std::string displayName;
    Role role = Role::Student;

    bool active() const { return !userId.empty(); }
    void clear() { *this = Session{}; }
};

// Owns every user record plus the payment journal, and provides the account
// operations the UI needs. The UI never touches repositories directly.
//
// AuthService is the single source of truth for users. Other services read
// through userCache() instead of keeping their own copy, which previously let
// seat/route state and fee balances drift apart between two caches.
class AuthService {
public:
    AuthService(CsvUserRepository& userRepo, CsvPaymentRepository& paymentRepo,
                ConfigService& configService)
        : userRepo_(userRepo), paymentRepo_(paymentRepo), config_(configService) {}

    using UserCache = CsvUserRepository::UserCache;

    void loadUsers();
    void loadJournal() { journal_ = paymentRepo_.loadAll(); }

    // Called by PaymentService after it appends, so balances stay in step with
    // the journal without re-reading the file on every lookup.
    void onJournalChanged() { loadJournal(); }

    AuthOutcome login(const std::string& username, const std::string& secret, Session& sessionOut);

    struct RegistrationResult {
        bool ok = false;
        std::string message;
        std::string username;
    };

    RegistrationResult registerStudent(const RegistrationForm& form);
    RegistrationResult registerStaff(const RegistrationForm& form,
                                     const std::string& authorizationCode);

    // Rebalances the student from the cached journal and returns a live pointer
    // into userCache(). The pointer is invalidated by any subsequent save().
    Student* findStudent(const std::string& userId);
    const Student* findStudent(const std::string& userId) const;

    const User* findUser(const std::string& userId) const;
    User* findUser(const std::string& userId);
    const Staff* findStaff(const std::string& userId) const;

    UserCache& userCache() { return userCache_; }
    const UserCache& userCache() const { return userCache_; }

    const std::vector<Payment>& journal() const { return journal_; }

    bool usernameExists(const std::string& username) const;
    std::string nextStudentUsername() const;
    std::string nextStaffUsername() const;

    bool setPassword(const std::string& userId, const std::string& newSecret, std::string& reason);

    // Writes a modified record back through the repository and refreshes the cache.
    bool persist(const User& user);

    int studentCount() const;
    int staffCount() const;
    int adminCount() const;

private:
    void syncBalances(Student& student) const;

    CsvUserRepository& userRepo_;
    CsvPaymentRepository& paymentRepo_;
    ConfigService& config_;
    UserCache userCache_;
    std::vector<Payment> journal_;
};

}  // namespace st