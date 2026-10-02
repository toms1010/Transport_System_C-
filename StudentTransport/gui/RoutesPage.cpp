#include "AppContext.hpp"
#include "RoutesPage.hpp"

#include "Theme.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace gui {

RouteDialog::RouteDialog(const RouteInput& input, bool editing, QWidget* parent)
    : QDialog(parent), editing_(editing) {
    setWindowTitle(editing_ ? QStringLiteral("Edit route") : QStringLiteral("Add route"));
    setModal(true);
    setMinimumWidth(460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(14);

    auto* heading = new QLabel(editing_ ? QStringLiteral("Edit route")
                                         : QStringLiteral("Add a new route"));
    heading->setObjectName("PageTitle");
    auto* subtitle = new QLabel(
        QStringLiteral("Fares and capacity drive what students owe on this route."));
    subtitle->setObjectName("PageSubtitle");
    subtitle->setWordWrap(true);
    root->addWidget(heading);
    root->addWidget(subtitle);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    id_ = new QLineEdit(input.id);
    id_->setPlaceholderText(QStringLiteral("e.g. R1"));
    id_->setEnabled(!editing_);

    name_ = new QLineEdit(input.name);
    name_->setPlaceholderText(QStringLiteral("e.g. Ameerpet - Uppal"));

    from_ = new QLineEdit(input.from);
    from_->setPlaceholderText(QStringLiteral("Starting point"));

    to_ = new QLineEdit(input.to);
    to_->setPlaceholderText(QStringLiteral("Ending point"));

    via_ = new QLineEdit(input.via);
    via_->setPlaceholderText(QStringLiteral("Optional"));

    fare_ = new QSpinBox;
    fare_->setRange(0, 100000000);
    fare_->setValue(static_cast<int>(input.fare));
    fare_->setPrefix(QStringLiteral("₹ "));

    capacity_ = new QSpinBox;
    capacity_->setRange(0, 500);
    capacity_->setValue(input.capacity);

    active_ = new QCheckBox(QStringLiteral("Route is active"));
    active_->setChecked(input.active);

    form->addRow(QStringLiteral("Route id"), id_);
    form->addRow(QStringLiteral("Route name"), name_);
    form->addRow(QStringLiteral("From"), from_);
    form->addRow(QStringLiteral("To"), to_);
    form->addRow(QStringLiteral("Via"), via_);
    form->addRow(QStringLiteral("Monthly fare"), fare_);
    form->addRow(QStringLiteral("Seat capacity"), capacity_);
    form->addRow(QString(), active_);

    error_ = new QLabel;
    error_->setWordWrap(true);
    error_->setStyleSheet("color: " + color::danger().name() + "; font-size: 12px;");
    error_->hide();

    auto* buttons = new QDialogButtonBox;
    auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
    auto* save = buttons->addButton(editing_ ? QStringLiteral("Save changes")
                                            : QStringLiteral("Create route"),
                                    QDialogButtonBox::AcceptRole);
    save->setObjectName("PrimaryButton");

    root->addLayout(form);
    root->addWidget(error_);
    root->addStretch();
    root->addWidget(buttons);

    connect(save, &QPushButton::clicked, this, &RouteDialog::submit);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

RouteInput RouteDialog::input() const {
    RouteInput result;
    result.id = id_->text().trimmed();
    result.name = name_->text().trimmed();
    result.from = from_->text().trimmed();
    result.to = to_->text().trimmed();
    result.via = via_->text().trimmed();
    result.fare = fare_->value();
    result.capacity = capacity_->value();
    result.active = active_->isChecked();
    return result;
}

void RouteDialog::setError(const QString& message) {
    error_->setText(message);
    error_->show();
}

void RouteDialog::submit() {
    if (name_->text().trimmed().isEmpty()) {
        setError(QStringLiteral("A route name is required."));
        return;
    }
    accept();
}

RoutesPage::RoutesPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    auto* header = new PageHeader(QStringLiteral("Route management"),
                                  QStringLiteral("Create the routes students are allotted to and "
                                                 "keep fares and capacity current."));
    auto* addButton = primaryButton(QStringLiteral("Add route"), QStringLiteral("add"));
    header->addAction(addButton);
    layout_->addWidget(header);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    auto* editButton = ghostButton(QStringLiteral("Edit"), QStringLiteral("edit"));
    auto* toggleButton = ghostButton(QStringLiteral("Activate / deactivate"),
                                     QStringLiteral("refresh"));
    auto* deleteButton = dangerButton(QStringLiteral("Delete"), QStringLiteral("delete"));
    for (QPushButton* button : {editButton, toggleButton, deleteButton}) {
        toolbar->addWidget(button);
    }
    toolbar->addStretch();
    layout_->addLayout(toolbar);

    auto* card = new Card;
    table_ = new QTableWidget;
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Route"),
                                       QStringLiteral("Endpoints"), QStringLiteral("Fare"),
                                       QStringLiteral("Occupied"), QStringLiteral("Free"),
                                       QStringLiteral("Status")});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    for (int column = 3; column < 7; ++column) {
        table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    configureTable(table_);
    table_->setMinimumHeight(380);
    card->body()->addWidget(table_);
    layout_->addWidget(card);
    layout_->addStretch();

    connect(addButton, &QPushButton::clicked, this, &RoutesPage::addRoute);
    connect(editButton, &QPushButton::clicked, this, &RoutesPage::editRoute);
    connect(toggleButton, &QPushButton::clicked, this, &RoutesPage::toggleActive);
    connect(deleteButton, &QPushButton::clicked, this, &RoutesPage::deleteRoute);
    connect(table_, &QTableWidget::itemDoubleClicked, this, &RoutesPage::editRoute);
}

QString RoutesPage::selectedRouteId() const {
    const int row = table_->currentRow();
    if (row < 0) return {};
    QTableWidgetItem* item = table_->item(row, 0);
    return item ? item->text() : QString();
}

RouteInput RoutesPage::selectedInput() const {
    const QString routeId = selectedRouteId();
    for (const RouteRow& route : controllers_.routes().rows()) {
        if (route.id != routeId) continue;
        RouteInput input;
        input.id = route.id;
        input.name = route.name;
        input.from = route.from;
        input.to = route.to;
        input.via = route.via;
        input.fare = route.fare;
        input.capacity = route.capacity;
        input.active = route.active;
        return input;
    }
    return {};
}

void RoutesPage::refresh() {
    table_->setRowCount(0);
    int row = 0;
    for (const RouteRow& route : controllers_.routes().rows()) {
        table_->insertRow(row);
        table_->setItem(row, 0, textItem(route.id));
        table_->setItem(row, 1, textItem(route.name));
        table_->setItem(row, 2, textItem(route.endpoints));
        table_->setItem(row, 3, moneyItem(route.fare));
        table_->setItem(row, 4, centeredItem(QStringLiteral("%1 / %2")
                                                 .arg(QString::number(route.occupied),
                                                      QString::number(route.capacity))));
        table_->setItem(row, 5, centeredItem(QString::number(route.free)));
        table_->setItem(row, 6,
                        statusItem(route.active ? QStringLiteral("Active")
                                                : QStringLiteral("Inactive"),
                                   route.active ? QStringLiteral("success")
                                                : QStringLiteral("warning")));
        ++row;
    }
    showEmptyState(table_, QStringLiteral("No routes yet. Use “Add route” to create one."));
}

void RoutesPage::addRoute() {
    RouteDialog dialog(RouteInput(), false, this);
    if (dialog.exec() != QDialog::Accepted) return;
    toast(controllers_.routes().add(dialog.input()));
    refresh();
}

void RoutesPage::editRoute() {
    const QString routeId = selectedRouteId();
    if (routeId.isEmpty()) {
        warn(QStringLiteral("No route selected"),
             QStringLiteral("Select a route in the table first."));
        return;
    }

    RouteDialog dialog(selectedInput(), true, this);
    if (dialog.exec() != QDialog::Accepted) return;
    toast(controllers_.routes().update(dialog.input()));
    refresh();
}

void RoutesPage::toggleActive() {
    const QString routeId = selectedRouteId();
    if (routeId.isEmpty()) {
        warn(QStringLiteral("No route selected"),
             QStringLiteral("Select a route in the table first."));
        return;
    }

    const RouteInput input = selectedInput();
    toast(controllers_.routes().setActive(routeId, !input.active));
    refresh();
}

void RoutesPage::deleteRoute() {
    const QString routeId = selectedRouteId();
    if (routeId.isEmpty()) {
        warn(QStringLiteral("No route selected"),
             QStringLiteral("Select a route in the table first."));
        return;
    }

    const int occupied = controllers_.routes().occupiedOf(routeId);
    const QString body = occupied > 0
                             ? QStringLiteral("%1 still has %2 student(s) allotted. Reassign "
                                              "them before deleting it.")
                                   .arg(routeId)
                                   .arg(QString::number(occupied))
                             : QStringLiteral("Delete route %1? This cannot be undone.")
                                   .arg(routeId);

    if (!confirm(QStringLiteral("Delete route"), body, QStringLiteral("Delete"))) return;

    toast(controllers_.routes().remove(routeId));
    refresh();
}

}  // namespace gui