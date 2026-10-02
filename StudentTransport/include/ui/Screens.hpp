#pragma once

#include "services/AuthService.hpp"
#include "services/ComplaintService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "services/UserService.hpp"
#include "ui/Menu.hpp"

namespace st {
namespace ui {

class Screens {
public:
    Screens(AuthService& auth, RouteService& routes, SeatService& seats, PaymentService& payments,
            ComplaintService& complaints, NoticeService& notices, ConfigService& config)
        : auth_(auth),
          routes_(routes),
          seats_(seats),
          payments_(payments),
          complaints_(complaints),
          notices_(notices),
          config_(config) {}

    void run();

private:
    int mainMenu();
    void registrationFlow();
    void loginFlow(Role expectedRole);
    void studentDashboard(const Session& session);
    void staffDashboard(const Session& session);
    void adminDashboard(const Session& session);

    void studentTransport(const Session& session);
    void studentRouteSeat(const Session& session);
    void studentPayments(const Session& session);
    void studentAllotmentCard(const Session& session);
    void studentComplaints(const Session& session);
    void studentProfile(const Session& session);
    void changePassword(const Session& session);

    std::string assignedRouteOf(const std::string& userId) const;

    void staffRoster(const Session& session, const std::string& routeFilter);
    void staffAssignRoute(const Session& session);
    void staffRouteManagement(const Session& session);
    void staffSeatAllocation(const Session& session);
    void staffPayments(const Session& session);
    void staffComplaints(const Session& session);
    void staffNotices(const Session& session, bool canEdit);
    void staffReports(const Session& session);

    void viewRoutes();
    void noticeBoard(const Session& session, bool canEdit);
    void helpContact();

    void showSeatMap(const std::string& routeId, const std::string& viewerId);
    bool requireStudent(const Session& session, Student*& student);

    // Shared by the student payment flow; kept here so the amount parser exists once.
    static int askAmountIn(const std::string& prompt, long long maximum);

    AuthService& auth_;
    RouteService& routes_;
    SeatService& seats_;
    PaymentService& payments_;
    ComplaintService& complaints_;
    NoticeService& notices_;
    ConfigService& config_;
};

}  // namespace ui
}  // namespace st