#pragma once

#include "controllers/StudentController.hpp"
#include "Widgets.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace gui {

class RosterPage : public Page {
    Q_OBJECT
public:
    RosterPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void applyFilters();
    void clearFilters();
    void openSeatMapFor();
    void openPaymentsFor();

private:
    void build();
    void rebuildFilters();
    QString selectedStudentId() const;

    QTableWidget* table_ = nullptr;
    QLineEdit* search_ = nullptr;
    QComboBox* routeFilter_ = nullptr;
    QComboBox* duesFilter_ = nullptr;
    QLabel* summary_ = nullptr;
};

}  // namespace gui
