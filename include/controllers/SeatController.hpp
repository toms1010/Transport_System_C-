#pragma once

#include "controllers/ActionResult.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

enum class SeatState { Free, Occupied, Mine, Selected };

struct SeatView {
    int number = 0;
    SeatState state = SeatState::Free;
    QString holderName;
    QString tooltip;
};

class SeatController {
public:
    explicit SeatController(AppContext& context);

    // viewerId decides which seat renders as "mine".
    QVector<SeatView> seatMap(const QString& routeId, const QString& viewerId) const;

    ActionResult claim(const QString& studentId, const QString& routeId, int seatNumber);
    ActionResult release(const QString& studentId);
    ActionResult assign(const QString& studentId, const QString& routeId, int seatNumber);
    ActionResult autoAssign(const QString& studentId, const QString& routeId);

    bool isFree(const QString& routeId, int seatNumber) const;
    int freeSeats(const QString& routeId) const;

    QString routeIdOf(const QString& studentId) const;
    int seatOf(const QString& studentId) const;
    bool hasSeat(const QString& studentId) const;

private:
    QString routeLabel(const QString& routeId) const;

    AppContext& context_;
};

}  // namespace gui