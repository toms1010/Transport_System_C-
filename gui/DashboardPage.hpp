#pragma once

#include "Widgets.hpp"

#include <QHash>

class QGridLayout;
class QLabel;
class QProgressBar;
class QTableWidget;

namespace gui {

class DashboardPage : public Page {
    Q_OBJECT
public:
    DashboardPage(Controllers& controllers, const st::Session& session,
                  QWidget* parent = nullptr);

    void refresh() override;

private:
    void build();
    void buildStudentView();
    void buildStaffView();
    void refreshStudent();
    void refreshStaff();

    StatCard* addStat(QGridLayout* grid, int row, int column, const QString& key,
                      const QString& label);
    StatCard* stat(const QString& key) const { return stats_.value(key, nullptr); }

    // Student view widgets.
    QLabel* routeName_ = nullptr;
    QLabel* routeVia_ = nullptr;
    QLabel* seatLabel_ = nullptr;
    QLabel* paymentLabel_ = nullptr;
    QLabel* paymentCaption_ = nullptr;
    QProgressBar* feeBar_ = nullptr;
    QTableWidget* notices_ = nullptr;
    QTableWidget* complaints_ = nullptr;
    QPushButton* viewNotices_ = nullptr;
    QPushButton* viewComplaints_ = nullptr;

    // Staff view widgets.
    QTableWidget* routes_ = nullptr;
    QTableWidget* activity_ = nullptr;

    QHash<QString, StatCard*> stats_;
};

}  // namespace gui