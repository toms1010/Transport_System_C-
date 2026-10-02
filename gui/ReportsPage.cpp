#include "AppContext.hpp"
#include "ReportsPage.hpp"

#include "Notify.hpp"
#include "Theme.hpp"

#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {

ReportsPage::ReportsPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void ReportsPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Reports"),
        QStringLiteral("Transport statistics, route occupancy and the financial position."));
    auto* exportButton = primaryButton(QStringLiteral("Export payment ledger"),
                                       QStringLiteral("download"));
    header->addAction(exportButton);
    layout_->addWidget(header);

    auto* card = new Card;
    auto* title = new QLabel(QStringLiteral("Financial summary"));
    title->setObjectName("SectionTitle");
    financialSummary_ = new QLabel;
    financialSummary_->setWordWrap(true);
    card->body()->addWidget(title);
    card->body()->addWidget(financialSummary_);
    layout_->addWidget(card);

    auto* occupancyCard = new Card;
    auto* occupancyTitle = new QLabel(QStringLiteral("Route occupancy"));
    occupancyTitle->setObjectName("SectionTitle");
    occupancySummary_ = new QLabel;
    occupancySummary_->setObjectName("Muted");
    occupancyCard->body()->addWidget(occupancyTitle);
    occupancyCard->body()->addWidget(occupancySummary_);

    routeTable_ = new QTableWidget;
    routeTable_->setColumnCount(5);
    routeTable_->setHorizontalHeaderLabels({QStringLiteral("Route"), QStringLiteral("Fare"),
                                            QStringLiteral("Occupied"),
                                            QStringLiteral("Capacity"),
                                            QStringLiteral("Utilisation")});
    routeTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 4; ++column) {
        routeTable_->horizontalHeader()->setSectionResizeMode(column,
                                                              QHeaderView::ResizeToContents);
    }
    routeTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    configureTable(routeTable_);
    routeTable_->setMinimumHeight(220);
    occupancyCard->body()->addWidget(routeTable_);
    layout_->addWidget(occupancyCard);

    auto* duo = new QHBoxLayout;
    duo->setSpacing(14);

    auto* studentsCard = new Card;
    auto* studentsTitle = new QLabel(QStringLiteral("Students by route"));
    studentsTitle->setObjectName("SectionTitle");
    studentTable_ = new QTableWidget;
    studentTable_->setColumnCount(4);
    studentTable_->setHorizontalHeaderLabels({QStringLiteral("Route"),
                                              QStringLiteral("Students"),
                                              QStringLiteral("Collected"),
                                              QStringLiteral("Outstanding")});
    studentTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 4; ++column) {
        studentTable_->horizontalHeader()->setSectionResizeMode(column,
                                                                QHeaderView::ResizeToContents);
    }
    configureTable(studentTable_);
    studentTable_->setMinimumHeight(220);
    studentsCard->body()->addWidget(studentsTitle);
    studentsCard->body()->addWidget(studentTable_);
    duo->addWidget(studentsCard, 1);

    auto* complaintsCard = new Card;
    auto* complaintsTitle = new QLabel(QStringLiteral("Complaints"));
    complaintsTitle->setObjectName("SectionTitle");
    complaintSummary_ = new QLabel;
    complaintSummary_->setWordWrap(true);
    complaintsCard->body()->addWidget(complaintsTitle);
    complaintsCard->body()->addWidget(complaintSummary_);
    complaintsCard->body()->addStretch();
    duo->addWidget(complaintsCard, 1);

    layout_->addLayout(duo);
    layout_->addStretch();

    connect(exportButton, &QPushButton::clicked, this, &ReportsPage::exportLedger);
}

void ReportsPage::refresh() {
    const AggregateView totals = controllers_.payments().aggregates();
    const long long expected = totals.collected + totals.outstanding;

    financialSummary_->setText(QStringLiteral("Expected        %1\n"
                                              "Collected       %2\n"
                                              "Outstanding     %3\n"
                                              "Students        %4 total, %5 with dues")
                                   .arg(money(expected), money(totals.collected),
                                        money(totals.outstanding),
                                        QString::number(totals.totalStudents),
                                        QString::number(totals.studentsWithDues)));

    occupancySummary_->setText(
        QStringLiteral("%1 of %2 seats occupied across %3 routes")
            .arg(QString::number(totals.occupiedSeats), QString::number(totals.totalCapacity),
                 QString::number(totals.routes)));

    routeTable_->setRowCount(0);
    int row = 0;
    for (const RouteRow& route : controllers_.routes().rows()) {
        routeTable_->insertRow(row);
        routeTable_->setItem(row, 0, textItem(QStringLiteral("%1 · %2").arg(route.id, route.name)));
        routeTable_->setItem(row, 1, moneyItem(route.fare));
        routeTable_->setItem(row, 2, centeredItem(QString::number(route.occupied)));
        routeTable_->setItem(row, 3, centeredItem(QString::number(route.capacity)));

        auto* bar = new QProgressBar;
        bar->setTextVisible(false);
        bar->setFixedHeight(9);
        bar->setMaximum(100);
        bar->setValue(route.occupancyPercent);
        routeTable_->setCellWidget(row, 4, bar);
        ++row;
    }
    showEmptyState(routeTable_, QStringLiteral("No routes have been created."));

    studentTable_->setRowCount(0);
    row = 0;
    for (const RouteRow& route : controllers_.routes().rows()) {
        int onRoute = 0;
        long long collected = 0;
        long long outstanding = 0;

        RosterFilter filter;
        filter.routeId = route.id;
        for (const StudentRow& student : controllers_.students().roster(filter)) {
            ++onRoute;
            collected += student.paid;
            outstanding += student.outstanding;
        }

        studentTable_->insertRow(row);
        studentTable_->setItem(row, 0, textItem(route.name));
        studentTable_->setItem(row, 1, centeredItem(QString::number(onRoute)));
        studentTable_->setItem(row, 2, moneyItem(collected));
        studentTable_->setItem(row, 3, moneyItem(outstanding));
        ++row;
    }
    showEmptyState(studentTable_, QStringLiteral("No students are allotted to any route yet."));

    const int open = controllers_.complaints().openCount();
    const int all =
        static_cast<int>(controllers_.complaints().list(ComplaintFilter::All, {}).size());
    complaintSummary_->setText(QStringLiteral("Total      %1\nOpen       %2\nResolved   %3")
                                   .arg(QString::number(all), QString::number(open),
                                        QString::number(all - open)));
}

void ReportsPage::exportLedger() {
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export payment ledger"),
        QStringLiteral("payment-ledger.csv"), QStringLiteral("CSV files (*.csv)"));
    if (path.isEmpty()) return;

    QString message;
    BusyOverlay overlay(this, QStringLiteral("Exporting…"));
    overlay.show();
    QCoreApplication::processEvents();
    const bool ok = controllers_.payments().exportLedger(path, message);
    overlay.hide();

    if (!ok) {
        warn(QStringLiteral("Export failed"), message);
        return;
    }
    toast(message);
}

}  // namespace gui
