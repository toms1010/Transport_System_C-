#include "StaffAccountsPage.hpp"

#include "AppContext.hpp"
#include "Theme.hpp"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {
namespace {

class StaffAccountDialog : public QDialog {
public:
    StaffAccountDialog(QWidget* parent, const QString& authorizationCode)
        : QDialog(parent) {
        setWindowTitle(QStringLiteral("New staff account"));
        setModal(true);
        setMinimumWidth(480);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(24, 22, 24, 18);
        root->setSpacing(12);

        auto* heading = new QLabel(QStringLiteral("Create a staff account"));
        heading->setObjectName("PageTitle");
        auto* subtitle = new QLabel(
            QStringLiteral("Staff registration needs the transport office authorization code."));
        subtitle->setObjectName("PageSubtitle");
        subtitle->setWordWrap(true);

        auto* form = new QFormLayout;
        form->setSpacing(9);

        fullName_ = new QLineEdit;
        fatherName_ = new QLineEdit;
        phone_ = new QLineEdit;
        address_ = new QLineEdit;
        username_ = new QLineEdit;
        username_->setPlaceholderText(QStringLiteral("e.g. transport.staff"));
        password_ = new QLineEdit;
        password_->setEchoMode(QLineEdit::Password);
        code_ = new QLineEdit;
        code_->setEchoMode(QLineEdit::Password);
        code_->setPlaceholderText(authorizationCode);

        form->addRow(QStringLiteral("Full name"), fullName_);
        form->addRow(QStringLiteral("Father / guardian"), fatherName_);
        form->addRow(QStringLiteral("Phone"), phone_);
        form->addRow(QStringLiteral("Address"), address_);
        form->addRow(QStringLiteral("Username"), username_);
        form->addRow(QStringLiteral("Password"), password_);
        form->addRow(QStringLiteral("Authorization code"), code_);

        auto* buttons = new QDialogButtonBox;
        auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
        auto* create = buttons->addButton(QStringLiteral("Create account"),
                                          QDialogButtonBox::AcceptRole);
        create->setObjectName("PrimaryButton");

        root->addWidget(heading);
        root->addWidget(subtitle);
        root->addLayout(form);
        root->addWidget(buttons);

        connect(create, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    st::RegistrationForm form() const {
        st::RegistrationForm result;
        result.fullName = toStd(fullName_->text().trimmed());
        result.fatherName = toStd(fatherName_->text().trimmed());
        result.phone = toStd(phone_->text().trimmed());
        result.address = toStd(address_->text().trimmed());
        result.username = toStd(username_->text().trimmed());
        result.password = toStd(password_->text());
        return result;
    }

    QString code() const { return code_->text().trimmed(); }

private:
    QLineEdit* fullName_ = nullptr;
    QLineEdit* fatherName_ = nullptr;
    QLineEdit* phone_ = nullptr;
    QLineEdit* address_ = nullptr;
    QLineEdit* username_ = nullptr;
    QLineEdit* password_ = nullptr;
    QLineEdit* code_ = nullptr;
};

class PasswordDialog : public QDialog {
public:
    explicit PasswordDialog(const QString& who, QWidget* parent) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Reset password"));
        setModal(true);
        setMinimumWidth(420);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(24, 22, 24, 18);
        root->setSpacing(12);

        auto* heading = new QLabel(QStringLiteral("Reset password for %1").arg(who));
        heading->setObjectName("PageTitle");
        auto* subtitle = new QLabel(
            QStringLiteral("Share the new password over a channel you trust."));
        subtitle->setObjectName("PageSubtitle");
        subtitle->setWordWrap(true);

        field_ = new QLineEdit;
        field_->setEchoMode(QLineEdit::Password);
        field_->setPlaceholderText(QStringLiteral("New password"));

        auto* buttons = new QDialogButtonBox;
        auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
        auto* save = buttons->addButton(QStringLiteral("Reset password"),
                                        QDialogButtonBox::AcceptRole);
        save->setObjectName("PrimaryButton");

        root->addWidget(heading);
        root->addWidget(subtitle);
        root->addWidget(field_);
        root->addWidget(buttons);

        connect(save, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    QString secret() const { return field_->text(); }

private:
    QLineEdit* field_ = nullptr;
};

}  // namespace

StaffAccountsPage::StaffAccountsPage(Controllers& controllers, const st::Session& session,
                                     QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void StaffAccountsPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Staff management"),
        QStringLiteral("Create staff accounts, assign them a route and control who can sign in."));
    auto* addButton = primaryButton(QStringLiteral("New staff account"),
                                    QStringLiteral("add"));
    header->addAction(addButton);
    layout_->addWidget(header);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(8);

    auto* routeCaption = new QLabel(QStringLiteral("Assign route"));
    routeCaption->setObjectName("FieldLabel");
    routeBox_ = new QComboBox;
    routeBox_->setMinimumWidth(220);
    routeBox_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    assignButton_ = ghostButton(QStringLiteral("Assign"));
    toggleButton_ = ghostButton(QStringLiteral("Activate / deactivate"),
                                QStringLiteral("refresh"));
    resetButton_ = ghostButton(QStringLiteral("Reset password"), QStringLiteral("lock"));

    actions->addWidget(routeCaption);
    actions->addWidget(routeBox_);
    actions->addWidget(assignButton_);
    actions->addWidget(toggleButton_);
    actions->addWidget(resetButton_);
    actions->addStretch();
    layout_->addLayout(actions);

    auto* card = new Card;
    table_ = new QTableWidget;
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Name"),
                                       QStringLiteral("Username"), QStringLiteral("Contact"),
                                       QStringLiteral("Route"), QStringLiteral("Status")});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    configureTable(table_);
    table_->setMinimumHeight(320);
    card->body()->addWidget(table_);
    layout_->addWidget(card);
    layout_->addStretch();

    connect(addButton, &QPushButton::clicked, this, &StaffAccountsPage::addStaff);
    connect(assignButton_, &QPushButton::clicked, this, &StaffAccountsPage::assignRoute);
    connect(toggleButton_, &QPushButton::clicked, this, &StaffAccountsPage::toggleActive);
    connect(resetButton_, &QPushButton::clicked, this, &StaffAccountsPage::resetPassword);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &StaffAccountsPage::updateButtons);
}

QString StaffAccountsPage::selectedStaffId() const {
    const int row = table_->currentRow();
    if (row < 0) return {};
    QTableWidgetItem* item = table_->item(row, 0);
    return item ? item->text() : QString();
}

void StaffAccountsPage::updateButtons() {
    const QString id = selectedStaffId();
    const bool haveSelection = !id.isEmpty();

    toggleButton_->setEnabled(haveSelection);
    resetButton_->setEnabled(haveSelection);

    routeBox_->blockSignals(true);
    routeBox_->clear();
    routeBox_->addItem(QStringLiteral("No route"), QString());
    for (const RouteOption& route : controllers_.routes().options(false)) {
        routeBox_->addItem(QStringLiteral("%1 · %2").arg(route.id, route.name), route.id);
    }

    StaffRow staff;
    if (controllers_.staff().findRow(id, staff) && !staff.administrator) {
        const int index = routeBox_->findData(staff.routeId);
        if (index >= 0) routeBox_->setCurrentIndex(index);
    }
    routeBox_->blockSignals(false);

    assignButton_->setEnabled(haveSelection && !staff.administrator);
}

void StaffAccountsPage::refresh() {
    table_->setRowCount(0);
    int row = 0;
    for (const StaffRow& staff : controllers_.staff().rows()) {
        table_->insertRow(row);
        table_->setItem(row, 0, textItem(staff.id));
        table_->setItem(row, 1, textItem(staff.name));
        table_->setItem(row, 2, textItem(staff.username));
        table_->setItem(row, 3, textItem(staff.phone));
        table_->setItem(row, 4, textItem(staff.routeName));
        table_->setItem(
            row, 5,
            statusItem(staff.administrator
                           ? QStringLiteral("Administrator")
                           : (staff.active ? QStringLiteral("Active")
                                           : QStringLiteral("Deactivated")),
                       staff.administrator ? QStringLiteral("success")
                                           : (staff.active ? QStringLiteral("success")
                                                           : QStringLiteral("danger"))));
        ++row;
    }
    showEmptyState(table_, QStringLiteral("No staff accounts yet."));
    updateButtons();
}

void StaffAccountsPage::addStaff() {
    StaffAccountDialog dialog(this, qs(controllers_.context().config().config()
                                              .staffAuthorizationCode));
    if (dialog.exec() != QDialog::Accepted) return;
    toast(controllers_.staff().create(dialog.form(), dialog.code()));
    refresh();
}

void StaffAccountsPage::assignRoute() {
    const QString id = selectedStaffId();
    if (id.isEmpty()) return;
    toast(controllers_.staff().assignRoute(id, routeBox_->currentData().toString()));
    refresh();
}

void StaffAccountsPage::toggleActive() {
    const QString id = selectedStaffId();
    if (id.isEmpty()) return;

    StaffRow staff;
    controllers_.staff().findRow(id, staff);

    if (staff.administrator) {
        warn(QStringLiteral("Not allowed"),
             QStringLiteral("Administrator accounts cannot be deactivated from here."));
        return;
    }
    if (!confirm(staff.active ? QStringLiteral("Deactivate account")
                              : QStringLiteral("Reactivate account"),
                 QStringLiteral("%1 %2? %3")
                     .arg(staff.active ? QStringLiteral("Deactivate")
                                       : QStringLiteral("Reactivate"),
                          staff.name,
                          staff.active ? QStringLiteral("They will not be able to sign in until "
                                                       "they are reactivated.")
                                       : QString()))) {
        return;
    }

    toast(controllers_.staff().setActive(id, !staff.active));
    refresh();
}

void StaffAccountsPage::resetPassword() {
    const QString id = selectedStaffId();
    if (id.isEmpty()) return;

    StaffRow staff;
    controllers_.staff().findRow(id, staff);

    PasswordDialog dialog(staff.name.isEmpty() ? staff.username : staff.name, this);
    if (dialog.exec() != QDialog::Accepted) return;

    toast(controllers_.staff().resetPassword(id, dialog.secret()));
    refresh();
}

}  // namespace gui
