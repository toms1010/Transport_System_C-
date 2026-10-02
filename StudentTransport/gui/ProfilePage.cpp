#include "ProfilePage.hpp"

#include "AppContext.hpp"
#include "Theme.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace gui {

ProfilePage::ProfilePage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void ProfilePage::build() {
    auto* header = new PageHeader(QStringLiteral("My profile"),
                                  QStringLiteral("Your details, your transport allotment and your "
                                                 "password."));
    layout_->addWidget(header);

    auto* columns = new QHBoxLayout;
    columns->setSpacing(14);

    auto* details = new Card;
    auto* detailsHeader = new QHBoxLayout;
    auto* detailsTitle = new QLabel(QStringLiteral("Account details"));
    detailsTitle->setObjectName("SectionTitle");
    saveButton_ = ghostButton(QStringLiteral("Edit"), QStringLiteral("edit"));
    detailsHeader->addWidget(detailsTitle);
    detailsHeader->addStretch();
    detailsHeader->addWidget(saveButton_);
    details->body()->addLayout(detailsHeader);

    fullName_ = new QLabel;
    username_ = new QLabel;
    role_ = new QLabel;
    father_ = new QLabel;
    phone_ = new QLabel;
    address_ = new QLabel;
    createdAt_ = new QLabel;
    route_ = new QLabel;
    seat_ = new QLabel;
    for (QLabel* label : {fullName_, username_, role_, father_, phone_, address_, createdAt_, route_,
                          seat_}) {
        label->setObjectName("FieldValue");
        label->setWordWrap(true);
    }

    fullNameEdit_ = new QLineEdit;
    fatherEdit_ = new QLineEdit;
    phoneEdit_ = new QLineEdit;
    addressEdit_ = new QLineEdit;
    for (QLineEdit* edit : {fullNameEdit_, fatherEdit_, phoneEdit_, addressEdit_}) {
        edit->hide();
    }

    editMessage_ = new QLabel;
    editMessage_->setWordWrap(true);
    editMessage_->hide();

    const auto readRow = [&](const QString& caption, QLabel* value, QLineEdit* edit) {
        details->body()->addWidget(rowField(caption, QString()));
        auto* row = new QWidget;
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(value);
        rowLayout->addWidget(edit);
        details->body()->addWidget(row);
    };

    readRow(QStringLiteral("Student id"), username_, nullptr);
    readRow(QStringLiteral("Role"), role_, nullptr);
    readRow(QStringLiteral("Full name"), fullName_, fullNameEdit_);
    readRow(QStringLiteral("Father / guardian"), father_, fatherEdit_);
    readRow(QStringLiteral("Phone"), phone_, phoneEdit_);
    readRow(QStringLiteral("Address"), address_, addressEdit_);
    readRow(QStringLiteral("Member since"), createdAt_, nullptr);

    if (session_.role == st::Role::Student) {
        readRow(QStringLiteral("Route"), route_, nullptr);
        readRow(QStringLiteral("Seat"), seat_, nullptr);
    } else if (session_.role == st::Role::Staff) {
        readRow(QStringLiteral("Assigned route"), route_, nullptr);
    }

    details->body()->addWidget(editMessage_);
    details->body()->addStretch();
    columns->addWidget(details, 3);

    auto* security = new Card;
    auto* securityTitle = new QLabel(QStringLiteral("Change password"));
    securityTitle->setObjectName("SectionTitle");
    security->body()->addWidget(securityTitle);

    auto* securityForm = new QFormLayout;
    securityForm->setSpacing(10);

    currentSecret_ = new QLineEdit;
    currentSecret_->setEchoMode(QLineEdit::Password);
    newSecret_ = new QLineEdit;
    newSecret_->setEchoMode(QLineEdit::Password);
    newSecret_->setPlaceholderText(
        QStringLiteral("At least 8 characters, with a letter and a digit"));
    confirmSecret_ = new QLineEdit;
    confirmSecret_->setEchoMode(QLineEdit::Password);

    securityForm->addRow(QStringLiteral("Current password"), currentSecret_);
    securityForm->addRow(QStringLiteral("New password"), newSecret_);
    securityForm->addRow(QStringLiteral("Confirm new password"), confirmSecret_);
    security->body()->addLayout(securityForm);

    passwordMessage_ = new QLabel;
    passwordMessage_->setWordWrap(true);
    passwordMessage_->hide();

    passwordButton_ = primaryButton(QStringLiteral("Update password"));

    security->body()->addWidget(passwordMessage_);
    security->body()->addWidget(passwordButton_);
    security->body()->addStretch();
    columns->addWidget(security, 2);

    layout_->addLayout(columns);

    auto* helpCard = new Card;
    auto* helpTitle = new QLabel(QStringLiteral("Need something corrected?"));
    helpTitle->setObjectName("SectionTitle");
    auto* help = new QLabel(
        QStringLiteral("Route, seat and fee changes are handled by the transport office: %1, %2 "
                       "(%3, %4).")
            .arg(qs(controllers_.context().config().config().contactName),
                 qs(controllers_.context().config().config().contactDesignation))
            .arg(qs(controllers_.context().config().config().contactPhone),
                 qs(controllers_.context().config().config().contactHours)));
    help->setObjectName("Muted");
    help->setWordWrap(true);
    helpCard->body()->addWidget(helpTitle);
    helpCard->body()->addWidget(help);
    layout_->addWidget(helpCard);
    layout_->addStretch();

    connect(saveButton_, &QPushButton::clicked, this, &ProfilePage::toggleEditing);
    connect(passwordButton_, &QPushButton::clicked, this, &ProfilePage::changePassword);
}

void ProfilePage::refresh() {
    const st::User* user = controllers_.context().auth().findUser(session_.userId);
    if (!user) return;

    username_->setText(qs(user->userId()));
    role_->setText(qs(user->roleLabel()));
    createdAt_->setText(qs(user->createdAt()));

    fullName_->setText(qs(user->fullName()));
    father_->setText(qs(user->fatherName()));
    phone_->setText(qs(user->phone()));
    address_->setText(qs(user->address()));

    fullNameEdit_->setText(qs(user->fullName()));
    fatherEdit_->setText(qs(user->fatherName()));
    phoneEdit_->setText(qs(user->phone()));
    addressEdit_->setText(qs(user->address()));

    StudentRow student;
    if (controllers_.students().findRow(qs(session_.userId), student)) {
        route_->setText(student.routeName);
        seat_->setText(student.seatLabel);
        return;
    }

    StaffRow staff;
    if (controllers_.staff().findRow(qs(session_.userId), staff)) {
        route_->setText(staff.routeName);
        seat_->hide();
    }
}

void ProfilePage::toggleEditing() {
    const bool editing = fullNameEdit_->isVisible();
    for (QLineEdit* edit : {fullNameEdit_, fatherEdit_, phoneEdit_, addressEdit_}) {
        edit->setVisible(!editing);
    }
    for (QLabel* label : {fullName_, father_, phone_, address_}) {
        label->setVisible(editing);
    }
    saveButton_->setText(editing ? QStringLiteral("Save") : QStringLiteral("Edit"));

    if (!editing) {
        saveDetails();
        return;
    }
    fullNameEdit_->setFocus();
}

void ProfilePage::saveDetails() {
    const ActionResult result = controllers_.auth().updateOwnProfile(
        qs(session_.userId), fullNameEdit_->text(), fatherEdit_->text(), phoneEdit_->text(),
        addressEdit_->text());

    if (!result.ok) {
        editMessage_->setText(result.message);
        editMessage_->setStyleSheet("color: " + color::danger().name() + "; font-size: 12px;");
        editMessage_->show();
        return;
    }

    editMessage_->setText(result.message);
    editMessage_->setStyleSheet("color: " + color::success().name() + "; font-size: 12px;");
    editMessage_->show();

    for (QLineEdit* edit : {fullNameEdit_, fatherEdit_, phoneEdit_, addressEdit_}) {
        edit->hide();
    }
    for (QLabel* label : {fullName_, father_, phone_, address_}) {
        label->setVisible(true);
    }
    saveButton_->setText(QStringLiteral("Edit"));

    refresh();
    toast(result);
}

void ProfilePage::changePassword() {
    const ActionResult result = controllers_.auth().changePassword(
        qs(session_.userId), currentSecret_->text(), newSecret_->text(), confirmSecret_->text());

    passwordMessage_->setText(result.message);
    passwordMessage_->setStyleSheet(
        (result.ok ? QStringLiteral("color: ") + color::success().name()
                   : QStringLiteral("color: ") + color::danger().name()) +
        QStringLiteral("; font-size: 12px;"));
    passwordMessage_->show();

    if (!result.ok) return;

    currentSecret_->clear();
    newSecret_->clear();
    confirmSecret_->clear();
    toast(result);
}

}  // namespace gui
