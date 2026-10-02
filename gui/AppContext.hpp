#pragma once

#include "controllers/ActionResult.hpp"
#include "models/FeeBreakdown.hpp"
#include "repositories/ComplaintRepository.hpp"
#include "repositories/PaymentRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/ComplaintService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "services/UserService.hpp"

#include <QString>
#include <QStringList>

namespace gui {

class AppContext {
public:
    AppContext();

    st::AuthService& auth() { return auth_; }
    st::RouteService& routes() { return routes_; }
    st::SeatService& seats() { return seats_; }
    st::PaymentService& payments() { return payments_; }
    st::ComplaintService& complaints() { return complaints_; }
    st::NoticeService& notices() { return notices_; }
    st::ConfigService& config() { return config_; }

    const st::CsvUserRepository& userRepo() const { return userRepo_; }
    const st::CsvRouteRepository& routeRepo() const { return routeRepo_; }

    QString institutionName() const;
    QString dataDirectory() const { return dataDir_; }
    bool administratorSeeded() const { return seededAdministrator_; }

    void refreshAll();

    // Settings-page data maintenance. Every file is copied to a timestamped
    // folder so a restore can put the whole dataset back together.
    ActionResult backupTo(const QString& directory) const;
    ActionResult restoreFrom(const QString& directory);
    QStringList dataFiles() const;

    std::vector<std::string> studentIds() const;
    QString studentName(const std::string& userId) const;
    QString routeName(const std::string& routeId) const;

private:
    void importLegacyAccounts();
    void seedAdministrator();

    QString dataDir_;

    st::ConfigService config_;
    st::CsvUserRepository userRepo_;
    st::CsvRouteRepository routeRepo_;
    st::CsvPaymentRepository paymentRepo_;
    st::CsvComplaintRepository complaintRepo_;
    st::CsvNoticeRepository noticeRepo_;
    st::AuthService auth_;
    st::RouteService routes_;
    st::SeatService seats_;
    st::PaymentService payments_;
    st::ComplaintService complaints_;
    st::NoticeService notices_;

    bool seededAdministrator_ = false;
};

}  // namespace gui