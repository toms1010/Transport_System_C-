#pragma once

#include "controllers/ActionResult.hpp"
#include "services/AuthService.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

struct StaffRow {
    QString id;
    QString name;
    QString username;
    QString phone;
    QString routeId;
    QString routeName;
    bool active = true;
    bool administrator = false;
};

class StaffController {
public:
    explicit StaffController(AppContext& context);

    QVector<StaffRow> rows() const;
    bool findRow(const QString& staffId, StaffRow& row) const;

    ActionResult create(const st::RegistrationForm& form, const QString& authorizationCode);
    ActionResult assignRoute(const QString& staffId, const QString& routeId);
    ActionResult setActive(const QString& staffId, bool active);
    ActionResult resetPassword(const QString& staffId, const QString& newSecret);

    int count() const;

private:
    AppContext& context_;
};

}  // namespace gui
