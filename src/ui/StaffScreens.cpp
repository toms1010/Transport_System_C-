#include "ui/Screens.hpp"

#include "services/ComplaintService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "services/UserService.hpp"
#include "ui/ConsoleUI.hpp"
#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>
#include <iostream>

namespace st {
namespace ui {
namespace {

}

std::string row(const std::string& label, const std::string& value) {
    std::string left = "  " + label;
    while (left.size() < 24) left.push_back(' ');
    return left + value;
}

std::string Screens::assignedRouteOf(const std::string& userId) const {
    const Staff* staff = auth_.findStaff(userId);
    return staff ? staff->assignedRouteId() : std::string();
}

void Screens::staffDashboard(const Session& session) {
    while (true) {
        banner("STAFF DASHBOARD", session.displayName);
        const std::string assigned = assignedRouteOf(session.userId);
        const Route* route = assigned.empty() ? nullptr : routes_.find(assigned);

        MenuOptions options;
        options.context.push_back(row("Staff ID", session.userId));
        options.context.push_back(row("Assigned route",
                                      assigned.empty() ? "none assigned yet" : route->label()));

        const int studentCount = static_cast<int>(routes_.studentCountOnRoute(assigned));
        const int dueCount = payments_.studentsWithDues();

        options.context.push_back("");
        options.context.push_back("  TRANSPORT MANAGEMENT OVERVIEW");
        options.context.push_back(row("Students on my route",
                                      std::to_string(assigned.empty() ? 0 : studentCount)));
        options.context.push_back(row("Active routes", std::to_string(routes_.cache().size())));
        options.context.push_back(row("Open complaints", std::to_string(complaints_.openCount())));
        options.context.push_back(row("Students with dues", std::to_string(dueCount)));
        options.context.push_back(
            row("Total outstanding", "Rs " + util::money(payments_.outstandingTotal())));

        const std::vector<std::string> items = {
            "My Route Assignment",
            "Student Roster",
            "Route Management",
            "Seat Allocation",
            "Payments & Dues",
            "Complaints",
            "Notice Board",
            "Reports",
            "Help / Contact",
            "Logout",
        };

        const int choice = menu("Staff Menu", items, options);
        switch (choice) {
            case 0: staffAssignRoute(session); break;
            case 1: staffRoster(session, assignedRouteOf(session.userId)); break;
            case 2: staffRouteManagement(session); break;
            case 3: staffSeatAllocation(session); break;
            case 4: staffPayments(session); break;
            case 5: staffComplaints(session); break;
            case 6: staffNotices(session, true); break;
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

void Screens::staffRoster(const Session& session, const std::string& routeFilter) {
    banner("STUDENT ROSTER", session.displayName);

    std::vector<const Student*> students;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (!s) continue;
        if (!routeFilter.empty() && s->routeId() != routeFilter) continue;
        students.push_back(s);
    }
    std::sort(students.begin(), students.end(),
              [](const Student* a, const Student* b) { return a->userId() < b->userId(); });

    if (students.empty()) {
        note(routeFilter.empty() ? "No students are registered yet."
                                 : "No students are allotted to route " + routeFilter + ".",
             StatusKind::Warning);
        pause();
        return;
    }

    std::vector<std::vector<std::string>> rows;
    long long outstanding = 0;
    for (const Student* s : students) {
        outstanding += s->outstanding();
        rows.push_back({s->userId(), s->username(), s->fullName(), s->phone(),
                        s->routeId().empty() ? "-" : s->routeId(),
                        s->seatNumber() > 0 ? std::to_string(s->seatNumber()) : "-",
                        "Rs " + util::money(s->outstanding()),
                        s->fullyPaid() ? "PAID" : "PENDING"});
    }

    table({"ID", "USERNAME", "NAME", "PHONE", "ROUTE", "SEAT", "OUTSTANDING", "STATUS"},
          {9, 10, 22, 13, 7, 6, 14, 9}, rows);

    field("Students shown", std::to_string(students.size()));
    field("Outstanding", "Rs " + util::money(outstanding));
    pause();
}

void Screens::staffAssignRoute(const Session& session) {
    while (true) {
        banner("MY ROUTE ASSIGNMENT", session.displayName);
        const std::string current = assignedRouteOf(session.userId);
        const Route* mine = current.empty() ? nullptr : routes_.find(current);

        field("Current assignment",
              mine ? mine->label() : (current.empty() ? "none yet" : current + " (missing)"));

        std::vector<std::string> items;
        std::vector<std::string> ids;
        for (const Route& r : routes_.cache()) {
            RouteLoad load = routes_.loadFor(r.routeId());
            items.push_back(r.label() + "   " + paint(Color::Dim, load.occupancyLabel()));
            ids.push_back(r.routeId());
        }
        items.push_back("Clear my assignment");

        const int pick = menu("Choose the route you are responsible for", items);
        if (pick < 0) return;

        Staff* staff = const_cast<Staff*>(
            dynamic_cast<const Staff*>(auth_.findUser(session.userId)));
        if (!staff) {
            note("Your staff record could not be loaded.", StatusKind::Error);
            pause();
            return;
        }

        if (pick == static_cast<int>(ids.size())) {
            staff->setAssignedRouteId("");
        } else {
            const std::string routeId = ids[static_cast<std::size_t>(pick)];
            const Route* r = routes_.find(routeId);
            std::cout << "\n";
            field("Route", r->label());
            field("Via", r->via());
            field("Students", std::to_string(routes_.studentCountOnRoute(routeId)), 12);
            if (!confirm("Take responsibility for route " + routeId + "?")) continue;
            staff->setAssignedRouteId(routeId);
        }

        if (!auth_.persist(*staff)) {
            note("Could not save the assignment.", StatusKind::Error);
            pause();
            return;
        }
        note("Assignment updated.", StatusKind::Ok);
        pause();
    }
}

void Screens::staffRouteManagement(const Session& session) {
    while (true) {
        banner("ROUTE MANAGEMENT", session.displayName);

        std::vector<std::vector<std::string>> rows;
        for (const Route& r : routes_.cache()) {
            const RouteLoad load = routes_.loadFor(r.routeId());
            rows.push_back({r.routeId(), r.endpoints(), "Rs " + util::money(r.fare()),
                            load.occupancyLabel(), r.active() ? "ACTIVE" : "INACTIVE"});
        }
        table({"ID", "PATH", "FARE", "OCCUPIED", "STATUS"}, {6, 30, 12, 10, 10}, rows);

        const std::vector<std::string> items = {"Add a route", "Edit a route", "Delete a route",
                                                "Back"};
        const int choice = menu("Route actions", items);
        if (choice < 0 || choice == 3) return;

        if (choice == 0) {
            std::cout << "\n";
            Route r;
            std::string reason;
            while (true) {
                r.setRouteId(util::upper(readLineField("Route id (e.g. R6)")));
                if (!validate::routeId(r.routeId(), reason)) {
                    note(reason, StatusKind::Error);
                    continue;
                }
                break;
            }
            r.setRouteName(readLineField("Route name"));
            r.setStartLocation(readLineField("Start location"));
            r.setDestination(readLineField("Destination"));
            r.setVia(readLineField("Via"));
            r.setFare(std::atoll(readLineField("Fare").c_str()));
            r.setCapacity(std::atoi(readLineField("Capacity").c_str()));
            if (routes_.addRoute(r, reason)) {
                note("Route " + r.routeId() + " created.", StatusKind::Ok);
            } else {
                note(reason, StatusKind::Error);
            }
            pause();
            continue;
        }

        if (choice == 1 || choice == 2) {
            std::vector<std::string> ids;
            for (const Route& r : routes_.cache()) ids.push_back(r.label());
            if (ids.empty()) {
                note("No routes to modify.", StatusKind::Warning);
                pause();
                continue;
            }
            const int pick = menu("Select a route", ids);
            if (pick < 0) continue;
            const std::string routeId = routes_.cache()[static_cast<std::size_t>(pick)].routeId();

            if (choice == 2) {
                std::string reason;
                if (!confirm("Delete route " + routeId + "?")) continue;
                if (routes_.deleteRoute(routeId, reason)) {
                    note("Route " + routeId + " deleted.", StatusKind::Ok);
                } else {
                    note(reason, StatusKind::Error);
                }
                pause();
                continue;
            }

            Route* r = routes_.findMutable(routeId);
            if (!r) {
                note("Route not found.", StatusKind::Error);
                pause();
                continue;
            }
            std::cout << "\n";
            const std::string fare = readLineField("Fare");
            if (!fare.empty()) r->setFare(std::atoll(fare.c_str()));
            const std::string capacity = readLineField("Capacity");
            if (!capacity.empty()) r->setCapacity(std::atoi(capacity.c_str()));
            const std::string via = readLineField("Via");
            if (!via.empty()) r->setVia(via);

            std::string reason;
            if (routes_.updateRoute(*r, reason)) {
                note("Route " + routeId + " updated.", StatusKind::Ok);
            } else {
                note(reason, StatusKind::Error);
            }
            pause();
            continue;
        }
    }
}

void Screens::staffSeatAllocation(const Session& session) {
    banner("SEAT ALLOCATION", session.displayName);

    std::vector<std::string> ids;
    for (const Route& r : routes_.cache()) {
        const RouteLoad load = routes_.loadFor(r.routeId());
        ids.push_back(r.label() + "   " + paint(Color::Dim, load.occupancyLabel()));
    }
    const int pick = menu("Select a route", ids);
    if (pick < 0) return;
    const std::string routeId = routes_.cache()[static_cast<std::size_t>(pick)].routeId();

    showSeatMap(routeId, "");

    std::vector<std::string> actions = {"Auto-assign the first free seat to an unassigned student",
                                        "Back"};
    const int action = menu("Seat tools", actions);
    if (action != 0) return;

    std::vector<const Student*> unassigned;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (!s) continue;
        if (s->routeId().empty() || s->seatNumber() == 0) unassigned.push_back(s);
    }
    if (unassigned.empty()) {
        note("Every student already holds a seat.", StatusKind::Info);
        pause();
        return;
    }

    std::vector<std::string> names;
    for (const Student* s : unassigned) {
        names.push_back(s->userId() + "  " + s->fullName());
    }
    const int who = menu("Select a student", names);
    if (who < 0) return;

    std::string detail;
    const SeatResult result =
        seats_.autoAssignFirstFree(unassigned[static_cast<std::size_t>(who)]->userId(), routeId, detail);
    std::cout << "\n";
    if (result == SeatResult::Success) {
        note(detail, StatusKind::Ok);
    } else {
        note(detail, StatusKind::Error);
    }
    pause();
}

void Screens::staffPayments(const Session& session) {
    banner("PAYMENTS & DUES", session.displayName);

    std::vector<const Student*> students;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (s) students.push_back(s);
    }
    std::sort(students.begin(), students.end(),
              [](const Student* a, const Student* b) { return a->outstanding() > b->outstanding(); });

    std::vector<std::vector<std::string>> rows;
    for (const Student* s : students) {
        const FeeBreakdown b = s->breakdown();
        rows.push_back({s->userId(), s->fullName(), "Rs " + util::money(b.registrationOutstanding()),
                        "Rs " + util::money(b.transportOutstanding()),
                        "Rs " + util::money(b.totalPaid()), "Rs " + util::money(b.outstanding()),
                        b.statusLabel()});
    }
    table({"ID", "NAME", "REG DUE", "TRN DUE", "PAID", "OUTSTANDING", "STATUS"},
          {9, 22, 12, 12, 12, 14, 16}, rows);

    field("Collected to date", "Rs " + util::money(payments_.collectedTotal()));
    field("Total outstanding", "Rs " + util::money(payments_.outstandingTotal()));
    std::cout << "\n";
    note("Registration and transport balances are tracked separately.", StatusKind::Neutral);
    pause();
}

void Screens::staffComplaints(const Session& session) {
    while (true) {
        banner("COMPLAINTS", session.displayName);

        std::vector<Complaint> all = complaints_.cache();
        std::sort(all.begin(), all.end(), [](const Complaint& a, const Complaint& b) {
            if (a.status() != b.status()) return a.status() == ComplaintStatus::Open;
            return a.createdAt() > b.createdAt();
        });

        if (all.empty()) {
            note("No complaints have been filed.", StatusKind::Info);
            pause();
            return;
        }

        std::vector<std::string> items;
        for (const Complaint& c : all) {
            const std::string tag =
                c.status() == ComplaintStatus::Open ? paint(Color::Yellow, "[OPEN]   ")
                                                    : paint(Color::Green, "[RESOLVED]");
            items.push_back(tag + c.complaintId() + "  " + c.studentId() + "  " + c.subject());
        }
        const int pick = menu("Complaint queue", items);
        if (pick < 0) return;

        const Complaint* c = complaints_.find(all[static_cast<std::size_t>(pick)].complaintId());
        if (!c) {
            note("Complaint not found.", StatusKind::Error);
            pause();
            continue;
        }

        banner(c->complaintId(), c->subject());
        field("Student", c->studentId());
        field("Status", c->statusLabel());
        field("Filed on", c->createdAt());
        section("Description");
        std::cout << "  " << c->description() << "\n";
        if (c->status() == ComplaintStatus::Resolved) {
            section("Existing response");
            std::cout << "  " << c->response() << "\n";
            field("Resolved by", c->resolvedBy());
            field("Resolved at", c->resolvedAt());
        }

        const std::vector<std::string> actions = {
            c->status() == ComplaintStatus::Open ? "Resolve with a response" : "Reopen complaint",
            "Back"};
        const int action = menu("Actions", actions);
        if (action != 0) continue;

        std::string message;
        if (c->status() == ComplaintStatus::Open) {
            const std::string response = readMultiline("Write your response (blank line to finish)");
            if (util::trim(response).empty()) {
                note("A response is required to resolve a complaint.", StatusKind::Error);
                pause();
                continue;
            }
            complaints_.resolve(c->complaintId(), session.username, response, message);
        } else {
            complaints_.reopen(c->complaintId(), session.username, message);
        }
        std::cout << "\n";
        note(message, StatusKind::Ok);
        pause();
    }
}

void Screens::staffNotices(const Session& session, bool canEdit) {
    while (true) {
        banner("NOTICE BOARD", canEdit ? session.displayName : "");

        const std::vector<Notice>& all = notices_.cache();
        if (all.empty()) {
            note("No notices published yet.", StatusKind::Neutral);
        } else {
            std::vector<std::vector<std::string>> rows;
            for (auto it = all.rbegin(); it != all.rend(); ++it) {
                rows.push_back({it->noticeId(), it->title(), it->authorId(), it->createdAt()});
            }
            table({"ID", "TITLE", "AUTHOR", "CREATED"}, {8, 40, 12, 18}, rows);
        }

        std::vector<std::string> items = {"Publish a notice", "Read a notice", "Back"};
        if (canEdit) {
            items.insert(items.end() - 1, "Edit a notice");
            items.insert(items.end() - 1, "Delete a notice");
        }

        const int choice = menu("Notice actions", items);
        const int backIndex = static_cast<int>(items.size()) - 1;
        if (choice < 0 || choice == backIndex) return;

        if (choice == 0) {
            std::cout << "\n";
            const std::string title = readLineField("Title");
            std::string reason;
            if (!validate::noticeTitle(title, reason)) {
                note(reason, StatusKind::Error);
                pause();
                continue;
            }
            const std::string content = readMultiline("Notice body (blank line to finish)");
            std::string createdId;
            const NoticeResult result =
                notices_.publish(title, content, session.username, createdId);
            std::cout << "\n";
            if (result == NoticeResult::Success) {
                note("Notice " + createdId + " published.", StatusKind::Ok);
            } else {
                note(noticeResultMessage(result), StatusKind::Error);
            }
            pause();
            continue;
        }

        if (choice == 1) {
            if (all.empty()) {
                note("There is nothing to read.", StatusKind::Warning);
                pause();
                continue;
            }
            std::vector<std::string> ids;
            for (const Notice& n : all) ids.push_back(n.noticeId() + "  " + n.title());
            const int pick = menu("Select a notice", ids);
            if (pick < 0) continue;
            const Notice* n = notices_.find(all[static_cast<std::size_t>(pick)].noticeId());
            if (!n) {
                note("Notice not found.", StatusKind::Error);
                pause();
                continue;
            }
            banner(n->noticeId(), n->title());
            field("Author", n->authorId());
            field("Created", n->createdAt());
            section("Content");
            std::cout << "  " << n->content() << "\n";
            pause();
            continue;
        }

        if (choice == 2 && canEdit) {
            if (all.empty()) {
                note("There are no notices to edit.", StatusKind::Warning);
                pause();
                continue;
            }
            std::vector<std::string> ids;
            for (const Notice& n : all) ids.push_back(n.noticeId() + "  " + n.title());
            const int pick = menu("Select a notice", ids);
            if (pick < 0) continue;
            const std::string id = all[static_cast<std::size_t>(pick)].noticeId();
            std::cout << "\n";
            const std::string title = readLineField("New title");
            const std::string content = readMultiline("New body (blank line to finish)");
            std::string message;
            const NoticeResult result = notices_.edit(id, title, content, message);
            std::cout << "\n";
            note(message, result == NoticeResult::Success ? StatusKind::Ok : StatusKind::Error);
            pause();
            continue;
        }

        if (choice == 3 && canEdit) {
            if (all.empty()) {
                note("There are no notices to delete.", StatusKind::Warning);
                pause();
                continue;
            }
            std::vector<std::string> ids;
            for (const Notice& n : all) ids.push_back(n.noticeId() + "  " + n.title());
            const int pick = menu("Select a notice", ids);
            if (pick < 0) continue;
            const std::string id = all[static_cast<std::size_t>(pick)].noticeId();
            if (!confirm("Delete notice " + id + "?")) continue;
            std::string message;
            const NoticeResult result = notices_.remove(id, message);
            note(message, result == NoticeResult::Success ? StatusKind::Ok : StatusKind::Error);
            pause();
            continue;
        }
    }
}

void Screens::staffReports(const Session& session) {
    banner("REPORTS", session.displayName);

    std::vector<std::vector<std::string>> rows;
    for (const Route& r : routes_.cache()) {
        const RouteLoad load = routes_.loadFor(r.routeId());
        const long long potential = static_cast<long long>(load.occupiedCount) * r.fare();

        std::vector<std::string> row;
        row.push_back(r.routeId());
        row.push_back(r.endpoints());
        row.push_back(std::to_string(load.occupiedCount));
        row.push_back(std::to_string(load.seatCapacity));
        row.push_back(std::to_string(load.freeSeats()));
        row.push_back("Rs " + util::money(potential));
        rows.push_back(row);
    }
    table({"ROUTE", "PATH", "STUDENTS", "CAPACITY", "FREE", "BILLED VALUE"}, {8, 28, 10, 10, 8, 16},
          rows);

    int students = 0;
    for (const auto& user : auth_.userCache()) {
        if (dynamic_cast<const Student*>(user.get()) != nullptr) ++students;
    }

    field("Registered students", std::to_string(students));
    field("Active routes", std::to_string(routes_.cache().size()));
    field("Open complaints", std::to_string(complaints_.openCount()));
    field("Total collected", "Rs " + util::money(payments_.collectedTotal()));
    field("Total outstanding", "Rs " + util::money(payments_.outstandingTotal()));
    field("Notices published", std::to_string(notices_.cache().size()));
    pause();
}

}  // namespace ui
}  // namespace st