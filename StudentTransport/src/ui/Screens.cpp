#include "ui/Screens.hpp"

#include "services/ComplaintService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "services/UserService.hpp"
#include "ui/ConsoleUI.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>
#include <iostream>

namespace st {
namespace ui {

void Screens::run() {
    while (true) {
        const int choice = mainMenu();
        if (choice < 0) return;
    }
}

int Screens::mainMenu() {
    banner(config_.config().institutionName + " TRANSPORT MANAGEMENT",
           config_.config().tagline());

    note("Register, sign in, browse routes and read notices.", StatusKind::Neutral);
    std::cout << "\n";

    const std::vector<std::string> items = {
        "Login",
        "Register",
        "View Routes",
        "Notice Board",
        "Help / Contact",
        "Exit",
    };

    MenuOptions options;
    options.footerText = "StudentTransport " + config_.config().institutionName +
                         "  \u00b7  itsourcecode.com";
    const int choice = menu("Main Menu", items, options);
    if (choice == kMenuCancelled) return 0;

    // Esc at the top level is a no-op rather than a quit: an accidental keypress
    // should never discard an in-progress session. Ctrl-D (closed input) exits.
    if (choice == kMenuCancelled) return 0;

    switch (choice) {
        case 0: {
            const int which = menu("Sign in as", {"Student", "Staff", "Administrator"});
            if (which < 0) return choice;
            loginFlow(which == 0 ? Role::Student : (which == 1 ? Role::Staff : Role::Admin));
            return choice;
        }
        case 1: registrationFlow(); return choice;
        case 2: viewRoutes(); return choice;
        case 3: noticeBoard(Session{}, false); return choice;
        case 4: helpContact(); return choice;
        case kMenuInputClosed:
        case 5: {
            banner("Goodbye", "Thank you for using the transport portal.");
            note("Session closed.", StatusKind::Ok);
            pause();
            return kMenuCancelled;
        }
        default: return choice;
    }
}

void Screens::registrationFlow() {
    const int which = menu("Register as", {"Student", "Staff (authorization required)"});
    if (which < 0) return;

    RegistrationForm form;

    banner("REGISTRATION", which == 0 ? "Student account" : "Staff account");
    note("Fields marked * are required.", StatusKind::Neutral);
    std::cout << "\n";

    while (true) {
        form.fullName = readLineField("Full name *");
        if (!validate::fullName(form.fullName, form.fullName)) {
            note("Full name is required.", StatusKind::Error);
            continue;
        }
        break;
    }

    form.fatherName = readLineField("Father / Guardian");
    if (form.fatherName.empty()) form.fatherName = "-";

    while (true) {
        form.phone = readLineField("Mobile number *");
        std::string reason;
        if (!validate::phone(form.phone, reason)) {
            note(reason, StatusKind::Error);
            continue;
        }
        break;
    }

    while (true) {
        form.address = readLineField("Address *");
        std::string reason;
        if (!validate::address(form.address, reason)) {
            note(reason, StatusKind::Error);
            continue;
        }
        break;
    }

    const std::string suggestion =
        which == 0 ? auth_.nextStudentUsername() : auth_.nextStaffUsername();
    while (true) {
        const std::string typed = readLineField("Username [" + suggestion + "]");
        form.username = typed.empty() ? suggestion : typed;
        std::string reason;
        if (!validate::username(form.username, reason)) {
            note(reason, StatusKind::Error);
            continue;
        }
        if (auth_.usernameExists(form.username)) {
            note("That username is already taken.", StatusKind::Error);
            continue;
        }
        break;
    }

    std::string strength;
    while (true) {
        form.password = readSecretField("Password *");
        if (!Security::checkPolicy(form.password, strength)) {
            note(strength, StatusKind::Error);
            continue;
        }
        const std::string again = readSecretField("Confirm password *");
        if (again != form.password) {
            note("Passwords do not match.", StatusKind::Error);
            continue;
        }
        std::cout << "  " << statusText(StatusKind::Info, "Password strength: " +
                                                    Security::strengthLabel(
                                                        Security::rateStrength(form.password)) +
                                                    ". Store it safely.") << "\n";
        break;
    }

    std::string authCode;
    if (which == 1) {
        std::cout << "\n";
        note("Staff accounts require the authorization code issued by the transport office.",
             StatusKind::Warning);
        authCode = readSecretField("Authorization code");
    }

    if (!confirm("Create this account?")) {
        note("Registration cancelled.", StatusKind::Neutral);
        return;
    }

    AuthService::RegistrationResult result;
    if (which == 0) {
        note("The transport fee is set once you choose a route and seat.", StatusKind::Neutral);
        result = auth_.registerStudent(form);
    } else {
        result = auth_.registerStaff(form, authCode);
    }

    std::cout << "\n";
    if (result.ok) {
        note(result.message, StatusKind::Ok);
        field("Username", result.username);
        field("Temporary password", "the one you just chose");
        note("Please change your password after signing in.", StatusKind::Warning);
    } else {
        note(result.message, StatusKind::Error);
    }
    pause();
}

void Screens::loginFlow(Role expectedRole) {
    banner("LOGIN", std::string("Sign in as ") + roleName(expectedRole));

    std::string username;
    std::string secret;
    while (true) {
        username = readLineField("Username");
        if (!username.empty()) break;
        note("Username is required.", StatusKind::Error);
    }
    while (true) {
        secret = readSecretField("Password");
        if (!secret.empty()) break;
        note("Password is required.", StatusKind::Error);
    }

    Session session;
    const AuthOutcome outcome = auth_.login(username, secret, session);
    if (outcome != AuthOutcome::Success) {
        std::cout << "\n";
        note(authOutcomeMessage(outcome), StatusKind::Error);
        if (outcome == AuthOutcome::WrongPassword) {
            note("If you keep failing, reset your password from a staff member.", StatusKind::Neutral);
        }
        pause();
        return;
    }

    if (session.role != expectedRole) {
        std::cout << "\n";
        note("That account is registered as " + std::string(roleName(session.role)) +
                 ", not " + roleName(expectedRole) + ".",
             StatusKind::Error);
        pause();
        return;
    }

    switch (session.role) {
        case Role::Admin: adminDashboard(session); break;
        case Role::Staff: staffDashboard(session); break;
        case Role::Student:
        default: studentDashboard(session); break;
    }
}

}  // namespace ui
}  // namespace st