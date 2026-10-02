#pragma once

#include "Widgets.hpp"

class QLabel;
class QProgressBar;
class QPushButton;
class QTableWidget;

namespace gui {

class ReportsPage : public Page {
    Q_OBJECT
public:
    ReportsPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void exportLedger();

private:
    void build();

    QLabel* occupancySummary_ = nullptr;
    QLabel* financialSummary_ = nullptr;
    QLabel* complaintSummary_ = nullptr;
    QTableWidget* routeTable_ = nullptr;
    QTableWidget* studentTable_ = nullptr;
};

}  // namespace gui
