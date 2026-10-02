#pragma once

#include "controllers/RouteController.hpp"
#include "controllers/SeatController.hpp"

#include <QDialog>

class QGridLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

namespace gui {

// Route chooser used by the registration wizard (spec section 13). Presents the
// routes as selectable cards rather than a flat list.
class RoutePickerDialog : public QDialog {
    Q_OBJECT
public:
    RoutePickerDialog(RouteController& routes, QWidget* parent = nullptr);

    QString routeId() const { return selectedRouteId_; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void rebuild();

    RouteController& routes_;
    QString selectedRouteId_;
    QVBoxLayout* list_ = nullptr;
    QPushButton* confirmButton_ = nullptr;
};

// Seat chooser used by the registration wizard and the "change seat" flow.
// Shows a real clickable grid; occupied seats are not selectable.
class SeatPickerDialog : public QDialog {
    Q_OBJECT
public:
    SeatPickerDialog(RouteController& routes, SeatController& seats, const QString& routeId,
                     const QString& viewerId, QWidget* parent = nullptr);

    int seatNumber() const { return selectedSeat_; }

private slots:
    void onSeatClicked();

private:
    void rebuild();

    RouteController& routes_;
    SeatController& seats_;
    QString routeId_;
    QString viewerId_;
    int selectedSeat_ = 0;
    QGridLayout* grid_ = nullptr;
    QLabel* info_ = nullptr;
    QPushButton* confirmButton_ = nullptr;
};

}  // namespace gui