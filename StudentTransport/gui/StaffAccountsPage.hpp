#pragma once

#include "Widgets.hpp"

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace gui {

class StaffAccountsPage : public Page {
    Q_OBJECT
public:
    StaffAccountsPage(Controllers& controllers, const st::Session& session,
                      QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void addStaff();
    void assignRoute();
    void toggleActive();
    void resetPassword();

private:
    void build();
    QString selectedStaffId() const;
    void updateButtons();

    QTableWidget* table_ = nullptr;
    QComboBox* routeBox_ = nullptr;
    QPushButton* assignButton_ = nullptr;
    QPushButton* toggleButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
};

}  // namespace gui
