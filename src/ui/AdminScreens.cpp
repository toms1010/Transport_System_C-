#include "ui/Screens.hpp"

#include "services/ComplaintService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "services/UserService.hpp"
#include "ui/ConsoleUI.hpp"
#include "utils/TextUtils.hpp"

#include <iostream>

namespace st {
namespace ui {
namespace {

std::string row(const std::string& label, const std::string& value) {
    std::string left = "  " + label;
    while (left.size() < 24) left.push_back(' ');
    return left + value;
}

}

void Screens::noticeBoard(const Session& session, bool canEdit) {
    if (canEdit && session.active()) {
        staffNotices(session, true);
        return;
    }

    banner("NOTICE BOARD", config_.config().tagline());
    const std::vector<Notice>& all = notices_.cache();
    if (all.empty()) {
        note("No notices have been published yet.", StatusKind::Neutral);
        pause();
        return;
    }

    std::vector<std::string> ids;
    for (auto it = all.rbegin(); it != all.rend(); ++it) {
        ids.push_back(it->noticeId() + "  " + it->title());
    }
    const int pick = menu("Select a notice", ids);
    if (pick < 0) return;

    const Notice* n = notices_.find(all[all.size() - 1 - static_cast<std::size_t>(pick)].noticeId());
    if (!n) {
        note("Notice not found.", StatusKind::Error);
        pause();
        return;
    }
    banner(n->noticeId(), n->title());
    field("Author", n->authorId());
    field("Created", n->createdAt());
    section("Content");
    std::cout << "  " << n->content() << "\n";
    pause();
}

void Screens::adminDashboard(const Session& session) {
    while (true) {
        banner("ADMIN DASHBOARD", session.displayName);

        MenuOptions options;
        options.context.push_back(row("Students", std::to_string(auth_.studentCount())));
        options.context.push_back(row("Staff accounts", std::to_string(auth_.staffCount())));
        options.context.push_back(row("Administrators", std::to_string(auth_.adminCount())));
        options.context.push_back(row("Routes", std::to_string(routes_.cache().size())));
        options.context.push_back(row("Open complaints", std::to_string(complaints_.openCount())));
        options.context.push_back(
            row("Outstanding", "Rs " + util::money(payments_.outstandingTotal())));
        options.context.push_back(
            row("Collected", "Rs " + util::money(payments_.collectedTotal())));

        const std::vector<std::string> items = {
            "Student Roster",   "Staff Roster",      "Route Management", "Seat Allocation",
            "Payments & Dues",  "Complaints",        "Notice Board",     "Reports",
            "Help / Contact",   "Logout",
        };

        const int choice = menu("Admin Menu", items, options);
        switch (choice) {
            case 0: staffRoster(session, ""); break;
            case 1: {
                banner("STAFF ACCOUNTS", session.displayName);
                std::vector<std::vector<std::string>> rows;
                for (const auto& user : auth_.userCache()) {
                    const auto* s = dynamic_cast<const Staff*>(user.get());
                    if (!s) continue;
                    rows.push_back({s->userId(), s->username(), s->fullName(), s->phone(),
                                    s->assignedRouteId().empty() ? "-" : s->assignedRouteId(),
                                    s->active() ? "ACTIVE" : "INACTIVE"});
                }
                table({"ID", "USERNAME", "NAME", "PHONE", "ROUTE", "STATUS"}, {9, 12, 24, 13, 8, 9},
                      rows);
                pause();
                break;
            }
            case 2: staffRouteManagement(session); break;
            case 3: staffSeatAllocation(session); break;
            case 4: staffPayments(session); break;
            case 5: staffComplaints(session); break;
            case 6: noticeBoard(session, true); break;
            case 7: staffReports(session); break;
            case 8: helpContact(); break;
            case 9:
                note("Logged out. Goodbye, " + session.displayName + ".", StatusKind::Ok);
                pause();
                return;
            default: return;
        }
    }
}

}  // namespace ui
}  // namespace st