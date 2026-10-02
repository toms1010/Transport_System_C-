#pragma once

#include "controllers/SeatController.hpp"
#include "Widgets.hpp"

#include <QStringList>

#include <vector>

class QComboBox;
class QGridLayout;
class QLabel;
class QPushButton;

namespace gui {

class SeatMapPage : public Page {
    Q_OBJECT
public:
    SeatMapPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

    // Used when the roster hands over a student to allot.
    void selectStudent(const QString& userId);

private slots:
    void onRouteChanged();
    void onStudentChanged();
    void claimSelectedSeat();
    void releaseMySeat();
    void assignToStudent();
    void autoAssign();
    void releaseStudentSeat();

private:
    void populateRoutes();
    void populateStudents();
    void rebuildGrid();
    void updateLegend();
    QString currentRouteId() const;
    QString currentStudentId() const;

    QComboBox* routeBox_ = nullptr;
    QComboBox* studentBox_ = nullptr;
    QGridLayout* grid_ = nullptr;
    QWidget* gridHost_ = nullptr;
    QLabel* summary_ = nullptr;
    QLabel* routeInfo_ = nullptr;
    QLabel* studentIdLabel_ = nullptr;
    QStringList studentIds_;
    QPushButton* claimButton_ = nullptr;
    QPushButton* releaseMySeatButton_ = nullptr;
    QPushButton* assignButton_ = nullptr;
    QPushButton* autoAssignButton_ = nullptr;
    QPushButton* releaseStudentButton_ = nullptr;
    QPushButton* changeRouteButton_ = nullptr;

    std::vector<SeatView> seats_;
    int pendingSeatNumber_ = 0;
};

}  // namespace gui