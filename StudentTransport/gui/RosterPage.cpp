#include "AppContext.hpp"
#include "RosterPage.hpp"

#include "PaymentsPage.hpp"
#include "SeatMapPage.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {

RosterPage::RosterPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void RosterPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Student management"),
        QStringLiteral("Every enrolled student with their route, seat and outstanding fees."));
    layout_->addWidget(header);

    auto* filters = new QHBoxLayout;
    filters->setSpacing(10);

    auto* searchCaption = new QLabel(QStringLiteral("Search"));
    searchCaption->setObjectName("FieldLabel");
    search_ = new QLineEdit;
    search_->setPlaceholderText(QStringLiteral("Name, username or id"));
    search_->setMinimumWidth(240);
    search_->setClearButtonEnabled(true);

    routeFilter_ = new QComboBox;
    routeFilter_->setMinimumWidth(220);
    routeFilter_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    duesFilter_ = new QComboBox;
    duesFilter_->setMinimumWidth(180);
    duesFilter_->addItem(QStringLiteral("Any fee status"), static_cast<int>(DuesFilter::Any));
    duesFilter_->addItem(QStringLiteral("With dues"), static_cast<int>(DuesFilter::WithDues));
    duesFilter_->addItem(QStringLiteral("Fully paid"), static_cast<int>(DuesFilter::FullyPaid));
    duesFilter_->addItem(QStringLiteral("No seat allotted"),
                         static_cast<int>(DuesFilter::NoSeat));

    filters->addWidget(searchCaption);
    filters->addWidget(search_);
    filters->addWidget(routeFilter_);
    filters->addWidget(duesFilter_);
    filters->addStretch();
    layout_->addLayout(filters);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(8);
    auto* seatButton = ghostButton(QStringLiteral("Allot a seat"), QStringLiteral("seat"));
    auto* payButton = ghostButton(QStringLiteral("Record payment"), QStringLiteral("payment"));
    auto* clearButton = ghostButton(QStringLiteral("Clear filters"), QStringLiteral("close"));
    for (QPushButton* button : {seatButton, payButton, clearButton}) {
        actions->addWidget(button);
    }
    actions->addStretch();
    layout_->addLayout(actions);

    auto* card = new Card;
    summary_ = new QLabel;
    summary_->setObjectName("Muted");
    card->body()->addWidget(summary_);

    table_ = new QTableWidget;
    table_->setColumnCount(8);
    table_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Student"),
                                       QStringLiteral("Contact"), QStringLiteral("Route"),
                                       QStringLiteral("Seat"), QStringLiteral("Total fees"),
                                       QStringLiteral("Outstanding"),
                                       QStringLiteral("Status")});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    for (int column = 4; column < 8; ++column) {
        table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    configureTable(table_);
    table_->setMinimumHeight(380);
    card->body()->addWidget(table_);
    layout_->addWidget(card);
    layout_->addStretch();

    connect(search_, &QLineEdit::textChanged, this, &RosterPage::applyFilters);
    connect(routeFilter_, &QComboBox::currentIndexChanged, this, &RosterPage::applyFilters);
    connect(duesFilter_, &QComboBox::currentIndexChanged, this, &RosterPage::applyFilters);
    connect(clearButton, &QPushButton::clicked, this, &RosterPage::clearFilters);
    connect(seatButton, &QPushButton::clicked, this, &RosterPage::openSeatMapFor);
    connect(payButton, &QPushButton::clicked, this, &RosterPage::openPaymentsFor);
}

void RosterPage::rebuildFilters() {
    const QString previous = routeFilter_->currentData().toString();
    routeFilter_->blockSignals(true);
    routeFilter_->clear();
    routeFilter_->addItem(QStringLiteral("All routes"), QString());

    for (const RouteRow& route : controllers_.routes().rows()) {
        routeFilter_->addItem(route.name, route.id);
    }

    const int index = routeFilter_->findData(previous);
    if (index >= 0) routeFilter_->setCurrentIndex(index);
    routeFilter_->blockSignals(false);
}

void RosterPage::refresh() {
    rebuildFilters();
    applyFilters();
}

void RosterPage::clearFilters() {
    search_->clear();
    routeFilter_->setCurrentIndex(0);
    duesFilter_->setCurrentIndex(0);
}

QString RosterPage::selectedStudentId() const {
    const int row = table_->currentRow();
    if (row < 0) return {};
    QTableWidgetItem* item = table_->item(row, 0);
    return item ? item->text() : QString();
}

void RosterPage::applyFilters() {
    RosterFilter filter;
    filter.search = search_->text().trimmed();
    filter.routeId = routeFilter_->currentData().toString();
    filter.dues = static_cast<DuesFilter>(duesFilter_->currentData().toInt());

    const QVector<StudentRow> rows = controllers_.students().roster(filter);

    table_->setRowCount(0);
    int row = 0;
    for (const StudentRow& student : rows) {
        table_->insertRow(row);
        table_->setItem(row, 0, textItem(student.id));

        auto* nameItem = textItem(student.name);
        nameItem->setToolTip(QStringLiteral("@%1").arg(student.username));
        table_->setItem(row, 1, nameItem);

        table_->setItem(row, 2, textItem(student.phone));
        table_->setItem(row, 3, textItem(student.routeName));
        table_->setItem(row, 4, centeredItem(student.seatLabel));
        table_->setItem(row, 5, moneyItem(student.total));
        table_->setItem(row, 6, moneyItem(student.outstanding));
        table_->setItem(row, 7, statusItem(student.statusLabel, student.statusKind));
        ++row;
    }

    summary_->setText(QStringLiteral("Showing %1 of %2 students")
                           .arg(QString::number(rows.size()),
                                QString::number(controllers_.students().count())));
    showEmptyState(table_, QStringLiteral("No students match these filters."));
}

void RosterPage::openSeatMapFor() {
    const QString id = selectedStudentId();
    if (id.isEmpty()) {
        warn(QStringLiteral("No student selected"),
             QStringLiteral("Select a student in the table first."));
        return;
    }
    if (auto* page = siblingPage<SeatMapPage>()) {
        page->selectStudent(id);
        activate(page);
    }
}

void RosterPage::openPaymentsFor() {
    const QString id = selectedStudentId();
    if (id.isEmpty()) {
        warn(QStringLiteral("No student selected"),
             QStringLiteral("Select a student in the table first."));
        return;
    }
    if (auto* page = siblingPage<PaymentsPage>()) {
        page->selectStudent(id);
        activate(page);
    }
}

}  // namespace gui
