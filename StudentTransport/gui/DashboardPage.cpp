#include "DashboardPage.hpp"

#include "AppContext.hpp"
#include "ComplaintsPage.hpp"
#include "NoticesPage.hpp"
#include "Theme.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {
namespace {

QString statusOf(const StatementView& statement) {
    if (statement.settled && !statement.awaitingSeat) return QStringLiteral("PAID IN FULL");
    if (statement.outstanding > 0) return QStringLiteral("DUES PENDING");
    return QStringLiteral("AWAITING SEAT");
}

}  // namespace

DashboardPage::DashboardPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void DashboardPage::build() {
    auto* greeting = new QLabel(QStringLiteral("Welcome back, %1").arg(qs(session_.displayName)));
    greeting->setObjectName("PageTitle");

    auto* subtitle = new QLabel(
        isOperator()
            ? qs(controllers_.context().config().config().departmentName) +
                  QStringLiteral(" · ") + qs(controllers_.context().config().config().contactHours)
            : qs(controllers_.context().config().config().contactName) +
                  QStringLiteral(" · ") + qs(controllers_.context().config().config().contactPhone));
    subtitle->setObjectName("PageSubtitle");

    layout_->addWidget(greeting);
    layout_->addWidget(subtitle);

    if (session_.role == st::Role::Student) {
        buildStudentView();
    } else {
        buildStaffView();
    }

    layout_->addStretch();
}

StatCard* DashboardPage::addStat(QGridLayout* grid, int row, int column, const QString& key,
                                 const QString& label) {
    auto* card = new StatCard(label);
    grid->addWidget(card, row, column);
    stats_.insert(key, card);
    return card;
}

void DashboardPage::buildStudentView() {
    // Three headline cards: route, seat, payment (spec section 11).
    auto* cards = new QWidget;
    auto* grid = new QGridLayout(cards);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(14);

    auto* routeCard = new Card;
    auto* routeTitle = new QLabel(QStringLiteral("🚌  ROUTE"));
    routeTitle->setObjectName("StatLabel");
    routeName_ = new QLabel(QStringLiteral("—"));
    routeName_->setObjectName("StatValue");
    routeName_->setStyleSheet("font-size: 17px;");
    routeName_->setWordWrap(true);
    routeVia_ = new QLabel(QStringLiteral("—"));
    routeVia_->setObjectName("Muted");
    routeVia_->setWordWrap(true);
    routeCard->body()->addWidget(routeTitle);
    routeCard->body()->addWidget(routeName_);
    routeCard->body()->addWidget(routeVia_);
    grid->addWidget(routeCard, 0, 0);

    auto* seatCard = new Card;
    auto* seatTitle = new QLabel(QStringLiteral("💺  SEAT"));
    seatTitle->setObjectName("StatLabel");
    seatLabel_ = new QLabel(QStringLiteral("—"));
    seatLabel_->setObjectName("StatValue");
    auto* seatCaption = new QLabel(QStringLiteral("Open the seat map to change it"));
    seatCaption->setObjectName("StatLabel");
    seatCard->body()->addWidget(seatTitle);
    seatCard->body()->addWidget(seatLabel_);
    seatCard->body()->addWidget(seatCaption);
    grid->addWidget(seatCard, 0, 1);

    auto* paymentCard = new Card;
    auto* paymentTitle = new QLabel(QStringLiteral("💳  PAYMENT"));
    paymentTitle->setObjectName("StatLabel");
    paymentLabel_ = new QLabel(QStringLiteral("—"));
    paymentLabel_->setObjectName("StatValue");
    paymentCaption_ = new QLabel;
    paymentCaption_->setObjectName("StatLabel");
    feeBar_ = new QProgressBar;
    feeBar_->setTextVisible(false);
    feeBar_->setFixedHeight(9);
    paymentCard->body()->addWidget(paymentTitle);
    paymentCard->body()->addWidget(paymentLabel_);
    paymentCard->body()->addWidget(paymentCaption_);
    paymentCard->body()->addWidget(feeBar_);
    grid->addWidget(paymentCard, 0, 2);

    layout_->addWidget(cards);

    auto* lower = new QHBoxLayout;
    lower->setSpacing(14);

    auto* noticesCard = new Card;
    auto* noticesHeader = new QHBoxLayout;
    auto* noticesTitle = new QLabel(QStringLiteral("📢  Latest notices"));
    noticesTitle->setObjectName("SectionTitle");
    viewNotices_ = ghostButton(QStringLiteral("View all"));
    noticesHeader->addWidget(noticesTitle);
    noticesHeader->addStretch();
    noticesHeader->addWidget(viewNotices_);
    noticesCard->body()->addLayout(noticesHeader);

    notices_ = new QTableWidget;
    notices_->setColumnCount(2);
    notices_->setHorizontalHeaderLabels({QStringLiteral("Notice"), QStringLiteral("Posted")});
    notices_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    notices_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    configureTable(notices_);
    notices_->setMinimumHeight(210);
    noticesCard->body()->addWidget(notices_);
    lower->addWidget(noticesCard, 1);

    auto* complaintsCard = new Card;
    auto* complaintsHeader = new QHBoxLayout;
    auto* complaintsTitle = new QLabel(QStringLiteral("📝  Recent complaints"));
    complaintsTitle->setObjectName("SectionTitle");
    viewComplaints_ = ghostButton(QStringLiteral("View all"));
    complaintsHeader->addWidget(complaintsTitle);
    complaintsHeader->addStretch();
    complaintsHeader->addWidget(viewComplaints_);
    complaintsCard->body()->addLayout(complaintsHeader);

    complaints_ = new QTableWidget;
    complaints_->setColumnCount(2);
    complaints_->setHorizontalHeaderLabels({QStringLiteral("Subject"), QStringLiteral("Status")});
    complaints_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    complaints_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    configureTable(complaints_);
    complaints_->setMinimumHeight(210);
    complaintsCard->body()->addWidget(complaints_);
    lower->addWidget(complaintsCard, 1);

    layout_->addLayout(lower);

    connect(viewNotices_, &QPushButton::clicked, this, [this] {
        if (auto* page = siblingPage<NoticesPage>()) activate(page);
    });
    connect(viewComplaints_, &QPushButton::clicked, this, [this] {
        if (auto* page = siblingPage<ComplaintsPage>()) activate(page);
    });
}

void DashboardPage::buildStaffView() {
    auto* cards = new QWidget;
    auto* grid = new QGridLayout(cards);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(14);

    const AggregateView totals = controllers_.payments().aggregates();

    addStat(grid, 0, 0, QStringLiteral("students"), QStringLiteral("Students"))
        ->setValue(QString::number(totals.totalStudents));
    addStat(grid, 0, 1, QStringLiteral("routes"), QStringLiteral("Routes"))
        ->setValue(QString::number(totals.routes));
    addStat(grid, 0, 2, QStringLiteral("complaints"), QStringLiteral("Open complaints"))
        ->setValue(QString::number(controllers_.complaints().openCount()));
    addStat(grid, 1, 0, QStringLiteral("occupied"), QStringLiteral("Occupied seats"))
        ->setValue(QString::number(totals.occupiedSeats));
    addStat(grid, 1, 1, QStringLiteral("available"), QStringLiteral("Available seats"))
        ->setValue(QString::number(totals.freeSeats));
    if (StatCard* dues = addStat(grid, 1, 2, QStringLiteral("dues"), QStringLiteral("Pending dues"))) {
        dues->setValue(money(totals.outstanding));
        dues->setAccent(totals.outstanding > 0 ? color::danger().name() : color::success().name());
        dues->setCaption(QStringLiteral("%1 student(s) with dues")
                             .arg(QString::number(totals.studentsWithDues)));
    }

    layout_->addWidget(cards);

    auto* occupancy = new Card;
    auto* occupancyTitle = new QLabel(QStringLiteral("Route occupancy"));
    occupancyTitle->setObjectName("SectionTitle");
    occupancy->body()->addWidget(occupancyTitle);

    routes_ = new QTableWidget;
    routes_->setColumnCount(5);
    routes_->setHorizontalHeaderLabels({QStringLiteral("Route"), QStringLiteral("Endpoints"),
                                        QStringLiteral("Occupied"), QStringLiteral("Fare"),
                                        QStringLiteral("Free seats")});
    routes_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    routes_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int column = 2; column < 5; ++column) {
        routes_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    configureTable(routes_);
    routes_->setMinimumHeight(220);
    occupancy->body()->addWidget(routes_);
    layout_->addWidget(occupancy);

    auto* activityCard = new Card;
    auto* activityTitle = new QLabel(QStringLiteral("Recent activity"));
    activityTitle->setObjectName("SectionTitle");
    activityCard->body()->addWidget(activityTitle);

    activity_ = new QTableWidget;
    activity_->setColumnCount(4);
    activity_->setHorizontalHeaderLabels({QStringLiteral("When"), QStringLiteral("Activity"),
                                          QStringLiteral("Detail"), QStringLiteral("Status")});
    activity_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    activity_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    activity_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    activity_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    configureTable(activity_);
    activity_->setMinimumHeight(200);
    activityCard->body()->addWidget(activity_);
    layout_->addWidget(activityCard);
}

void DashboardPage::refresh() {
    if (session_.role == st::Role::Student) {
        refreshStudent();
    } else {
        refreshStaff();
    }
}

void DashboardPage::refreshStudent() {
    const StatementView statement = controllers_.payments().statement(qs(session_.userId));
    if (!statement.valid) return;

    routeName_->setText(statement.routeName);
    const QString routeId = controllers_.seats().routeIdOf(qs(session_.userId));
    const RouteOption* option = nullptr;
    const QVector<RouteOption> options = controllers_.routes().options(false);
    for (const RouteOption& candidate : options) {
        if (candidate.id == routeId) {
            option = &candidate;
            break;
        }
    }
    routeVia_->setText(option && !option->via.isEmpty()
                           ? QStringLiteral("via %1").arg(option->via)
                           : QStringLiteral("No route selected yet"));

    if (controllers_.seats().hasSeat(qs(session_.userId))) {
        seatLabel_->setText(QStringLiteral("%1").arg(controllers_.seats().seatOf(qs(session_.userId))));
    } else {
        seatLabel_->setText(QStringLiteral("Not allotted"));
    }

    paymentLabel_->setText(money(statement.outstanding));
    paymentLabel_->setStyleSheet(statement.outstanding > 0
                                     ? QStringLiteral("color: ") + color::danger().name() +
                                           QStringLiteral("; font-size: 24px; font-weight: 700;")
                                     : QStringLiteral("color: ") + color::success().name() +
                                           QStringLiteral("; font-size: 24px; font-weight: 700;"));
    paymentCaption_->setText(statusOf(statement));

    int paidPercent = 0;
    if (statement.totalCharge > 0) {
        paidPercent = static_cast<int>(statement.totalPaid * 100 / statement.totalCharge);
    }
    feeBar_->setValue(paidPercent);

    notices_->setRowCount(0);
    int row = 0;
    const QVector<NoticeRow> notices = controllers_.notices().rows();
    for (const NoticeRow& notice : notices) {
        if (row >= 4) break;
        notices_->insertRow(row);
        notices_->setItem(row, 0, textItem(notice.title));
        notices_->setItem(row, 1, textItem(notice.createdAt));
        ++row;
    }
    showEmptyState(notices_, QStringLiteral("No notices have been published."));

    complaints_->setRowCount(0);
    row = 0;
    const QVector<ComplaintRow> complaints =
        controllers_.complaints().forStudent(qs(session_.userId));
    for (const ComplaintRow& complaint : complaints) {
        if (row >= 4) break;
        complaints_->insertRow(row);
        complaints_->setItem(row, 0, textItem(complaint.subject));
        complaints_->setItem(row, 1, statusItem(complaint.statusLabel, complaint.statusKind));
        ++row;
    }
    showEmptyState(complaints_, QStringLiteral("You have not raised any complaints."));
}

void DashboardPage::refreshStaff() {
    const AggregateView totals = controllers_.payments().aggregates();

    if (StatCard* card = stat(QStringLiteral("students"))) {
        card->setValue(QString::number(totals.totalStudents));
    }
    if (StatCard* card = stat(QStringLiteral("routes"))) {
        card->setValue(QString::number(totals.routes));
    }
    if (StatCard* card = stat(QStringLiteral("complaints"))) {
        card->setValue(QString::number(controllers_.complaints().openCount()));
    }
    if (StatCard* card = stat(QStringLiteral("occupied"))) {
        card->setValue(QString::number(totals.occupiedSeats));
        card->setCaption(QStringLiteral("of %1 capacity").arg(QString::number(totals.totalCapacity)));
    }
    if (StatCard* card = stat(QStringLiteral("available"))) {
        card->setValue(QString::number(totals.freeSeats));
    }
    if (StatCard* card = stat(QStringLiteral("dues"))) {
        card->setValue(money(totals.outstanding));
        card->setAccent(totals.outstanding > 0 ? color::danger().name() : color::success().name());
        card->setCaption(QStringLiteral("%1 student(s) with dues")
                             .arg(QString::number(totals.studentsWithDues)));
    }

    routes_->setRowCount(0);
    int row = 0;
    for (const RouteRow& route : controllers_.routes().rows()) {
        routes_->insertRow(row);
        routes_->setItem(row, 0, textItem(route.name));
        routes_->setItem(row, 1, textItem(route.endpoints));
        routes_->setItem(row, 2, centeredItem(QStringLiteral("%1 / %2")
                                                  .arg(QString::number(route.occupied),
                                                       QString::number(route.capacity))));
        routes_->setItem(row, 3, moneyItem(route.fare));
        routes_->setItem(row, 4, centeredItem(QString::number(route.free)));
        ++row;
    }
    showEmptyState(routes_, QStringLiteral("No routes have been created."));

    activity_->setRowCount(0);
    row = 0;

    for (const ComplaintRow& complaint : controllers_.complaints().list(ComplaintFilter::All, {})) {
        if (row >= 8) break;
        activity_->insertRow(row);
        activity_->setItem(row, 0, textItem(complaint.createdAt));
        activity_->setItem(row, 1, textItem(complaint.resolved
                                                ? QStringLiteral("Complaint resolved")
                                                : QStringLiteral("Complaint raised")));
        activity_->setItem(row, 2, textItem(QStringLiteral("%1 · %2")
                                                .arg(complaint.subject, complaint.studentName)));
        activity_->setItem(row, 3, statusItem(complaint.statusLabel, complaint.statusKind));
        ++row;
    }
    showEmptyState(activity_, QStringLiteral("No activity to show yet."));
}

}  // namespace gui