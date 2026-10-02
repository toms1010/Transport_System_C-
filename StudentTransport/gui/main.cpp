#include "AppContext.hpp"
#include "LoginWindow.hpp"
#include "MainWindow.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "controllers/Controllers.hpp"

#include <QApplication>
#include <QDialog>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Student Transport");
    QApplication::setApplicationDisplayName("Student Transport Management System");
    QApplication::setOrganizationName("TransportCell");
    QApplication::setApplicationVersion("2.1.0");

    gui::applyTheme(app);

    gui::AppContext context;
    gui::Controllers controllers(context);

    // Login and window alternate until the user closes the window for good.
    while (true) {
        gui::LoginWindow login(controllers);
        if (login.exec() != QDialog::Accepted) return 0;

        gui::MainWindow window(controllers, login.session());
        QObject::connect(&window, &gui::MainWindow::logoutRequested, &window, &QWidget::close);
        window.show();
        app.exec();

        if (!window.isVisible()) break;
    }

    return 0;
}