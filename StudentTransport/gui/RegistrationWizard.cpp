#include "RegistrationWizard.hpp"

#include "AppContext.hpp"
#include "Pickers.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace gui {
namespace {

constexpr int kSeatColumns = 5;

}  // namespace

RegistrationWizard::RegistrationWizard(Controllers& controllers, bool asStaff, QWidget* parent)
    : QDialog(parent), controllers_(controllers), asStaff_(asStaff) {
    setWindowTitle(asStaff_ ? QStringLiteral("Register as staff")
                             : QStringLiteral("Register as student"));
    setModal(true);
    setMinimumWidth(560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(26, 24, 26, 20);
    root->setSpacing(14);

    auto* heading = new QLabel(asStaff_ ? QStringLiteral("Create a staff account")
                                         : QStringLiteral("Create a student account"));
    heading->setObjectName("PageTitle");
    auto* subtitle = new QLabel(
        asStaff_
            ? QStringLiteral("Staff registration needs the transport office authorization code.")
            : QStringLiteral("Pick a route and a seat as part of signing up."));
    subtitle->setObjectName("PageSubtitle");
    subtitle->setWordWrap(true);

    root->addWidget(heading);
    root->addWidget(subtitle);

    // Every widget updateStepBar() touches is created before it runs.
    steps_ = new QStackedWidget;
    buildSteps();

    error_ = new QLabel;
    error_->setWordWrap(true);
    error_->setStyleSheet("color: " + color::danger().name() + "; font-size: 12px;");
    error_->hide();

    backButton_ = ghostButton(QStringLiteral("Back"));
    nextButton_ = primaryButton(QStringLiteral("Next"));
    finishButton_ = primaryButton(QStringLiteral("Create account"));
    finishButton_->hide();

    updateStepBar();

    root->addWidget(stepBar_);
    root->addWidget(steps_, 1);
    root->addWidget(error_);

    auto* nav = new QHBoxLayout;
    nav->setSpacing(8);
    nav->addWidget(backButton_);
    nav->addStretch();
    nav->addWidget(nextButton_);
    nav->addWidget(finishButton_);
    root->addLayout(nav);

    connect(backButton_, &QPushButton::clicked, this, &RegistrationWizard::goBack);
    connect(nextButton_, &QPushButton::clicked, this, &RegistrationWizard::goNext);
    connect(finishButton_, &QPushButton::clicked, this, &RegistrationWizard::finish);
}

void RegistrationWizard::buildSteps() {
    // ---- Step 1: personal information ----------------------------------------
    auto* personal = new QWidget;
    auto* personalForm = new QFormLayout(personal);
    personalForm->setSpacing(10);

    fullName_ = new QLineEdit;
    fullName_->setPlaceholderText(QStringLiteral("e.g. Ananya Sharma"));
    fatherName_ = new QLineEdit;
    fatherName_->setPlaceholderText(QStringLiteral("Parent or guardian"));
    phone_ = new QLineEdit;
    phone_->setPlaceholderText(QStringLiteral("10 digits"));
    address_ = new QLineEdit;
    address_->setPlaceholderText(QStringLiteral("Room, street, locality"));

    personalForm->addRow(QStringLiteral("Full name"), fullName_);
    personalForm->addRow(QStringLiteral("Father / guardian"), fatherName_);
    personalForm->addRow(QStringLiteral("Phone"), phone_);
    personalForm->addRow(QStringLiteral("Address"), address_);
    steps_->addWidget(personal);

    // ---- Step 2: credentials -------------------------------------------------
    auto* credentials = new QWidget;
    auto* credentialsForm = new QFormLayout(credentials);
    credentialsForm->setSpacing(10);

    username_ = new QLineEdit;
    username_->setPlaceholderText(QStringLiteral("Letters, digits, dot, dash, underscore"));
    password_ = new QLineEdit;
    password_->setEchoMode(QLineEdit::Password);
    password_->setPlaceholderText(
        QStringLiteral("At least 8 characters, with a letter and a digit"));
    confirm_ = new QLineEdit;
    confirm_->setEchoMode(QLineEdit::Password);

    credentialsForm->addRow(QStringLiteral("Username"), username_);
    credentialsForm->addRow(QStringLiteral("Password"), password_);
    credentialsForm->addRow(QStringLiteral("Confirm password"), confirm_);

    if (asStaff_) {
        authorization_ = new QLineEdit;
        authorization_->setEchoMode(QLineEdit::Password);
        authorization_->setPlaceholderText(
            qs(controllers_.context().config().config().staffAuthorizationCode));
        credentialsForm->addRow(QStringLiteral("Authorization code"), authorization_);
    }
    steps_->addWidget(credentials);

    // ---- Step 3: route and seat ---------------------------------------------
    auto* transport = new QWidget;
    auto* transportLayout = new QVBoxLayout(transport);
    transportLayout->setSpacing(10);

    auto* routeCaption = new QLabel(QStringLiteral("Route"));
    routeCaption->setObjectName("FieldLabel");
    routeSummary_ = new QLabel;
    routeSummary_->setObjectName("FieldValue");
    routeSummary_->setWordWrap(true);
    auto* routeRow = new QWidget;
    auto* routeRowLayout = new QHBoxLayout(routeRow);
    routeRowLayout->setContentsMargins(0, 0, 0, 0);
    routeRowLayout->addWidget(routeSummary_);
    routeRowLayout->addStretch();
    auto* changeRoute = ghostButton(QStringLiteral("Choose route"));
    routeRowLayout->addWidget(changeRoute);

    seatSummary_ = new QLabel;
    seatSummary_->setObjectName("FieldValue");
    auto* seatRow = new QWidget;
    auto* seatRowLayout = new QHBoxLayout(seatRow);
    seatRowLayout->setContentsMargins(0, 0, 0, 0);
    seatRowLayout->addWidget(seatSummary_);
    seatRowLayout->addStretch();
    auto* changeSeat = ghostButton(QStringLiteral("Choose seat"));
    changeSeat->setEnabled(false);
    seatRowLayout->addWidget(changeSeat);

    transportLayout->addWidget(routeCaption);
    transportLayout->addWidget(routeRow);
    transportLayout->addWidget(rowField(QStringLiteral("Seat"), QString()));
    transportLayout->addWidget(seatRow);
    transportLayout->addStretch();

    if (!asStaff_) {
        connect(changeRoute, &QPushButton::clicked, this, &RegistrationWizard::chooseRoute);
        connect(changeSeat, &QPushButton::clicked, this, &RegistrationWizard::chooseSeat);
        seatButton_ = changeSeat;
    } else {
        routeSummary_->setText(QStringLiteral("Staff are not allotted a route during sign-up."));
        seatSummary_->setText(QStringLiteral("—"));
        changeRoute->hide();
    }

    steps_->addWidget(transport);

    // ---- Step 4: summary -----------------------------------------------------
    auto* summary = new QWidget;
    auto* summaryLayout = new QVBoxLayout(summary);
    summaryLayout->setSpacing(8);

    summaryName_ = new QLabel;
    summaryUsername_ = new QLabel;
    summaryRoute_ = new QLabel;
    summarySeat_ = new QLabel;
    summaryFees_ = new QLabel;
    for (QLabel* label : {summaryName_, summaryUsername_, summaryRoute_, summarySeat_,
                          summaryFees_}) {
        label->setObjectName("FieldValue");
    }
    summaryFees_->setWordWrap(true);

    auto* summaryGrid = new QGridLayout;
    summaryGrid->setHorizontalSpacing(28);
    summaryGrid->setVerticalSpacing(10);
    summaryGrid->addWidget(rowField(QStringLiteral("Name"), QString()), 0, 0);
    summaryGrid->addWidget(summaryName_, 0, 1);
    summaryGrid->addWidget(rowField(QStringLiteral("Username"), QString()), 1, 0);
    summaryGrid->addWidget(summaryUsername_, 1, 1);
    summaryGrid->addWidget(rowField(QStringLiteral("Route"), QString()), 2, 0);
    summaryGrid->addWidget(summaryRoute_, 2, 1);
    summaryGrid->addWidget(rowField(QStringLiteral("Seat"), QString()), 3, 0);
    summaryGrid->addWidget(summarySeat_, 3, 1);
    summaryGrid->addWidget(rowField(QStringLiteral("Fees"), QString()), 4, 0);
    summaryGrid->addWidget(summaryFees_, 4, 1);
    summaryGrid->setColumnStretch(1, 1);

    summaryLayout->addLayout(summaryGrid);
    summaryLayout->addStretch();
    steps_->addWidget(summary);
}

void RegistrationWizard::updateStepBar() {
    const int step = steps_->currentIndex();

    if (stepLabels_.isEmpty()) {
        const QStringList captions = {QStringLiteral("Personal"), QStringLiteral("Account"),
                                      asStaff_ ? QStringLiteral("Review")
                                                : QStringLiteral("Transport"),
                                      QStringLiteral("Summary")};

        stepBar_ = new QWidget;
        stepBar_->setObjectName("WizardSteps");
        auto* layout = new QHBoxLayout(stepBar_);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(16);
        for (int index = 0; index < captions.size(); ++index) {
            auto* label = new QLabel(QStringLiteral("%1. %2")
                                         .arg(QString::number(index + 1), captions.at(index)));
            label->setObjectName("WizardStep");
            layout->addWidget(label);
            stepLabels_.push_back(label);
        }
        layout->addStretch();
    }

    for (int index = 0; index < stepLabels_.size(); ++index) {
        stepLabels_.at(index)->setProperty("current", index == step ? true : false);
        stepLabels_.at(index)->setProperty("done", index < step ? true : false);
        stepLabels_.at(index)->style()->unpolish(stepLabels_.at(index));
        stepLabels_.at(index)->style()->polish(stepLabels_.at(index));
    }

    backButton_->setEnabled(step > 0);
    const bool last = step == steps_->count() - 1;
    nextButton_->setVisible(!last);
    finishButton_->setVisible(last);
}

bool RegistrationWizard::validateCurrentStep() {
    error_->hide();

    switch (steps_->currentIndex()) {
        case Personal: {
            if (fullName_->text().trimmed().isEmpty()) {
                error_->setText(QStringLiteral("Enter your full name."));
                error_->show();
                return false;
            }
            break;
        }
        case Credentials: {
            if (username_->text().trimmed().isEmpty()) {
                error_->setText(QStringLiteral("Choose a username."));
                error_->show();
                return false;
            }
            if (password_->text() != confirm_->text()) {
                error_->setText(QStringLiteral("The two passwords do not match."));
                error_->show();
                return false;
            }
            if (asStaff_ && authorization_->text().trimmed().isEmpty()) {
                error_->setText(QStringLiteral("An authorization code is required."));
                error_->show();
                return false;
            }
            break;
        }
        case Transport: {
            if (!asStaff_ && selectedRouteId_.isEmpty()) {
                error_->setText(QStringLiteral("Choose a route for your transport."));
                error_->show();
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

void RegistrationWizard::goNext() {
    if (!validateCurrentStep()) return;

    if (steps_->currentIndex() == Credentials && !asStaff_) {
        const RouteOption* chosen = nullptr;
        for (const RouteOption& option : controllers_.routes().options(true)) {
            if (option.id != selectedRouteId_) continue;
            chosen = &option;
            break;
        }
        routeSummary_->setText(chosen ? QStringLiteral("%1 · %2 — %3")
                                            .arg(chosen->id, chosen->name, chosen->endpoints)
                                      : QStringLiteral("No route selected"));
    }

    if (steps_->currentIndex() == Transport) {
        updateSummary();
    }

    steps_->setCurrentIndex(steps_->currentIndex() + 1);
    updateStepBar();
}

void RegistrationWizard::goBack() {
    steps_->setCurrentIndex(steps_->currentIndex() - 1);
    updateStepBar();
}

void RegistrationWizard::chooseRoute() {
    RoutePickerDialog dialog(controllers_.routes(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    selectedRouteId_ = dialog.routeId();
    selectedSeat_ = 0;
    chooseSeat();
}

void RegistrationWizard::chooseSeat() {
    if (selectedRouteId_.isEmpty()) return;

    SeatPickerDialog dialog(controllers_.routes(), controllers_.seats(), selectedRouteId_,
                            QString(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    selectedSeat_ = dialog.seatNumber();
    seatSummary_->setText(selectedSeat_ > 0 ? QStringLiteral("Seat %1").arg(selectedSeat_)
                                             : QStringLiteral("No seat selected"));
    if (seatButton_) seatButton_->setEnabled(selectedSeat_ > 0);
}

void RegistrationWizard::updateSummary() {
    summaryName_->setText(fullName_->text().trimmed());
    summaryUsername_->setText(username_->text().trimmed());

    if (asStaff_) {
        summaryRoute_->setText(QStringLiteral("Not applicable"));
        summarySeat_->setText(QStringLiteral("Not applicable"));
        summaryFees_->setText(QStringLiteral("Staff accounts are not billed transport fees."));
        return;
    }

    const RouteOption* chosen = nullptr;
    for (const RouteOption& option : controllers_.routes().options(false)) {
        if (option.id != selectedRouteId_) continue;
        chosen = &option;
        break;
    }

    summaryRoute_->setText(chosen ? QStringLiteral("%1 · %2").arg(chosen->id, chosen->name)
                                  : QStringLiteral("—"));
    summarySeat_->setText(selectedSeat_ > 0 ? QStringLiteral("Seat %1").arg(selectedSeat_)
                                            : QStringLiteral("No seat selected"));

    const long long registration =
        controllers_.context().config().config().registrationFee;
    const long long transport = chosen ? chosen->fare : 0;
    summaryFees_->setText(QStringLiteral("Registration %1   +   Transport %2   =   Total %3")
                              .arg(money(registration), money(transport),
                                   money(registration + transport)));
}

void RegistrationWizard::finish() {
    st::RegistrationForm form;
    form.fullName = toStd(fullName_->text().trimmed());
    form.fatherName = toStd(fatherName_->text().trimmed());
    form.phone = toStd(phone_->text().trimmed());
    form.address = toStd(address_->text().trimmed());
    form.username = toStd(username_->text().trimmed());
    form.password = toStd(password_->text());

    const ActionResult result =
        asStaff_ ? controllers_.auth().registerStaff(form, authorization_->text().trimmed())
                 : controllers_.auth().registerStudent(form);

    if (!result.ok) {
        error_->setText(result.message);
        error_->show();
        return;
    }

    createdUsername_ = qs(form.username);
    accept();
}

}  // namespace gui