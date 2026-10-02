// Regenerates the screenshots used in README.md and docs/.
//
// This is a development tool, not part of the application. It builds only when
// ST_BUILD_GUI_CAPTURE is enabled, because it links every page for the sole
// purpose of driving them offscreen.
//
//   cmake -S . -B build -DST_BUILD_GUI_CAPTURE=ON
//   cmake --build build --target gui_capture
//   ./build/bin/gui_capture <output-directory> [--dark]
//
// It runs against a throwaway data directory and seeds its own demo records, so
// it never touches data/ in the source tree.

#include "AppContext.hpp"
#include "LoginWindow.hpp"
#include "MainWindow.hpp"
#include "Pickers.hpp"
#include "PublicWindow.hpp"
#include "RegistrationWizard.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "controllers/Controllers.hpp"

#include <QApplication>
#include <QDir>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringList>

#include <iostream>

namespace {

QString g_outputDir;

bool writeShot(QWidget* widget, const QString& name) {
    if (!widget) return false;
    QApplication::processEvents();

    const QString path = QDir(g_outputDir).filePath(name + QStringLiteral(".png"));
    if (!widget->grab().save(path)) {
        std::cerr << "FAILED " << path.toStdString() << std::endl;
        return false;
    }
    std::cout << "  " << path.toStdString() << std::endl;
    return true;
}

// Output names are fixed rather than derived from the nav label, so that
// regenerating the screenshots overwrites the files README.md already links to
// instead of creating a parallel set with different names.
struct NavName {
    const char* label;
    const char* name;
    bool student;
};

const NavName kNames[] = {
    // Light, staff and administration.
    {"Dashboard", "10-admin-dashboard", false},
    {"Students", "11-admin-students", false},
    {"Routes", "12-admin-routes", false},
    {"Seat map", "13-admin-seat-map", false},
    {"Payments & dues", "14-admin-payments", false},
    {"Complaints", "15-admin-complaints", false},
    {"Notices", "16-admin-notices", false},
    {"Reports", "17-admin-reports", false},
    {"Staff accounts", "18-admin-staff-accounts", false},
    {"My profile", "19-admin-profile", false},
    {"Help & contact", "20-admin-help", false},
    {"Settings", "21-admin-settings", false},

    // Light, student.
    {"Dashboard", "30-student-dashboard", true},
    {"My transport", "31-student-seat-map", true},
    {"Payments & dues", "32-student-payments", true},
    {"My complaints", "33-student-complaints", true},
    {"Notice board", "34-student-notices", true},
    {"Allotment card", "35-student-allotment-card", true},
    {"My profile", "36-student-profile", true},

    // Dark: a deliberate sample, not the full set.
    {"Dashboard", "40-dark-dashboard", false},
    {"Seat map", "41-dark-seat-map", false},
    {"Settings", "43-dark-settings", false},
    {"Payments & dues", "42-dark-payments", true},
};

QString outputName(const QString& prefix, const QString& label) {
    // The dark student prefix is "dark-student", so this cannot be startsWith.
    const bool student = prefix.contains(QStringLiteral("student"));
    const bool dark = prefix.startsWith(QStringLiteral("dark"));

    // Dark and light share labels, so each pass only considers its own entries.
    const int count = static_cast<int>(sizeof(kNames) / sizeof(kNames[0]));
    for (int index = 0; index < count; ++index) {
        const QString name = QString::fromLatin1(kNames[index].name);
        const bool isDarkEntry = name.startsWith(QStringLiteral("4"));
        if (isDarkEntry != dark) continue;
        if (label != QLatin1String(kNames[index].label)) continue;
        if (kNames[index].student != student) continue;
        return name;
    }
    return {};
}

// Creates a small, realistic dataset so the screenshots are not all empty
// tables. Records are only added when they are missing, so re-running is safe.
void seedDemoData(gui::Controllers& controllers) {
    gui::AppContext& context = controllers.context();

    const struct {
        const char* username;
        const char* fullName;
        const char* fatherName;
        const char* phone;
        const char* address;
    } people[] = {
        {"ananya", "Ananya Sharma", "Ravi Sharma", "9876543210", "Hostel Block C, Room 214"},
        {"rahul", "Rahul Verma", "Suresh Verma", "9812345678", "12-4-9 Nehru Nagar"},
        {"meera", "Meera Iyer", "Lakshmi Iyer", "9701122334", "MG Road, Ameerpet"},
        {"arjun", "Arjun Nair", "Suresh Nair", "9655443322", "Banjara Hills, Road 12"},
    };

    for (const auto& person : people) {
        if (context.auth().usernameExists(person.username)) continue;
        st::RegistrationForm form;
        form.fullName = person.fullName;
        form.fatherName = person.fatherName;
        form.phone = person.phone;
        form.address = person.address;
        form.username = person.username;
        form.password = "demo1234";
        controllers.auth().registerStudent(form);
    }

    const QStringList ids = controllers.students().idList();
    const QVector<gui::RouteOption> routes = controllers.routes().options(false);
    if (ids.isEmpty() || routes.isEmpty()) return;

    const int seats[] = {2, 5, 7};
    for (int index = 0; index < ids.size() && index < 3; ++index) {
        controllers.seats().assign(ids.at(index), routes.first().id, seats[index]);
    }
    if (ids.size() > 3 && routes.size() > 1) {
        controllers.seats().assign(ids.at(3), routes.at(1).id, 3);
    }

    gui::ReceiptView receipt;
    if (controllers.payments().history(ids.first()).isEmpty()) {
        controllers.payments().record(ids.at(0), st::FeeKind::Registration, 2500,
                                      gui::qs("ADM0001"), receipt);
    }
    if (ids.size() > 1 && controllers.payments().history(ids.at(1)).isEmpty()) {
        controllers.payments().record(ids.at(1), st::FeeKind::Transport, 1200,
                                      gui::qs("ADM0001"), receipt);
    }

    QString complaintId;
    if (controllers.complaints().forStudent(ids.first()).isEmpty()) {
        controllers.complaints().submit(
            ids.at(0), QStringLiteral("Bus arrives 20 minutes late"),
            QStringLiteral("The 7:30 pickup on Route 4 has been consistently late for the past "
                           "two weeks. Students are being marked absent at the gate."),
            complaintId);
    }
    if (ids.size() > 2 && controllers.complaints().openCount() < 2) {
        controllers.complaints().submit(
            ids.at(2), QStringLiteral("Broken window on the rear row"),
            QStringLiteral("The window on the rear row does not close, so it gets very wet "
                           "during the monsoon."),
            complaintId);
    }

    if (controllers.notices().rows().isEmpty()) {
        controllers.notices().publish(
            QStringLiteral("Route 4 timing change"),
            QStringLiteral("From Monday the Route 4 pickup moves 15 minutes earlier to 07:15. "
                           "Please reach your stop five minutes before departure."),
            gui::qs("ADM0001"));
        controllers.notices().publish(
            QStringLiteral("Fee deadline extended"),
            QStringLiteral("The last date to clear outstanding transport fees without a late "
                           "charge has moved to the 28th."),
            gui::qs("ADM0001"));
    }
}

void captureWindowPages(gui::Controllers& controllers, const QString& role,
                        const QString& prefix) {
    gui::MainWindow window(controllers, controllers.auth().session());
    auto* stack = window.findChild<QStackedWidget*>();

    const QList<QPushButton*> buttons = window.findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->objectName() != QStringLiteral("NavButton")) continue;
        button->click();
        QApplication::processEvents();

        const QString name = outputName(prefix, button->text());
        if (name.isEmpty()) continue;
        if (!writeShot(&window, name)) continue;

        // The screenshot should show the page the nav entry claims to open.
        if (stack && stack->currentWidget()) {
            std::cout << "     " << role.toStdString() << " " << button->text().toStdString() << " -> "
                      << stack->currentWidget()->metaObject()->className() << std::endl;
        }
    }
}

void captureDialogs(gui::Controllers& controllers, bool dark) {
    const QString prefix = dark ? QStringLiteral("dark-") : QString();
    const auto name = [&prefix](const char* base) { return prefix + QString::fromLatin1(base); };

    {
        gui::LoginWindow login(controllers);
        writeShot(&login, name("01-login"));
    }

    // The dark set is a sample: the login plus the four pages above.
    if (dark) return;

    {
        gui::RegistrationWizard wizard(controllers, false);
        writeShot(&wizard, name("02-registration-wizard"));
    }
    {
        gui::RoutePickerDialog picker(controllers.routes());
        writeShot(&picker, name("03-route-picker"));
    }
    const QVector<gui::RouteOption> routes = controllers.routes().options(true);
    if (!routes.isEmpty()) {
        gui::SeatPickerDialog seats(controllers.routes(), controllers.seats(), routes.first().id,
                                    QString());
        writeShot(&seats, name("04-seat-picker"));
    }
    {
        gui::PublicWindow routesWindow(controllers, gui::PublicWindow::Tab::Routes);
        writeShot(&routesWindow, name("05-public-routes"));
    }
    {
        gui::PublicWindow aboutWindow(controllers, gui::PublicWindow::Tab::About);
        writeShot(&aboutWindow, name("06-public-about"));
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("TransportCell");
    QApplication::setApplicationName("Student Transport");

    const QStringList args = app.arguments();
    if (args.size() < 2) {
        std::cerr << "usage: gui_capture <output-directory> [--dark]" << std::endl;
        return 2;
    }

    g_outputDir = args.at(1);
    const bool wantDark = args.contains(QStringLiteral("--dark"));
    QDir().mkpath(g_outputDir);

    gui::applyTheme(app);
    // Set the theme explicitly rather than inheriting whatever was persisted on
    // this machine, so a light run can never come out dark.
    gui::setTheme(wantDark ? gui::ThemeKind::Dark : gui::ThemeKind::Light);

    gui::AppContext context;
    gui::Controllers controllers(context);
    seedDemoData(controllers);

    std::cout << "capturing " << gui::toStd(g_outputDir)
              << (wantDark ? " (dark)" : " (light)") << std::endl;

    if (!controllers.auth().login(QStringLiteral("admin"), QStringLiteral("admin123")).ok) {
        std::cerr << "could not sign in as admin" << std::endl;
        return 1;
    }
    captureWindowPages(controllers, QStringLiteral("admin"),
                       wantDark ? QStringLiteral("dark-admin")
                                : QStringLiteral("admin"));
    captureDialogs(controllers, wantDark);

    if (controllers.auth().login(QStringLiteral("ananya"), QStringLiteral("demo1234")).ok) {
        captureWindowPages(controllers, QStringLiteral("student"),
                           wantDark ? QStringLiteral("dark-student")
                                    : QStringLiteral("student"));
    } else {
        std::cerr << "could not sign in as the demo student" << std::endl;
    }

    std::cout << "done" << std::endl;
    return 0;
}