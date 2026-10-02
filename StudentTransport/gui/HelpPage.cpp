#include "AppContext.hpp"
#include "HelpPage.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace gui {

HelpPage::HelpPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void HelpPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Help & contact"),
        QStringLiteral("How the main tasks work, and who to call about anything else."));
    layout_->addWidget(header);

    const struct {
        const char* title;
        const char* body;
    } topics[] = {
        {"Choosing a route",
         "Open My transport, pick a route from the drop-down, then click any green seat and "
         "confirm. Your transport fee starts as soon as you hold a seat."},
        {"Changing route or seat",
         "On the seat map, select a different route and claim a new seat. Your old seat is "
         "released automatically so someone else can take it."},
        {"Paying fees",
         "Registration and transport are tracked separately. Choose which fee a payment settles, "
         "enter an amount and a reference, then confirm. Receipts are kept in your history."},
        {"Raising a complaint",
         "Use My complaints to submit a new complaint with a subject and details. You will see it "
         "move from Open to Resolved, along with the staff response."},
        {"Reading notices",
         "The notice board carries announcements from the transport office. Notices appear on "
         "your dashboard as soon as they are published."},
        {"Changing your password",
         "Open My profile, fill in your current password and a new one. Passwords are never "
         "stored in readable form."},
    };

    auto* grid = new QGridLayout;
    grid->setSpacing(14);

    int column = 0;
    for (const auto& topic : topics) {
        auto* card = new Card;

        auto* title = new QLabel(qs(topic.title));
        title->setObjectName("SectionTitle");
        auto* body = new QLabel(qs(topic.body));
        body->setObjectName("Muted");
        body->setWordWrap(true);

        card->body()->addWidget(title);
        card->body()->addWidget(body);
        card->body()->addStretch();

        grid->addWidget(card, column / 2, column % 2);
        ++column;
    }
    layout_->addLayout(grid);

    auto* contact = new Card;
    auto* contactTitle = new QLabel(QStringLiteral("Transport office"));
    contactTitle->setObjectName("SectionTitle");
    contact->body()->addWidget(contactTitle);

    const st::SystemConfig& config = controllers_.context().config().config();
    contact->body()->addWidget(rowField(QStringLiteral("Office"), qs(config.contactDesignation)));
    contact->body()->addWidget(rowField(QStringLiteral("Contact"), qs(config.contactName)));
    contact->body()->addWidget(rowField(QStringLiteral("Phone"), qs(config.contactPhone)));
    contact->body()->addWidget(rowField(QStringLiteral("Room"), qs(config.contactRoom)));
    contact->body()->addWidget(rowField(QStringLiteral("Hours"), qs(config.contactHours)));
    contact->body()->addStretch();
    layout_->addWidget(contact);

    layout_->addStretch();
}

void HelpPage::refresh() {}

}  // namespace gui
