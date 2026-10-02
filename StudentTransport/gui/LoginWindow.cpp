#include "LoginWindow.hpp"

#include "AppContext.hpp"
#include "PublicWindow.hpp"
#include "RegistrationWizard.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace gui {

LoginWindow::LoginWindow(Controllers& controllers, QWidget* parent)
    : QDialog(parent), controllers_(controllers) {
    setWindowTitle(QStringLiteral("Sign in — %1 Transport").arg(controllers.context().institutionName()));
    setModal(true);
    setFixedSize(470, 610);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* hero = new QFrame;
    hero->setObjectName("HeroPanel");
    hero->setFixedHeight(200);

    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(34, 30, 34, 26);
    heroLayout->setSpacing(4);

    auto* badge = new QLabel;
    badge->setPixmap(icon(QStringLiteral("transport")).pixmap(44, 44));
    badge->setStyleSheet("color: " + color::sidebarAccent().name() + ";");

    auto* brand = new QLabel(QStringLiteral("Student Transport"));
    brand->setObjectName("HeroTitle");
    auto* sub = new QLabel(QStringLiteral("Management System"));
    sub->setObjectName("HeroSubtitle");
    auto* tagline = new QLabel(qs(controllers_.context().config().config().tagline()));
    tagline->setObjectName("HeroTagline");
    tagline->setWordWrap(true);

    heroLayout->addWidget(badge);
    heroLayout->addWidget(brand);
    heroLayout->addWidget(sub);
    heroLayout->addSpacing(6);
    heroLayout->addWidget(tagline);

    outer->addWidget(hero);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* panel = new QWidget;
    auto* form = new QVBoxLayout(panel);
    form->setContentsMargins(34, 24, 34, 26);
    form->setSpacing(11);

    auto* formTitle = new QLabel(QStringLiteral("Sign in to continue"));
    formTitle->setObjectName("PageTitle");

    username_ = new QLineEdit;
    username_->setPlaceholderText(QStringLiteral("Username"));
    username_->setMinimumHeight(38);

    password_ = new QLineEdit;
    password_->setEchoMode(QLineEdit::Password);
    password_->setPlaceholderText(QStringLiteral("Password"));
    password_->setMinimumHeight(38);

    reveal_ = new QCheckBox(QStringLiteral("Show password"));
    connect(reveal_, &QCheckBox::toggled, this, [this](bool on) {
        password_->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
        password_->setFocus();
    });

    error_ = new QLabel;
    error_->setWordWrap(true);
    error_->setStyleSheet("background: " + color::dangerSoft().name() +
                          "; color: " + color::danger().name() +
                          "; border: 1px solid " + color::danger().name() +
                          "; border-radius: 6px; padding: 8px 10px; font-size: 12px;");
    error_->hide();

    signIn_ = primaryButton(QStringLiteral("Sign in"));
    signIn_->setMinimumHeight(40);
    signIn_->setDefault(true);

    auto* forgot = ghostButton(QStringLiteral("Forgot password?"));

    auto* studentLink = ghostButton(QStringLiteral("Create a student account"));
    studentLink->setObjectName("LinkButton");
    auto* staffLink = ghostButton(QStringLiteral("Register as staff"));
    staffLink->setObjectName("LinkButton");

    auto* links = new QHBoxLayout;
    links->setContentsMargins(0, 2, 0, 0);
    links->addWidget(studentLink);
    links->addStretch();
    links->addWidget(staffLink);

    auto* publicRow = new QHBoxLayout;
    publicRow->setSpacing(6);
    auto* routesButton = ghostButton(QStringLiteral("View routes"));
    auto* noticesButton = ghostButton(QStringLiteral("Notice board"));
    auto* aboutButton = ghostButton(QStringLiteral("About"));
    for (QPushButton* button : {routesButton, noticesButton, aboutButton}) {
        publicRow->addWidget(button);
    }
    publicRow->addStretch();

    auto* contact = new QLabel(QStringLiteral("Trouble signing in? Contact %1, %2.")
                                   .arg(qs(controllers_.context().config().config().contactName),
                                        qs(controllers_.context().config().config().contactPhone)));
    contact->setObjectName("Muted");
    contact->setWordWrap(true);

    form->addWidget(formTitle);
    form->addWidget(username_);
    form->addWidget(password_);
    form->addWidget(reveal_);
    form->addWidget(error_);
    form->addWidget(signIn_);
    form->addWidget(forgot);
    form->addSpacing(6);
    form->addLayout(links);
    form->addSpacing(4);
    form->addLayout(publicRow);
    form->addStretch();
    form->addWidget(contact);

    scroll->setWidget(panel);
    outer->addWidget(scroll, 1);

    connect(signIn_, &QPushButton::clicked, this, &LoginWindow::attemptLogin);
    connect(password_, &QLineEdit::returnPressed, this, &LoginWindow::attemptLogin);
    connect(username_, &QLineEdit::returnPressed, this, [this] { password_->setFocus(); });
    connect(studentLink, &QPushButton::clicked, this, &LoginWindow::registerStudent);
    connect(staffLink, &QPushButton::clicked, this, &LoginWindow::registerStaff);
    connect(forgot, &QPushButton::clicked, this, &LoginWindow::showForgotPassword);
    connect(routesButton, &QPushButton::clicked, this, &LoginWindow::showPublicRoutes);
    connect(noticesButton, &QPushButton::clicked, this, &LoginWindow::showPublicNotices);
    connect(aboutButton, &QPushButton::clicked, this, &LoginWindow::showAbout);

    if (controllers_.context().administratorSeeded()) {
        setError(QStringLiteral("First run: an administrator account was created. Sign in as "
                                "“admin” with the password “admin123”, then change it."));
    }

    username_->setFocus();
}

void LoginWindow::setError(const QString& message) {
    error_->setText(message);
    error_->show();
}

void LoginWindow::attemptLogin() {
    const ActionResult result =
        controllers_.auth().login(username_->text(), password_->text());

    if (!result.ok) {
        setError(result.message);
        password_->clear();
        password_->setFocus();
        return;
    }

    error_->hide();
    accept();
}

void LoginWindow::registerStudent() {
    RegistrationWizard wizard(controllers_, false, this);
    if (wizard.exec() != QDialog::Accepted) return;

    username_->setText(wizard.createdUsername());
    setError(QStringLiteral("Account created. Sign in with your new username and password."));
    password_->clear();
    password_->setFocus();
}

void LoginWindow::registerStaff() {
    RegistrationWizard wizard(controllers_, true, this);
    if (wizard.exec() == QDialog::Accepted) {
        username_->setText(wizard.createdUsername());
        setError(QStringLiteral("Account created. Sign in with your new credentials."));
    }
}

void LoginWindow::showForgotPassword() {
    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(QStringLiteral("Forgot password"));
    box.setText(QStringLiteral("Passwords cannot be reset from this screen."));
    box.setInformativeText(
        QStringLiteral("This build stores only salted password digests, so there is nothing to "
                       "recover.\n\nCall the transport office (%1, %2, %3) and an administrator "
                       "will reset it for you.")
            .arg(qs(controllers_.context().config().config().contactName),
                 qs(controllers_.context().config().config().contactPhone),
                 qs(controllers_.context().config().config().contactHours)));
    box.setTextFormat(Qt::PlainText);
    box.exec();
}

void LoginWindow::showPublicRoutes() {
    PublicWindow window(controllers_, PublicWindow::Tab::Routes, this);
    window.exec();
}

void LoginWindow::showPublicNotices() {
    PublicWindow window(controllers_, PublicWindow::Tab::Notices, this);
    window.exec();
}

void LoginWindow::showAbout() {
    PublicWindow window(controllers_, PublicWindow::Tab::About, this);
    window.exec();
}

}  // namespace gui