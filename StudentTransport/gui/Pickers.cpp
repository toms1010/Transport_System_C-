#include "Pickers.hpp"

#include "Theme.hpp"
#include "Widgets.hpp"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace gui {

RoutePickerDialog::RoutePickerDialog(RouteController& routes, QWidget* parent)
    : QDialog(parent), routes_(routes) {
    setWindowTitle(QStringLiteral("Select a transport route"));
    setModal(true);
    setMinimumSize(680, 560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(14);

    auto* heading = new QLabel(QStringLiteral("Select a transport route"));
    heading->setObjectName("PageTitle");
    auto* subtitle =
        new QLabel(QStringLiteral("Only routes with at least one free seat are listed."));
    subtitle->setObjectName("PageSubtitle");
    root->addWidget(heading);
    root->addWidget(subtitle);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* host = new QWidget;
    list_ = new QVBoxLayout(host);
    list_->setContentsMargins(0, 0, 0, 0);
    list_->setSpacing(12);
    scroll->setWidget(host);
    root->addWidget(scroll, 1);

    auto* buttons = new QDialogButtonBox;
    auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
    confirmButton_ = buttons->addButton(QStringLiteral("Continue"), QDialogButtonBox::AcceptRole);
    confirmButton_->setObjectName("PrimaryButton");
    confirmButton_->setEnabled(false);
    root->addWidget(buttons);

    connect(confirmButton_, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    rebuild();
}

void RoutePickerDialog::rebuild() {
    while (QLayoutItem* item = list_->takeAt(0)) {
        if (QWidget* widget = item->widget()) widget->deleteLater();
        delete item;
    }

    confirmButton_->setEnabled(false);

    const QVector<RouteOption> options = routes_.options(true);
    if (options.isEmpty()) {
        auto* empty = new QLabel(
            QStringLiteral("No active route currently has a free seat. Ask the transport office "
                           "to add capacity."));
        empty->setObjectName("Muted");
        empty->setWordWrap(true);
        list_->addWidget(empty);
        return;
    }

    for (const RouteOption& option : options) {
        auto* card = new Card;
        card->setObjectName("TintedCard");

        auto* header = new QHBoxLayout;
        auto* name = new QLabel(QStringLiteral("%1 · %2").arg(option.id, option.name));
        name->setObjectName("SectionTitle");
        auto* pick = primaryButton(QStringLiteral("Select route"));
        pick->setEnabled(option.free > 0);
        header->addWidget(name);
        header->addStretch();
        header->addWidget(pick);
        card->body()->addLayout(header);

        auto* endpoints = new QLabel(option.endpoints);
        endpoints->setObjectName("FieldValue");
        card->body()->addWidget(endpoints);

        auto* via = new QLabel(QStringLiteral("via %1").arg(option.via));
        via->setObjectName("Muted");
        card->body()->addWidget(via);

        auto* facts = new QLabel(QStringLiteral("Fare %1   ·   Capacity %2   ·   %3 free")
                                     .arg(money(option.fare), QString::number(option.capacity),
                                          QString::number(option.free)));
        facts->setObjectName("Muted");
        card->body()->addWidget(facts);

        const QString routeId = option.id;
        connect(pick, &QPushButton::clicked, this, [this, routeId] {
            selectedRouteId_ = routeId;
            confirmButton_->setEnabled(true);
        });

        list_->addWidget(card);
    }
    list_->addStretch();
}

void RoutePickerDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

SeatPickerDialog::SeatPickerDialog(RouteController& routes, SeatController& seats,
                                   const QString& routeId, const QString& viewerId,
                                   QWidget* parent)
    : QDialog(parent), routes_(routes), seats_(seats), routeId_(routeId), viewerId_(viewerId) {
    setWindowTitle(QStringLiteral("Select a seat"));
    setModal(true);
    setMinimumSize(560, 600);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(14);

    auto* heading = new QLabel(QStringLiteral("Select a seat"));
    heading->setObjectName("PageTitle");
    info_ = new QLabel;
    info_->setObjectName("PageSubtitle");
    root->addWidget(heading);
    root->addWidget(info_);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* host = new QWidget;
    grid_ = new QGridLayout(host);
    grid_->setContentsMargins(0, 0, 0, 0);
    grid_->setSpacing(10);
    grid_->setAlignment(Qt::AlignTop);
    scroll->setWidget(host);
    root->addWidget(scroll, 1);

    auto* buttons = new QDialogButtonBox;
    auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
    confirmButton_ = buttons->addButton(QStringLiteral("Confirm seat"),
                                        QDialogButtonBox::AcceptRole);
    confirmButton_->setObjectName("PrimaryButton");
    confirmButton_->setEnabled(false);
    root->addWidget(buttons);

    connect(confirmButton_, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    rebuild();
}

void SeatPickerDialog::rebuild() {
    while (QLayoutItem* item = grid_->takeAt(0)) {
        if (QWidget* widget = item->widget()) widget->deleteLater();
        delete item;
    }

    const QVector<SeatView> seats = seats_.seatMap(routeId_, viewerId_);
    info_->setText(QStringLiteral("%1 · %2 seats · %3 free")
                       .arg(routes_.nameOf(routeId_),
                            QString::number(routes_.capacityOf(routeId_)),
                            QString::number(routes_.freeSeatsOf(routeId_))));

    for (int index = 0; index < seats.size(); ++index) {
        const SeatView& view = seats.at(index);

        auto* button = new QPushButton(QString::number(view.number).rightJustified(2, '0'));
        button->setObjectName("SeatButton");
        button->setProperty("seatState", view.state == SeatState::Mine
                                              ? QStringLiteral("mine")
                                              : (view.state == SeatState::Occupied
                                                     ? QStringLiteral("taken")
                                                     : QStringLiteral("free")));
        button->setFixedSize(64, 50);
        button->setToolTip(view.tooltip);
        button->setEnabled(view.state == SeatState::Free);
        button->setCursor(button->isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);

        button->setProperty("seatNumber", view.number);
        connect(button, &QPushButton::clicked, this, &SeatPickerDialog::onSeatClicked);
        grid_->addWidget(button, index / 5, index % 5);
    }

    if (seats.isEmpty()) {
        auto* empty = new QLabel(QStringLiteral("This route has no seats configured yet."));
        empty->setObjectName("Muted");
        grid_->addWidget(empty, 0, 0);
    }
}

void SeatPickerDialog::onSeatClicked() {
    auto* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    selectedSeat_ = button->property("seatNumber").toInt();

    for (QPushButton* candidate : findChildren<QPushButton*>()) {
        if (!candidate->property("seatNumber").isValid()) continue;
        if (candidate->property("seatState").toString() != QStringLiteral("free")) continue;
        const bool selected = candidate->property("seatNumber").toInt() == selectedSeat_;
        candidate->setProperty("seatState", selected ? QStringLiteral("selected")
                                                     : QStringLiteral("free"));
        candidate->style()->unpolish(candidate);
        candidate->style()->polish(candidate);
    }

    info_->setText(QStringLiteral("%1 · seat %2 selected")
                       .arg(routes_.nameOf(routeId_), QString::number(selectedSeat_)));
    confirmButton_->setEnabled(true);
}

}  // namespace gui