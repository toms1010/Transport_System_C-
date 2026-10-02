#pragma once

#include "controllers/ActionResult.hpp"
#include "services/AuthService.hpp"

#include <QString>

namespace gui {

class AppContext;

class AuthController {
public:
    explicit AuthController(AppContext& context);

    ActionResult login(const QString& username, const QString& password);
    ActionResult registerStudent(const st::RegistrationForm& form);
    ActionResult registerStaff(const st::RegistrationForm& form, const QString& authorizationCode);
    ActionResult changePassword(const QString& userId, const QString& currentSecret,
                                const QString& newSecret, const QString& confirmSecret);
    ActionResult updateOwnProfile(const QString& userId, const QString& fullName,
                                   const QString& fatherName, const QString& phone,
                                   const QString& address);

    const st::Session& session() const { return session_; }
    void beginSession(const st::Session& session) { session_ = session; }
    void endSession() { session_.clear(); }

    bool isStudent() const { return session_.role == st::Role::Student; }
    bool isStaff() const { return session_.role == st::Role::Staff; }
    bool isAdmin() const { return session_.role == st::Role::Admin; }
    bool isOperator() const { return session_.role != st::Role::Student; }

    QString suggestedUsername() const;

private:
    AppContext& context_;
    st::Session session_;
};

}  // namespace gui