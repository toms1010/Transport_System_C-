#include "AppContext.hpp"
#include "PublicWindow.hpp"

#include "Theme.hpp"
#include "Widgets.hpp"
#include "utils/TextUtils.hpp"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {

PublicWindow::PublicWindow(Controllers& controllers, Tab initial, QWidget* parent)
    : QDialog(parent), controllers_(controllers) {
    setWindowTitle(QStringLiteral("%1 Transport").arg(controllers_.context().institutionName()));
    setModal(true);
    setMinimumSize(760, 560);

    build();

    switch (initial) {
        case Tab::Notices:
            tabs_->setCurrentIndex(1);
            break;
        case Tab::About:
            tabs_->setCurrentIndex(2);
            break;
        case Tab::Routes:
        default:
            tabs_->setCurrentIndex(0);
            break;
    }
}

void PublicWindow::build() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(14);

    auto* heading = new QLabel(controllers_.context().institutionName());
    heading->setObjectName("PageTitle");
    auto* subtitle = new QLabel(qs(controllers_.context().config().config().tagline()));
    subtitle->setObjectName("PageSubtitle");

    root->addWidget(heading);
    root->addWidget(subtitle);

    tabs_ = new QTabWidget;
    buildRoutesTab();
    buildNoticesTab();
    buildAboutTab();
    root->addWidget(tabs_, 1);

    auto* buttons = new QDialogButtonBox;
    auto* close = buttons->addButton(QStringLiteral("Close"), QDialogButtonBox::RejectRole);
    close->setObjectName("PrimaryButton");
    root->addWidget(buttons);

    connect(close, &QPushButton::clicked, this, &QDialog::reject);
}

void PublicWindow::buildRoutesTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* caption = new QLabel(
        QStringLiteral("Routes and fares currently offered by the transport cell."));
    caption->setObjectName("Muted");
    layout->addWidget(caption);

    routes_ = new QTableWidget;
    routes_->setColumnCount(5);
    routes_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Route"),
                                         QStringLiteral("Endpoints"), QStringLiteral("Fare"),
                                         QStringLiteral("Free seats")});
    routes_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    routes_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    routes_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    routes_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    routes_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    configureTable(routes_);
    layout->addWidget(routes_);

    int row = 0;
    for (const RouteRow& route : controllers_.routes().rows()) {
        if (!route.active) continue;
        routes_->insertRow(row);
        routes_->setItem(row, 0, textItem(route.id));
        routes_->setItem(row, 1, textItem(route.name));
        routes_->setItem(row, 2, textItem(route.endpoints));
        routes_->setItem(row, 3, moneyItem(route.fare));
        routes_->setItem(row, 4, centeredItem(QString::number(route.free)));
        ++row;
    }
    showEmptyState(routes_, QStringLiteral("No routes have been published yet."));

    tabs_->addTab(page, QStringLiteral("Routes"));
}

void PublicWindow::buildNoticesTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* caption = new QLabel(QStringLiteral("Announcements from the transport office."));
    caption->setObjectName("Muted");
    layout->addWidget(caption);

    notices_ = new QTableWidget;
    notices_->setColumnCount(3);
    notices_->setHorizontalHeaderLabels({QStringLiteral("Notice"), QStringLiteral("Posted"),
                                         QStringLiteral("Details")});
    notices_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    notices_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    notices_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    configureTable(notices_);
    layout->addWidget(notices_);

    int row = 0;
    for (const NoticeRow& notice : controllers_.notices().rows()) {
        notices_->insertRow(row);
        notices_->setItem(row, 0, textItem(notice.title));
        notices_->setItem(row, 1, textItem(notice.createdAt));
        auto* excerpt = textItem(notice.excerpt);
        excerpt->setToolTip(notice.content);
        notices_->setItem(row, 2, excerpt);
        ++row;
    }
    showEmptyState(notices_, QStringLiteral("Nothing has been published yet."));

    tabs_->addTab(page, QStringLiteral("Notice board"));
}

void PublicWindow::buildAboutTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    const st::SystemConfig& config = controllers_.context().config().config();

    auto* card = new Card;
    card->body()->addWidget(rowField(QStringLiteral("Institution"), qs(config.institutionName)));
    card->body()->addWidget(rowField(QStringLiteral("Office"), qs(config.departmentName)));
    card->body()->addWidget(rowField(QStringLiteral("Location"), qs(config.institutionCity)));
    card->body()->addWidget(rowField(QStringLiteral("Version"),
                                     QStringLiteral("2.1.0 · C++17 / Qt 6")));
    card->body()->addWidget(
        rowField(QStringLiteral("Registration fee"), money(config.registrationFee)));
    layout->addWidget(card);

    auto* contactCard = new Card;
    auto* contactTitle = new QLabel(QStringLiteral("Contact"));
    contactTitle->setObjectName("SectionTitle");
    contactCard->body()->addWidget(contactTitle);
    contactCard->body()->addWidget(rowField(QStringLiteral("In-charge"), qs(config.contactName)));
    contactCard->body()->addWidget(rowField(QStringLiteral("Designation"),
                                            qs(config.contactDesignation)));
    contactCard->body()->addWidget(rowField(QStringLiteral("Phone"), qs(config.contactPhone)));
    contactCard->body()->addWidget(rowField(QStringLiteral("Room"), qs(config.contactRoom)));
    contactCard->body()->addWidget(rowField(QStringLiteral("Hours"), qs(config.contactHours)));
    contactCard->body()->addStretch();
    layout->addWidget(contactCard);
    layout->addStretch();

    tabs_->addTab(page, QStringLiteral("About"));
}

}  // namespace gui