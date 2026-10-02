#include "controllers/StaffController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

#include "controllers/RouteController.hpp"
#include "models/Admin.hpp"
#include "models/Staff.hpp"

#include <memory>

namespace gui {

StaffController::StaffController(AppContext& context) : context_(context) {}

QVector<StaffRow> StaffController::rows() const {
    QVector<StaffRow> result;
    for (const auto& user : context_.auth().userCache()) {
        const st::Staff* staff = dynamic_cast<const st::Staff*>(user.get());
        if (!staff) continue;

        StaffRow row;
        row.id = qs(staff->userId());
        row.name = qs(staff->fullName());
        row.username = qs(staff->username());
        row.phone = qs(staff->phone());
        row.routeId = qs(staff->assignedRouteId());
        row.routeName = row.routeId.isEmpty() ? QStringLiteral("Unassigned")
                                              : context_.routeName(staff->assignedRouteId());
        row.active = staff->active();
        row.administrator = false;
        result.push_back(row);
    }

    for (const auto& user : context_.auth().userCache()) {
        if (dynamic_cast<const st::Admin*>(user.get()) == nullptr) continue;

        StaffRow row;
        row.id = qs(user->userId());
        row.name = qs(user->fullName());
        row.username = qs(user->username());
        row.phone = qs(user->phone());
        row.routeId.clear();
        row.routeName = QStringLiteral("All routes");
        row.active = user->active();
        row.administrator = true;
        result.push_back(row);
    }
    return result;
}

bool StaffController::findRow(const QString& staffId, StaffRow& row) const {
    for (const StaffRow& candidate : rows()) {
        if (candidate.id == staffId) {
            row = candidate;
            return true;
        }
    }
    return false;
}

ActionResult StaffController::create(const st::RegistrationForm& form,
                                     const QString& authorizationCode) {
    const st::AuthService::RegistrationResult result =
        context_.auth().registerStaff(form, toStd(authorizationCode));
    if (!result.ok) {
        return ActionResult::failure(QStringLiteral("Could not create the account"),
                                     qs(result.message));
    }
    return ActionResult::success(
        QStringLiteral("Staff account %1 created.").arg(qs(result.username)));
}

ActionResult StaffController::assignRoute(const QString& staffId, const QString& routeId) {
    st::Staff* staff = dynamic_cast<st::Staff*>(context_.auth().findUser(toStd(staffId)));
    if (!staff) {
        return ActionResult::failure(QStringLiteral("Could not assign the route"),
                                     QStringLiteral("That staff account no longer exists."));
    }

    // Staff is abstract, so the change is made on a clone whose concrete type is
    // preserved, then written back through the repository.
    std::unique_ptr<st::User> updated = staff->clone();
    updated->setPhone(staff->phone());
    dynamic_cast<st::Staff*>(updated.get())->setAssignedRouteId(toStd(routeId));

    if (!context_.auth().persist(*updated)) {
        return ActionResult::failure(QStringLiteral("Could not assign the route"),
                                     QStringLiteral("The change could not be saved."));
    }
    return ActionResult::success(routeId.isEmpty()
                                     ? QStringLiteral("Route assignment cleared.")
                                     : QStringLiteral("Assigned to %1.").arg(routeId));
}

ActionResult StaffController::setActive(const QString& staffId, bool active) {
    st::User* user = context_.auth().findUser(toStd(staffId));
    if (!user) {
        return ActionResult::failure(QStringLiteral("Could not update the account"),
                                     QStringLiteral("That account no longer exists."));
    }

    std::unique_ptr<st::User> updated = user->clone();
    updated->setActive(active);

    if (!context_.auth().persist(*updated)) {
        return ActionResult::failure(QStringLiteral("Could not update the account"),
                                     QStringLiteral("The change could not be saved."));
    }
    return ActionResult::success(active ? QStringLiteral("Account reactivated.")
                                        : QStringLiteral("Account deactivated."));
}

ActionResult StaffController::resetPassword(const QString& staffId, const QString& newSecret) {
    std::string reason;
    if (!context_.auth().setPassword(toStd(staffId), toStd(newSecret), reason)) {
        return ActionResult::failure(QStringLiteral("Could not reset the password"),
                                     qs(reason));
    }
    return ActionResult::success(QStringLiteral("Password reset."));
}

int StaffController::count() const {
    int total = 0;
    for (const auto& user : context_.auth().userCache()) {
        if (dynamic_cast<const st::Staff*>(user.get()) != nullptr) ++total;
    }
    return total;
}

}  // namespace gui