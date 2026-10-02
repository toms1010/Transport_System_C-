#include "controllers/AuthController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

namespace gui {

ActionResult ActionResult::success(const QString& message) {
    ActionResult result;
    result.ok = true;
    result.message = message;
    return result;
}

ActionResult ActionResult::failure(const QString& title, const QString& message) {
    ActionResult result;
    result.ok = false;
    result.title = title;
    result.message = message;
    return result;
}

namespace {

QString profileFailure(const std::string& reason) {
    return qs(reason);
}

}  // namespace

AuthController::AuthController(AppContext& context) : context_(context) {}

ActionResult AuthController::login(const QString& username, const QString& password) {
    const QString trimmed = username.trimmed();
    if (trimmed.isEmpty() || password.isEmpty()) {
        return ActionResult::failure(QStringLiteral("Sign in"),
                                     QStringLiteral("Enter both your username and password."));
    }

    st::Session session;
    const st::AuthOutcome outcome =
        context_.auth().login(toStd(trimmed), toStd(password), session);

    if (outcome != st::AuthOutcome::Success) {
        return ActionResult::failure(QStringLiteral("Sign in failed"),
                                     qs(st::authOutcomeMessage(outcome)));
    }

    session_ = session;
    return ActionResult::success(QStringLiteral("Signed in as %1").arg(qs(session.displayName)));
}

ActionResult AuthController::registerStudent(const st::RegistrationForm& form) {
    const st::AuthService::RegistrationResult result = context_.auth().registerStudent(form);
    if (!result.ok) {
        return ActionResult::failure(QStringLiteral("Could not create the account"),
                                     qs(result.message));
    }
    return ActionResult::success(
        QStringLiteral("Student account %1 is ready. Sign in to pick a seat.").arg(qs(result.username)));
}

ActionResult AuthController::registerStaff(const st::RegistrationForm& form,
                                           const QString& authorizationCode) {
    const st::AuthService::RegistrationResult result =
        context_.auth().registerStaff(form, toStd(authorizationCode));
    if (!result.ok) {
        return ActionResult::failure(QStringLiteral("Could not create the account"),
                                     qs(result.message));
    }
    return ActionResult::success(
        QStringLiteral("Staff account %1 is ready.").arg(qs(result.username)));
}

ActionResult AuthController::changePassword(const QString& userId, const QString& currentSecret,
                                            const QString& newSecret,
                                            const QString& confirmSecret) {
    if (newSecret != confirmSecret) {
        return ActionResult::failure(QStringLiteral("Password"),
                                     QStringLiteral("The two new passwords do not match."));
    }
    if (newSecret.isEmpty()) {
        return ActionResult::failure(QStringLiteral("Password"),
                                     QStringLiteral("Enter a new password."));
    }

    // The current secret is verified by attempting a login against the same
    // record, so the storage layer is never asked to read back a plaintext
    // password that it does not have.
    const st::User* user = context_.auth().findUser(toStd(userId));
    if (!user) {
        return ActionResult::failure(QStringLiteral("Password"),
                                     QStringLiteral("That account no longer exists."));
    }

    st::Session probe;
    const st::AuthOutcome outcome =
        context_.auth().login(user->username(), toStd(currentSecret), probe);
    if (outcome != st::AuthOutcome::Success) {
        return ActionResult::failure(QStringLiteral("Password"),
                                     QStringLiteral("Your current password is not correct."));
    }

    std::string reason;
    if (!context_.auth().setPassword(toStd(userId), toStd(newSecret), reason)) {
        return ActionResult::failure(QStringLiteral("Password"), qs(reason));
    }
    return ActionResult::success(QStringLiteral("Your password has been updated."));
}

ActionResult AuthController::updateOwnProfile(const QString& userId, const QString& fullName,
                                              const QString& fatherName, const QString& phone,
                                              const QString& address) {
    st::User* user = context_.auth().findUser(toStd(userId));
    if (!user) {
        return ActionResult::failure(QStringLiteral("Profile"),
                                     QStringLiteral("That account no longer exists."));
    }

    // User is abstract, so the edit is made on a clone. The concrete type
    // survives clone(), which is why storing users polymorphically matters here.
    std::unique_ptr<st::User> updated = user->clone();
    if (!updated) {
        return ActionResult::failure(QStringLiteral("Profile"),
                                     QStringLiteral("That record could not be edited."));
    }

    updated->setFullName(toStd(fullName.trimmed()));
    updated->setFatherName(toStd(fatherName.trimmed()));
    updated->setPhone(toStd(phone.trimmed()));
    updated->setAddress(toStd(address.trimmed()));

    if (!context_.auth().persist(*updated)) {
        return ActionResult::failure(QStringLiteral("Profile"),
                                     profileFailure("The change could not be saved."));
    }
    return ActionResult::success(QStringLiteral("Your details have been updated."));
}

QString AuthController::suggestedUsername() const {
    if (session_.role == st::Role::Staff) return qs(context_.auth().nextStaffUsername());
    return qs(context_.auth().nextStudentUsername());
}

}  // namespace gui