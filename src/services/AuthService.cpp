#include "services/AuthService.hpp"

#include "models/Admin.hpp"
#include "models/Payment.hpp"
#include "utils/FileUtils.hpp"

#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace st {
namespace {

std::string nextIdWithPrefix(const std::vector<std::unique_ptr<User>>& users, const char* prefix) {
    int maxId = 0;
    const std::size_t len = std::strlen(prefix);
    for (const auto& u : users) {
        if (util::startsWith(u->userId(), prefix)) {
            long long v = 0;
            util::parseWholeNumber(u->userId().substr(len), v);
            if (v > maxId) maxId = static_cast<int>(v > 0 ? v : 0);
        }
    }
    std::ostringstream out;
    out << prefix << std::setw(4) << std::setfill('0') << (maxId + 1);
    return out.str();
}

std::string nextNumberedName(const std::vector<std::unique_ptr<User>>& users, const char* prefix) {
    const std::size_t len = std::strlen(prefix);
    int maxId = 0;
    for (const auto& u : users) {
        if (util::startsWith(u->username(), prefix)) {
            long long v = 0;
            util::parseWholeNumber(u->username().substr(len), v);
            if (v > maxId) maxId = static_cast<int>(v > 0 ? v : 0);
        }
    }
    std::ostringstream out;
    out << prefix << std::setw(3) << std::setfill('0') << (maxId + 1);
    return out.str();
}

}

void AuthService::loadUsers() {
    userCache_ = userRepo_.loadAll();
}

const char* authOutcomeMessage(AuthOutcome outcome) {
    switch (outcome) {
        case AuthOutcome::Success: return "Login successful.";
        case AuthOutcome::UnknownUser: return "No account matches that username.";
        case AuthOutcome::WrongPassword: return "Incorrect password.";
        case AuthOutcome::InactiveAccount: return "This account has been deactivated.";
        case AuthOutcome::EmptyInput: return "Username and password are both required.";
        default: return "Login failed.";
    }
}

AuthOutcome AuthService::login(const std::string& username, const std::string& secret,
                               Session& sessionOut) {
    const std::string uname = util::trim(username);
    if (uname.empty() || secret.empty()) {
        sessionOut.clear();
        return AuthOutcome::EmptyInput;
    }

    for (const auto& user : userCache_) {
        if (user->username() != uname) continue;
        if (!user->active()) {
            sessionOut.clear();
            return AuthOutcome::InactiveAccount;
        }
        const VerifyResult result =
            Security::verifyPassword(secret, user->salt(), user->passwordHash());
        if (result != VerifyResult::Ok) {
            sessionOut.clear();
            return AuthOutcome::WrongPassword;
        }
        sessionOut.userId = user->userId();
        sessionOut.username = user->username();
        sessionOut.displayName = user->displayName();
        sessionOut.role = user->role();
        return AuthOutcome::Success;
    }

    sessionOut.clear();
    return AuthOutcome::UnknownUser;
}

AuthService::RegistrationResult AuthService::registerStudent(const RegistrationForm& form) {
    RegistrationResult result;

    std::string reason;
    if (!validate::fullName(form.fullName, reason)) {
        result.message = reason;
        return result;
    }
    if (!validate::username(form.username, reason)) {
        result.message = reason;
        return result;
    }
    if (!validate::phone(form.phone, reason)) {
        result.message = reason;
        return result;
    }
    if (!validate::address(form.address, reason)) {
        result.message = reason;
        return result;
    }
    if (!Security::checkPolicy(form.password, reason)) {
        result.message = reason;
        return result;
    }
    if (usernameExists(form.username)) {
        result.message = "That username is already taken.";
        return result;
    }

    Student student(nextIdWithPrefix(userCache_, "STU"), form.username, form.fullName);
    student.setFatherName(form.fatherName);
    student.setPhone(form.phone);
    student.setAddress(form.address);
    student.setCreatedAt(util::nowStamp());
    student.setRegistrationFee(config_.config().registrationFee);
    student.setTransportFee(0);  // set when a seat is allotted

    std::string salt;
    std::string digest;
    Security::hashPassword(form.password, salt, digest);
    student.setSalt(salt);
    student.setPasswordHash(digest);

    const bool stored = userRepo_.save(student, userCache_);
    result.ok = stored;
    result.username = student.username();
    result.message = stored ? "Student account created." : "Could not save the new account.";
    return result;
}

AuthService::RegistrationResult AuthService::registerStaff(const RegistrationForm& form,
                                                          const std::string& authorizationCode) {
    RegistrationResult result;

    std::string reason;
    if (!validate::fullName(form.fullName, reason)) {
        result.message = reason;
        return result;
    }
    if (!validate::username(form.username, reason)) {
        result.message = reason;
        return result;
    }
    if (!validate::phone(form.phone, reason)) {
        result.message = reason;
        return result;
    }
    const std::string expected = config_.config().staffAuthorizationCode;
    if (util::trim(authorizationCode).empty() ||
        !Security::constantTimeEquals(util::trim(authorizationCode), expected)) {
        result.message = "Staff registration requires a valid authorization code.";
        return result;
    }
    if (!Security::checkPolicy(form.password, reason)) {
        result.message = reason;
        return result;
    }
    if (usernameExists(form.username)) {
        result.message = "That username is already taken.";
        return result;
    }

    Staff staff(nextIdWithPrefix(userCache_, "STF"), form.username, form.fullName);
    staff.setFatherName(form.fatherName);
    staff.setPhone(form.phone);
    staff.setAddress(form.address);
    staff.setCreatedAt(util::nowStamp());

    std::string salt;
    std::string digest;
    Security::hashPassword(form.password, salt, digest);
    staff.setSalt(salt);
    staff.setPasswordHash(digest);

    const bool stored = userRepo_.save(staff, userCache_);
    result.ok = stored;
    result.username = staff.username();
    result.message = stored ? "Staff account created." : "Could not save the new account.";
    return result;
}

void AuthService::syncBalances(Student& student) const {
    long long regPaid = 0;
    long long trnPaid = 0;
    for (const Payment& p : journal_) {
        if (p.studentId() != student.userId()) continue;
        if (p.kind() == FeeKind::Registration) regPaid += p.amount();
        else trnPaid += p.amount();
    }
    // A journal can outlive a fare change, so the running balances are clamped
    // to the current fees. Otherwise totalPaid() could exceed totalAmount().
    student.setRegistrationPaid(regPaid);
    student.setTransportPaid(trnPaid);
    student.clampPaidToFees();
}

const User* AuthService::findUser(const std::string& userId) const {
    for (const auto& user : userCache_) {
        if (user->userId() == userId) return user.get();
    }
    return nullptr;
}

Student* AuthService::findStudent(const std::string& userId) {
    for (auto& user : userCache_) {
        auto* student = dynamic_cast<Student*>(user.get());
        if (!student || student->userId() != userId) continue;
        syncBalances(*student);
        return student;
    }
    return nullptr;
}

const Student* AuthService::findStudent(const std::string& userId) const {
    return const_cast<AuthService*>(this)->findStudent(userId);
}

const Staff* AuthService::findStaff(const std::string& userId) const {
    const User* user = findUser(userId);
    return dynamic_cast<const Staff*>(user);
}

User* AuthService::findUser(const std::string& userId) {
    for (auto& user : userCache_) {
        if (user->userId() == userId) return user.get();
    }
    return nullptr;
}

int AuthService::studentCount() const {
    int n = 0;
    for (const auto& u : userCache_) {
        if (dynamic_cast<const Student*>(u.get())) ++n;
    }
    return n;
}

int AuthService::staffCount() const {
    int n = 0;
    for (const auto& u : userCache_) {
        if (dynamic_cast<const Staff*>(u.get())) ++n;
    }
    return n;
}

int AuthService::adminCount() const {
    int n = 0;
    for (const auto& u : userCache_) {
        if (dynamic_cast<const Admin*>(u.get())) ++n;
    }
    return n;
}

bool AuthService::usernameExists(const std::string& username) const {
    for (const auto& u : userCache_) {
        if (u->username() == username) return true;
    }
    return false;
}

std::string AuthService::nextStudentUsername() const {
    return nextNumberedName(userCache_, "STU");
}

std::string AuthService::nextStaffUsername() const {
    return nextNumberedName(userCache_, "STF");
}

bool AuthService::persist(const User& user) { return userRepo_.update(user, userCache_); }

bool AuthService::setPassword(const std::string& userId, const std::string& newSecret,
                              std::string& reason) {
    if (!Security::checkPolicy(newSecret, reason)) return false;
    for (auto& user : userCache_) {
        if (user->userId() != userId) continue;
        std::string salt;
        std::string digest;
        Security::hashPassword(newSecret, salt, digest);
        user->setSalt(salt);
        user->setPasswordHash(digest);
        return persist(*user);
    }
    reason = "Account not found.";
    return false;
}

}  // namespace st