#include "AppContext.hpp"
#include "SettingsPage.hpp"

#include "Notify.hpp"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace gui {

SettingsPage::SettingsPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void SettingsPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Settings"),
        QStringLiteral("Appearance, regional formats and data maintenance."));
    layout_->addWidget(header);

    auto* columns = new QHBoxLayout;
    columns->setSpacing(14);

    auto* appearance = new QGroupBox(QStringLiteral("Appearance"));
    auto* appearanceForm = new QFormLayout(appearance);
    appearanceForm->setSpacing(10);

    themeBox_ = new QComboBox;
    themeBox_->addItem(QStringLiteral("Light"), static_cast<int>(ThemeKind::Light));
    themeBox_->addItem(QStringLiteral("Dark"), static_cast<int>(ThemeKind::Dark));
    themeBox_->setCurrentIndex(themeBox_->findData(static_cast<int>(activeTheme())));

    fontBox_ = new QComboBox;
    fontBox_->addItem(QStringLiteral("Small"));
    fontBox_->addItem(QStringLiteral("Medium"));
    fontBox_->addItem(QStringLiteral("Large"));

    appearanceForm->addRow(QStringLiteral("Theme"), themeBox_);
    appearanceForm->addRow(QStringLiteral("Font size"), fontBox_);
    columns->addWidget(appearance, 1);

    auto* regional = new QGroupBox(QStringLiteral("Regional"));
    auto* regionalForm = new QFormLayout(regional);
    regionalForm->setSpacing(10);

    currencyBox_ = new QComboBox;
    currencyBox_->addItems({QStringLiteral("₹ (INR)"), QStringLiteral("$ (USD)"),
                            QStringLiteral("€ (EUR)"), QStringLiteral("£ (GBP)")});

    dateBox_ = new QComboBox;
    dateBox_->addItems({QStringLiteral("DD-MM-YYYY"), QStringLiteral("MM/DD/YYYY"),
                        QStringLiteral("YYYY-MM-DD")});

    regionalForm->addRow(QStringLiteral("Currency"), currencyBox_);
    regionalForm->addRow(QStringLiteral("Date format"), dateBox_);
    columns->addWidget(regional, 1);
    layout_->addLayout(columns);

    auto* dataCard = new Card;
    auto* dataTitle = new QLabel(QStringLiteral("Data"));
    dataTitle->setObjectName("SectionTitle");
    dataCard->body()->addWidget(dataTitle);

    dataFolder_ = new QLabel(controllers_.context().dataDirectory());
    dataFolder_->setObjectName("Muted");
    dataFolder_->setWordWrap(true);
    dataFolder_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dataCard->body()->addWidget(dataFolder_);

    auto* dataButtons = new QHBoxLayout;
    dataButtons->setSpacing(8);
    auto* backupButton = ghostButton(QStringLiteral("Back up data"), QStringLiteral("download"));
    auto* restoreButton = ghostButton(QStringLiteral("Restore data"), QStringLiteral("upload"));
    for (QPushButton* button : {backupButton, restoreButton}) {
        dataButtons->addWidget(button);
    }
    dataButtons->addStretch();
    dataCard->body()->addLayout(dataButtons);
    layout_->addWidget(dataCard);

    auto* about = new Card;
    auto* aboutTitle = new QLabel(QStringLiteral("About"));
    aboutTitle->setObjectName("SectionTitle");
    version_ = new QLabel(QStringLiteral("Student Transport Management System · version 2.1.0\n"
                                         "%1 · %2")
                              .arg(controllers_.context().institutionName(),
                                   qs(controllers_.context().config().config().departmentName)));
    version_->setObjectName("Muted");
    version_->setWordWrap(true);
    about->body()->addWidget(aboutTitle);
    about->body()->addWidget(version_);
    layout_->addWidget(about);
    layout_->addStretch();

    connect(themeBox_, &QComboBox::currentIndexChanged, this, &SettingsPage::applyAppearance);
    connect(fontBox_, &QComboBox::currentIndexChanged, this, &SettingsPage::applyAppearance);
    connect(backupButton, &QPushButton::clicked, this, &SettingsPage::backupData);
    connect(restoreButton, &QPushButton::clicked, this, &SettingsPage::restoreData);
}

void SettingsPage::refresh() {
    themeBox_->setCurrentIndex(themeBox_->findData(static_cast<int>(activeTheme())));
    fontBox_->setCurrentText(QStringLiteral("Medium"));
    dataFolder_->setText(controllers_.context().dataDirectory());
}

void SettingsPage::applyAppearance() {
    const auto theme = static_cast<ThemeKind>(themeBox_->currentData().toInt());
    if (theme != activeTheme()) {
        setTheme(theme);
        toast(theme == ThemeKind::Dark ? QStringLiteral("Dark theme applied.")
                                        : QStringLiteral("Light theme applied."));
    }
}

void SettingsPage::backupData() {
    const QString target = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Choose a folder for the backup"),
        controllers_.context().dataDirectory());
    if (target.isEmpty()) return;

    BusyOverlay overlay(this, QStringLiteral("Copying data files…"));
    overlay.show();
    QCoreApplication::processEvents();
    const ActionResult result = controllers_.context().backupTo(target);
    overlay.hide();

    toast(result);
}

void SettingsPage::restoreData() {
    const QString source = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Choose a backup folder"),
        controllers_.context().dataDirectory());
    if (source.isEmpty()) return;

    if (!confirm(QStringLiteral("Restore data"),
                 QStringLiteral("Replace every file in %1 with the copy in %2? Sign in again "
                                "afterwards.")
                     .arg(controllers_.context().dataDirectory(), source),
                 QStringLiteral("Restore"))) {
        return;
    }

    BusyOverlay overlay(this, QStringLiteral("Restoring data…"));
    overlay.show();
    QCoreApplication::processEvents();
    const ActionResult result = controllers_.context().restoreFrom(source);
    overlay.hide();

    toast(result);
}

}  // namespace gui
