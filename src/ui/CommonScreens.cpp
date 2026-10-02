#include "ui/Screens.hpp"

#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "ui/ConsoleUI.hpp"
#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"

#include <iostream>

namespace st {
namespace ui {

int Screens::askAmountIn(const std::string& prompt, long long maximum) {
    const int cap = maximum > 0 && maximum < 100000000LL ? static_cast<int>(maximum)
                                                         : 100000000;
    while (true) {
        const std::string typed = readLineField(prompt);
        int amount = 0;
        if (typed.empty()) continue;
        if (!util::parseIntInRange(typed, 1, cap, amount)) {
            note("Enter a whole number between 1 and " + std::to_string(cap) + ".",
                 StatusKind::Error);
            continue;
        }
        return amount;
    }
}

bool Screens::requireStudent(const Session& session, Student*& student) {
    student = auth_.findStudent(session.userId);
    if (!student) {
        note("Your student record could not be loaded.", StatusKind::Error);
        pause();
        return false;
    }
    return true;
}

void Screens::showSeatMap(const std::string& routeId, const std::string& viewerId) {
    const Route* route = routes_.find(routeId);
    if (!route) {
        note("Route " + routeId + " does not exist.", StatusKind::Error);
        return;
    }
    const RouteLoad load = routes_.loadFor(routeId);

    pageHeader("Seat Map", route->label());
    std::cout << "\n  " << paint(Color::Bold, route->endpoints()) << "\n";
    field("Via", route->via(), 12);
    field("Fare", "Rs " + util::money(route->fare()), 12);
    field("Occupancy",
          load.occupancyLabel() + "   (" + std::to_string(load.freeSeats()) + " seats available)", 12);

    const std::vector<Seat> seats = seats_.seatMap(routeId, viewerId);
    std::cout << "\n  " << paint(Color::Bold, "FRONT OF BUS") << "\n\n";

    int column = 0;
    for (const Seat& seat : seats) {
        switch (seat.statusFor(viewerId)) {
            case SeatStatus::Selected:
                std::cout << "  " << paint(Color::Green, seat.markerFor(viewerId));
                break;
            case SeatStatus::Occupied:
                std::cout << "  " << paint(Color::Yellow, seat.markerFor(viewerId));
                break;
            case SeatStatus::Available:
            default:
                std::cout << "  " << paint(Color::Dim, seat.markerFor(viewerId));
                break;
        }
        if (++column % 6 == 0) std::cout << "\n";
    }
    if (!seats.empty() && static_cast<int>(seats.size()) % 6 != 0) std::cout << "\n";
    std::cout << "\n  " << paint(Color::Dim, "[ n ] " + std::string(seatStatusName(SeatStatus::Available)))
              << "   " << paint(Color::Yellow, "[X ] " + std::string(seatStatusName(SeatStatus::Occupied)))
              << "   "
              << paint(Color::Green, "[n]\xE2\x9C\x93 " + std::string(seatStatusName(SeatStatus::Selected)))
              << "\n";
}

void Screens::viewRoutes() {
    banner("ROUTES & FARES", config_.config().tagline());

    const std::vector<Route>& all = routes_.cache();
    if (all.empty()) {
        note("No routes are configured yet.", StatusKind::Warning);
        pause();
        return;
    }

    std::vector<std::vector<std::string>> rows;
    for (const Route& r : all) {
        const RouteLoad load = routes_.loadFor(r.routeId());
        std::vector<std::string> row;
        row.push_back(r.routeId());
        row.push_back(r.endpoints());
        row.push_back(r.via());
        row.push_back("Rs " + util::money(r.fare()));
        row.push_back(load.occupancyLabel());
        row.push_back(std::to_string(load.freeSeats()));
        rows.push_back(row);
    }

    table({"ID", "PATH", "VIA", "FARE", "SEATS", "FREE"}, {5, 27, 20, 11, 8, 5}, rows);
    note("Registration fee of Rs " + util::money(config_.config().registrationFee) +
             " applies to every new student.",
         StatusKind::Neutral);
    pause();
}

void Screens::helpContact() {
    const SystemConfig& cfg = config_.config();

    banner("HELP & CONTACT", cfg.departmentName);
    field("Institution", cfg.institutionName + ", " + cfg.institutionCity);
    field("Office", cfg.contactRoom);
    field("Transport In-charge", cfg.contactName);
    field("Designation", cfg.contactDesignation);
    field("Phone", cfg.contactPhone);
    field("Office hours", cfg.contactHours);
    std::cout << "\n";
    note("Complaints raised in the portal are tracked to resolution by staff.", StatusKind::Info);
    pause();
}

void Screens::changePassword(const Session& session) {
    banner("CHANGE PASSWORD", session.username);

    const std::string current = readSecretField("Current password");
    Session probe;
    if (auth_.login(session.username, current, probe) != AuthOutcome::Success) {
        note("Current password is incorrect.", StatusKind::Error);
        pause();
        return;
    }

    while (true) {
        const std::string next = readSecretField("New password");
        std::string reason;
        if (!Security::checkPolicy(next, reason)) {
            note(reason, StatusKind::Error);
            continue;
        }
        const std::string again = readSecretField("Confirm new password");
        if (again != next) {
            note("Passwords do not match.", StatusKind::Error);
            continue;
        }
        if (!auth_.setPassword(session.userId, next, reason)) {
            note(reason, StatusKind::Error);
            pause();
            return;
        }
        note("Password updated.", StatusKind::Ok);
        pause();
        return;
    }
}

}  // namespace ui
}  // namespace st