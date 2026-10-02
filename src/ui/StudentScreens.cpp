#include "ui/Screens.hpp"

#include "services/ComplaintService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "ui/ConsoleUI.hpp"
#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>
#include <cctype>
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

void Screens::studentDashboard(const Session& session) {
    while (true) {
        Student* student = auth_.findStudent(session.userId);
        banner("STUDENT DASHBOARD", session.displayName);
        if (!student) {
            note("Your student record could not be loaded.", StatusKind::Error);
            pause();
            return;
        }

        const Route* route = routes_.find(student->routeId());

        // The menu owns and repaints the screen, so the summary is handed to it
        // as context rather than printed here where a redraw would wipe it.
        MenuOptions options;
        options.context.push_back(row("Student ID", student->userId()));
        options.context.push_back(row("Route",
                                      route ? route->label() : std::string("not selected yet")));
        options.context.push_back(row("Seat", student->seatNumber() > 0
                                                 ? std::to_string(student->seatNumber())
                                                 : "not assigned yet"));
        options.context.push_back(row("Payment", student->fullyPaid()
                                                  ? "[PAID]"
                                                  : (student->totalPaid() == 0
                                                         ? "[PENDING]"
                                                         : "[PARTIALLY PAID]")));
        options.context.push_back(row("Outstanding", "Rs " + util::money(student->outstanding())));

        const std::vector<std::string> items = {
            "My Transport",
            "Route & Seat",
            "Payments & Dues",
            "Allotment Card",
            "Complaints",
            "Notice Board",
            "Profile",
            "Change Password",
            "Help / Contact",
            "Logout",
        };

        const int choice = menu("Student Menu", items, options);
        switch (choice) {
            case 0: studentTransport(session); break;
            case 1: studentRouteSeat(session); break;
            case 2: studentPayments(session); break;
            case 3: studentAllotmentCard(session); break;
            case 4: studentComplaints(session); break;
            case 5: noticeBoard(session, false); break;
            case 6: studentProfile(session); break;
            case 7: changePassword(session); break;
            case 8: helpContact(); break;
            case 9:
                note("Logged out. Goodbye, " + session.displayName + ".", StatusKind::Ok);
                pause();
                return;
            default: return;
        }
    }
}

void Screens::studentTransport(const Session& session) {
    Student* student = nullptr;
    if (!requireStudent(session, student)) return;

    banner("MY TRANSPORT", student->displayName());
    if (!student->seatAssigned()) {
        note("You have no route or seat yet.", StatusKind::Warning);
        note("Use 'Route & Seat' to choose one.", StatusKind::Neutral);
        pause();
        return;
    }

    const Route* route = routes_.find(student->routeId());
    if (!route) {
        note("Your assigned route no longer exists. Contact the transport office.", StatusKind::Error);
        pause();
        return;
    }

    field("Route", route->label());
    field("Endpoints", route->endpoints());
    field("Via", route->via());
    field("Seat number", std::to_string(student->seatNumber()));
    field("Annual fare", "Rs " + util::money(route->fare()));
    std::cout << "\n";
    showSeatMap(student->routeId(), student->userId());
    pause();
}

void Screens::studentRouteSeat(const Session& session) {
    Student* student = nullptr;
    if (!requireStudent(session, student)) return;

    std::string chosenRoute = student->routeId();
    int chosenSeat = student->seatNumber();

    for (;;) {
        banner("ROUTE & SEAT", student->displayName());
        if (student->seatAssigned()) {
            const Route* current = routes_.find(student->routeId());
            field("Current route", current ? current->label() : student->routeId());
            field("Current seat", std::to_string(student->seatNumber()));
            std::cout << "\n";
        } else {
            note("No route selected yet.", StatusKind::Neutral);
            std::cout << "\n";
        }

        std::vector<std::string> items;
        std::vector<std::string> routeIds;
        for (const Route& r : routes_.cache()) {
            const RouteLoad load = routes_.loadFor(r.routeId());
            items.push_back(r.label() + "   " + paint(Color::Dim, "Rs " + util::money(r.fare()) +
                                                       "  " + load.occupancyLabel()));
            routeIds.push_back(r.routeId());
        }
        if (items.empty()) {
            note("No routes are configured.", StatusKind::Error);
            pause();
            return;
        }

        const int routeIndex = menu("Choose a route", items);
        if (routeIndex < 0) return;
        chosenRoute = routeIds[static_cast<std::size_t>(routeIndex)];

        const Route* route = routes_.find(chosenRoute);
        const std::vector<int> free = routes_.availableSeats(chosenRoute);
        if (free.empty()) {
            note("Route " + chosenRoute + " is full.", StatusKind::Error);
            pause();
            continue;
        }

        banner("SELECT A SEAT", route->label());
        showSeatMap(chosenRoute, student->userId());

        std::vector<std::string> seatItems;
        for (int seat : free) {
            seatItems.push_back("Seat " + std::to_string(seat));
        }
        const int seatIndex = menu("Available seats", seatItems);
        if (seatIndex < 0) continue;
        chosenSeat = free[static_cast<std::size_t>(seatIndex)];

        std::string detail;
        const SeatResult result =
            seats_.assign(student->userId(), chosenRoute, chosenSeat, detail);
        std::cout << "\n";
        if (result == SeatResult::Success) {
            note(detail, StatusKind::Ok);
            if (Student* fresh = auth_.findStudent(session.userId)) {
                field("Annual fare", "Rs " + util::money(routes_.find(chosenRoute)->fare()));
                field("Total payable", "Rs " + util::money(fresh->totalAmount()));
            }
            pause();
            return;
        }
        note(detail, StatusKind::Error);
        pause();
    }
}

void Screens::studentPayments(const Session& session) {
    while (true) {
        Student* student = nullptr;
        if (!requireStudent(session, student)) return;

        banner("PAYMENTS & DUES", student->displayName());
        const FeeBreakdown b = student->breakdown();
        field("Total amount", "Rs " + util::money(b.totalAmount()));
        field("Total paid", "Rs " + util::money(b.totalPaid()));
        field("Outstanding", "Rs " + util::money(b.outstanding()));
        field("Status", b.statusLabel());

        const std::vector<Payment> history = payments_.historyFor(student->userId());
        if (!history.empty()) {
            section("Payment History");
            std::vector<std::vector<std::string>> rows;
            for (const Payment& p : history) {
                rows.push_back({p.paymentId(), p.paymentDate(), p.kindLabel(),
                                "Rs " + util::money(p.amount()), p.reference(), p.recordedBy()});
            }
            table({"REF", "DATE", "TYPE", "AMOUNT", "REFERENCE", "BY"}, {10, 17, 13, 12, 15, 10},
                  rows);
        }

        if (b.isSettled()) {
            note("Nothing outstanding. Your account is fully paid.", StatusKind::Ok);
            pause();
            return;
        }

        std::vector<std::string> items;
        const long long regDue = b.registrationOutstanding();
        const long long trnDue = b.transportOutstanding();
        if (regDue > 0) items.push_back("Pay registration fee (Rs " + util::money(regDue) + ")");
        if (trnDue > 0) items.push_back("Pay transport fee (Rs " + util::money(trnDue) + ")");
        items.push_back("Back");

        const int choice = menu("Make a payment", items);
        if (choice < 0 || choice == static_cast<int>(items.size()) - 1) return;

        FeeKind bucket = FeeKind::Registration;
        if (regDue <= 0) {
            bucket = FeeKind::Transport;
        } else if (trnDue <= 0) {
            bucket = FeeKind::Registration;
        } else {
            bucket = choice == 0 ? FeeKind::Registration : FeeKind::Transport;
        }

        const long long bucketDue =
            bucket == FeeKind::Registration ? regDue : trnDue;
        const int amount = askAmountIn("Amount to pay", bucketDue);

        const PaymentReceipt receipt =
            payments_.recordPayment(student->userId(), bucket, amount, session.username);
        std::cout << "\n";
        if (!receipt.ok) {
            note(receipt.message, StatusKind::Error);
            pause();
            continue;
        }
        note(receipt.message.empty()
                 ? std::string("Payment recorded.")
                 : receipt.message,
             StatusKind::Ok);

        section("Receipt");
        field("Payment ID", receipt.paymentId);
        field("Reference", receipt.reference);
        field("Bucket", feeKindName(receipt.bucket));
        field("Amount applied", "Rs " + util::money(receipt.appliedAmount));
        field("Outstanding after", "Rs " + util::money(receipt.outstandingAfter));
        pause();
    }
}

void Screens::studentAllotmentCard(const Session& session) {
    Student* student = nullptr;
    if (!requireStudent(session, student)) return;

    banner("ALLOTMENT CARD", student->displayName());
    const Route* route = routes_.find(student->routeId());
    if (!route) {
        note("No route has been allotted yet.", StatusKind::Warning);
        pause();
        return;
    }

    pageHeader("Student Transport Allotment");
    std::cout << "\n";
    field("Student", student->fullName());
    field("Student ID", student->userId());
    field("Guardian", student->fatherName());
    field("Mobile", student->phone());
    field("Route", route->label());
    field("Seat number", std::to_string(student->seatNumber()));
    field("Annual fare", "Rs " + util::money(route->fare()));
    std::cout << "  " << util::repeat('-', contentWidth()) << "\n";
    field("Total amount", "Rs " + util::money(student->totalAmount()));
    field("Outstanding", "Rs " + util::money(student->outstanding()));
    field("Payment status", student->fullyPaid() ? std::string("PAID") : student->breakdown().statusLabel());
    std::cout << "\n  " << util::pad("Signature", 24)
              << "__________________________" << "\n";
    note("Show this card to the driver at boarding.", StatusKind::Neutral);
    pause();
}

void Screens::studentComplaints(const Session& session) {
    Student* student = nullptr;
    if (!requireStudent(session, student)) return;

    while (true) {
        banner("MY COMPLAINTS", student->displayName());
        const std::vector<Complaint> mine = complaints_.forStudent(session.userId);

        if (mine.empty()) {
            note("You have not raised any complaints.", StatusKind::Neutral);
        } else {
            for (const Complaint& c : mine) {
                std::cout << "\n  " << paint(Color::Bold, c.complaintId() + "  " + c.subject())
                          << "\n";
                std::cout << "    " << paint(Color::Dim, c.createdAt()) << "   "
                          << (c.status() == ComplaintStatus::Open
                                  ? paint(Color::Yellow, "[OPEN]")
                                  : paint(Color::Green, "[RESOLVED]"))
                          << "\n";
                if (c.status() == ComplaintStatus::Resolved) {
                    std::cout << "    " << paint(Color::Dim, "Response: " + c.response())
                              << "  (" + c.resolvedBy() + ", " + c.resolvedAt() + ")\n";
                }
            }
        }

        const std::vector<std::string> items = {"Submit a complaint", "View full details", "Back"};
        const int choice = menu("Complaints", items);
        if (choice < 0 || choice == 2) return;

        if (choice == 0) {
            std::cout << "\n";
            const std::string subject = readLineField("Subject");
            std::string reason;
            if (!validate::complaintSubject(subject, reason)) {
                note(reason, StatusKind::Error);
                pause();
                continue;
            }
            const std::string description = readMultiline("Describe the issue (blank line to finish)");
            if (util::trim(description).empty()) {
                note("Complaint description cannot be empty.", StatusKind::Error);
                pause();
                continue;
            }
            std::string createdId;
            const ComplaintResult result =
                complaints_.submit(session.userId, subject, description, createdId);
            std::cout << "\n";
            if (result == ComplaintResult::Success) {
                note("Complaint " + createdId + " filed.", StatusKind::Ok);
            } else {
                note(complaintResultMessage(result), StatusKind::Error);
            }
            pause();
            continue;
        }

        if (mine.empty()) {
            note("Nothing to show.", StatusKind::Warning);
            pause();
            continue;
        }
        std::vector<std::string> items2;
        for (const Complaint& c : mine) {
            items2.push_back(c.complaintId() + "  " + c.statusLabel() + "  " + c.subject());
        }
        const int pick = menu("Select a complaint", items2);
        if (pick < 0) continue;
        const Complaint* c = complaints_.find(mine[static_cast<std::size_t>(pick)].complaintId());
        if (!c) {
            note("Complaint not found.", StatusKind::Error);
            pause();
            continue;
        }
        banner(c->complaintId(), c->subject());
        field("Status", c->statusLabel());
        field("Filed on", c->createdAt());
        section("Description");
        std::cout << "  " << c->description() << "\n";
        if (c->status() == ComplaintStatus::Resolved) {
            section("Staff Response");
            std::cout << "  " << c->response() << "\n";
            field("Resolved by", c->resolvedBy());
            field("Resolved at", c->resolvedAt());
        }
        pause();
    }
}

void Screens::studentProfile(const Session& session) {
    Student* student = nullptr;
    if (!requireStudent(session, student)) return;

    banner("MY PROFILE", student->displayName());
    field("Student ID", student->userId());
    field("Username", student->username());
    field("Full name", student->fullName());
    field("Guardian", student->fatherName());
    field("Mobile", student->phone());
    field("Address", student->address());
    field("Registered on", student->createdAt());
    field("Account status", student->active() ? "ACTIVE" : "INACTIVE");
    pause();
}

}  // namespace ui
}  // namespace st