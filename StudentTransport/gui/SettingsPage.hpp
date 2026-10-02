#pragma once

#include "Theme.hpp"
#include "Widgets.hpp"

class QComboBox;
class QLabel;
class QPushButton;

namespace gui {

class SettingsPage : public Page {
    Q_OBJECT
public:
    SettingsPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void applyAppearance();
    void backupData();
    void restoreData();

private:
    void build();

    QComboBox* themeBox_ = nullptr;
    QComboBox* fontBox_ = nullptr;
    QComboBox* currencyBox_ = nullptr;
    QComboBox* dateBox_ = nullptr;
    QLabel* dataFolder_ = nullptr;
    QLabel* version_ = nullptr;
};

}  // namespace gui
