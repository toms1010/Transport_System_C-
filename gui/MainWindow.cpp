#include "MainWindow.hpp"

#include "AllotmentPage.hpp"
#include "AppContext.hpp"
#include "ComplaintsPage.hpp"
#include "DashboardPage.hpp"
#include "HelpPage.hpp"
#include "NoticesPage.hpp"
#include "PaymentsPage.hpp"
#include "ProfilePage.hpp"
#include "ReportsPage.hpp"
#include "RosterPage.hpp"
#include "RoutesPage.hpp"
#include "SeatMapPage.hpp"
#include "SettingsPage.hpp"
#include "StaffAccountsPage.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace gui {
namespace {

struct NavItem {
    const char* label;
    const char* icon;
};

const NavItem kStudentNav[] = {
    {"Dashboard", "dashboard"},   {"My transport", "transport"}, {"Payments & dues", "payment"},
    {"My complaints", "complaint"}, {"Notice board", "notice"},  {"Allotment card", "card"},
    {"My profile", "profile"},     {"Help & contact", "help"},   {"Settings", "settings"},
};

const NavItem kStaffNav[] = {
    {"Dashboard", "dashboard"},   {"Students", "users"},     {"Routes", "transport"},
    {"Seat map", "seat"},         {"Payments & dues", "payment"}, {"Complaints", "complaint"},
    {"Notices", "notice"},        {"Reports", "report"},     {"Staff", "staff"},
    {"My profile", "profile"},    {"Help & contact", "help"}, {"Settings", "settings"},
};

}  // namespace

MainWindow::MainWindow(Controllers& controllers, const st::Session& session, QWidget* parent)
    : QMainWindow(parent), controllers_(controllers), session_(session) {
    setWindowTitle(
        QStringLiteral("%1 Transport · %2").arg(controllers.context().institutionName(),
                                                 qs(session.displayName)));
    resize(1340, 860);
    setMinimumSize(1120, 680);

    buildSidebar();
    buildTopBar();

    stack_ = new QStackedWidget;

    auto* footer = new QFrame;
    footer->setFixedHeight(30);
    footer->setObjectName("Footer");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 0);
    auto* dataLabel = new QLabel(QStringLiteral("Data folder: %1").arg(controllers.context().dataDirectory()));
    dataLabel->setObjectName("Muted");
    footerLayout->addWidget(dataLabel);
    footerLayout->addStretch();

    auto* rightColumn = new QVBoxLayout;
    rightColumn->setContentsMargins(0, 0, 0, 0);
    rightColumn->setSpacing(0);
    rightColumn->addWidget(topBar_);
    rightColumn->addWidget(stack_, 1);
    rightColumn->addWidget(footer);

    auto* central = new QWidget;
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(sidebar_);
    root->addLayout(rightColumn, 1);
    setCentralWidget(central);

    registerPages();

    connect(navGroup_, &QButtonGroup::buttonClicked, this, &MainWindow::onNavClicked);
    if (!navButtons_.isEmpty()) navButtons_.first()->setChecked(true);

    // setChecked() does not emit buttonClicked, so the opening page is refreshed here.
    refreshCurrentPage();
}

QStringList MainWindow::navLabels() const {
    QStringList labels;
    const bool student = session_.role == st::Role::Student;

    if (student) {
        for (const NavItem& item : kStudentNav) labels << QString::fromLatin1(item.label);
        return labels;
    }

    for (const NavItem& item : kStaffNav) labels << QString::fromLatin1(item.label);
    if (session_.role == st::Role::Admin) {
        // Staff accounts only exist for administrators.
        labels.removeAll(QStringLiteral("Staff"));
        labels.insert(8, QStringLiteral("Staff accounts"));
    }
    return labels;
}

void MainWindow::buildSidebar() {
    sidebar_ = new QFrame;
    sidebar_->setObjectName("Sidebar");
    sidebar_->setFixedWidth(244);

    auto* sidebarLayout = new QVBoxLayout(sidebar_);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto* brand = new QWidget(sidebar_);
    auto* brandLayout = new QVBoxLayout(brand);
    brandLayout->setContentsMargins(22, 26, 22, 20);
    brandLayout->setSpacing(2);

    auto* title = new QLabel(controllers_.context().institutionName());
    title->setObjectName("BrandTitle");
    auto* subtitle = new QLabel(QStringLiteral("TRANSPORT CELL"));
    subtitle->setObjectName("BrandSubtitle");
    brandLayout->addWidget(title);
    brandLayout->addWidget(subtitle);
    sidebarLayout->addWidget(brand);

    navGroup_ = new QButtonGroup(this);
    navGroup_->setExclusive(true);
    navLabels_ = navLabels();

    auto* navHost = new QWidget(sidebar_);
    auto* navLayout = new QVBoxLayout(navHost);
    navLayout->setContentsMargins(10, 0, 10, 0);
    navLayout->setSpacing(2);

    const bool student = session_.role == st::Role::Student;
    const NavItem* items = student ? kStudentNav : kStaffNav;
    const int itemCount = static_cast<int>(sizeof(kStudentNav) / sizeof(kStudentNav[0]));

    for (const QString& label : navLabels_) {
        const char* iconName = "dashboard";
        for (int index = 0; index < itemCount; ++index) {
            if (label != QLatin1String(items[index].label)) continue;
            iconName = items[index].icon;
            break;
        }

        auto* button = new QPushButton;
        button->setObjectName("NavButton");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(40);
        button->setIcon(icon(QString::fromLatin1(iconName)));
        button->setIconSize(QSize(17, 17));
        button->setText(label);
        button->setToolTip(label);
        navGroup_->addButton(button);
        navLayout->addWidget(button);
        navButtons_.push_back(button);
    }

    navLayout->addStretch();
    sidebarLayout->addWidget(navHost, 1);

    const QString roleName =
        session_.role == st::Role::Admin
            ? QStringLiteral("Administrator")
            : (session_.role == st::Role::Staff ? QStringLiteral("Transport staff")
                                                : QStringLiteral("Student"));

    auto* signOut = ghostButton(QStringLiteral("Sign out"), QStringLiteral("logout"));

    auto* footer = new QWidget(sidebar_);
    auto* footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(22, 12, 18, 20);
    footerLayout->setSpacing(3);

    auto* roleLabel = new QLabel(roleName.toUpper());
    roleLabel->setObjectName("SidebarFooter");
    auto* signedIn = new QLabel(qs(session_.displayName));
    signedIn->setObjectName("SidebarUser");
    signedIn->setWordWrap(true);

    footerLayout->addWidget(roleLabel);
    footerLayout->addWidget(signedIn);
    footerLayout->addWidget(signOut);
    sidebarLayout->addWidget(footer);

    connect(signOut, &QPushButton::clicked, this, &MainWindow::onLogout);
}

void MainWindow::buildTopBar() {
    topBar_ = new QWidget;
    topBar_->setFixedHeight(70);
    topBar_->setObjectName("TopBar");

    auto* layout = new QHBoxLayout(topBar_);
    layout->setContentsMargins(24, 12, 24, 12);
    layout->setSpacing(12);

    windowTitle_ = new QLabel;
    windowTitle_->setObjectName("WindowTitle");

    auto* avatar = new Avatar(qs(session_.displayName));

    auto* identity = new QVBoxLayout;
    identity->setSpacing(1);
    nameLabel_ = new QLabel(qs(session_.displayName));
    nameLabel_->setObjectName("UserName");
    auto* meta = new QLabel(QStringLiteral("%1 · @%2")
                                .arg(qs(st::roleName(session_.role)), qs(session_.username)));
    meta->setObjectName("UserMeta");
    identity->addWidget(nameLabel_);
    identity->addWidget(meta);

    themeButton_ = ghostButton(QString(), activeTheme() == ThemeKind::Dark
                                         ? QStringLiteral("sun")
                                         : QStringLiteral("moon"));
    themeButton_->setToolTip(QStringLiteral("Switch between light and dark"));
    themeButton_->setFixedWidth(38);
    applyThemeIcon();

    layout->addWidget(windowTitle_, 1);
    layout->addWidget(avatar);
    layout->addLayout(identity);
    layout->addWidget(themeButton_);

    connect(themeButton_, &QPushButton::clicked, this, &MainWindow::onThemeToggled);
}

void MainWindow::applyThemeIcon() {
    if (!themeButton_) return;
    const bool dark = activeTheme() == ThemeKind::Dark;
    themeButton_->setIcon(icon(dark ? QStringLiteral("sun") : QStringLiteral("moon")));
    themeButton_->setToolTip(dark ? QStringLiteral("Switch to the light theme")
                                  : QStringLiteral("Switch to the dark theme"));
}

void MainWindow::onThemeToggled() {
    setTheme(activeTheme() == ThemeKind::Dark ? ThemeKind::Light : ThemeKind::Dark);
    applyThemeIcon();
    refreshCurrentPage();
}

Page* MainWindow::createPage(const QString& label) {
    if (label == QStringLiteral("Dashboard")) {
        return new DashboardPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Students")) {
        return new RosterPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Routes")) {
        return new RoutesPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Seat map") || label == QStringLiteral("My transport")) {
        return new SeatMapPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Payments & dues")) {
        return new PaymentsPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Complaints") || label == QStringLiteral("My complaints")) {
        return new ComplaintsPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Notices") || label == QStringLiteral("Notice board")) {
        return new NoticesPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Allotment card")) {
        return new AllotmentPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Staff") || label == QStringLiteral("Staff accounts")) {
        return new StaffAccountsPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Reports")) {
        return new ReportsPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("My profile")) {
        return new ProfilePage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Help & contact")) {
        return new HelpPage(controllers_, session_, stack_);
    }
    if (label == QStringLiteral("Settings")) {
        return new SettingsPage(controllers_, session_, stack_);
    }
    return nullptr;
}

void MainWindow::registerPages() {
    for (const QString& label : navLabels_) {
        Page* page = createPage(label);
        if (!page) continue;
        pages_.push_back(page);
        stack_->addWidget(page);
    }
}

void MainWindow::onNavClicked(QAbstractButton* button) {
    auto* navButton = qobject_cast<QPushButton*>(button);
    if (!navButton) return;

    const int index = static_cast<int>(navButtons_.indexOf(navButton));
    if (index < 0 || index >= pages_.size()) return;

    stack_->setCurrentIndex(index);
    windowTitle_->setText(navButton->text());
    refreshCurrentPage();
}

void MainWindow::refreshCurrentPage() {
    if (pages_.isEmpty()) return;
    const int index = stack_->currentIndex();
    if (index >= 0 && index < pages_.size() && pages_.at(index)) {
        pages_.at(index)->refresh();
    }
}

void MainWindow::onLogout() {
    const auto answer =
        QMessageBox::question(this, QStringLiteral("Sign out"),
                              QStringLiteral("Sign out of %1?").arg(qs(session_.displayName)),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    emit logoutRequested();
}

}  // namespace gui