#include "controllers/SeatController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

#include "models/Route.hpp"
#include "models/Seat.hpp"
#include "models/Student.hpp"

namespace gui {

SeatController::SeatController(AppContext& context) : context_(context) {}

QString SeatController::routeLabel(const QString& routeId) const {
    if (routeId.isEmpty()) return QStringLiteral("no route");
    const st::Route* route = context_.routes().find(toStd(routeId));
    return route ? qs(route->routeName()) : QStringLiteral("that route");
}

QVector<SeatView> SeatController::seatMap(const QString& routeId,
                                          const QString& viewerId) const {
    QVector<SeatView> result;
    if (routeId.isEmpty()) return result;

    const std::string route = toStd(routeId);
    const std::string viewer = toStd(viewerId);

    const std::vector<st::Seat> seats = context_.seats().seatMap(route, viewer);
    result.reserve(static_cast<int>(seats.size()));

    for (const st::Seat& seat : seats) {
        SeatView view;
        view.number = seat.seatNumber();

        if (seat.available()) {
            view.state = SeatState::Free;
            view.tooltip = QStringLiteral("Seat %1 · free").arg(seat.seatNumber());
        } else {
            view.holderName = context_.studentName(seat.studentId());
            const bool mine = !viewer.empty() && seat.studentId() == viewer;
            view.state = mine ? SeatState::Mine : SeatState::Occupied;
            view.tooltip = mine ? QStringLiteral("Seat %1 · your seat").arg(seat.seatNumber())
                                : QStringLiteral("Seat %1 · held by %2")
                                      .arg(seat.seatNumber())
                                      .arg(view.holderName);
        }
        result.push_back(view);
    }
    return result;
}

ActionResult SeatController::claim(const QString& studentId, const QString& routeId,
                                   int seatNumber) {
    std::string detail;
    const st::SeatResult result =
        context_.seats().assign(toStd(studentId), toStd(routeId), seatNumber, detail);
    if (result != st::SeatResult::Success) {
        return ActionResult::failure(QStringLiteral("Could not claim the seat"), qs(detail));
    }
    return ActionResult::success(QStringLiteral("Seat %1 on %2 is now yours.")
                                     .arg(seatNumber)
                                     .arg(routeLabel(routeId)));
}

ActionResult SeatController::release(const QString& studentId) {
    std::string detail;
    const st::SeatResult result = context_.seats().release(toStd(studentId), detail);
    if (result != st::SeatResult::Success) {
        return ActionResult::failure(QStringLiteral("Could not release the seat"), qs(detail));
    }
    return ActionResult::success(QStringLiteral("Your seat has been released."));
}

ActionResult SeatController::assign(const QString& studentId, const QString& routeId,
                                    int seatNumber) {
    std::string detail;
    const st::SeatResult result =
        context_.seats().assign(toStd(studentId), toStd(routeId), seatNumber, detail);
    if (result != st::SeatResult::Success) {
        return ActionResult::failure(QStringLiteral("Could not assign the seat"), qs(detail));
    }
    return ActionResult::success(QStringLiteral("Seat %1 assigned on %2.")
                                     .arg(seatNumber)
                                     .arg(routeLabel(routeId)));
}

ActionResult SeatController::autoAssign(const QString& studentId, const QString& routeId) {
    std::string detail;
    const st::SeatResult result =
        context_.seats().autoAssignFirstFree(toStd(studentId), toStd(routeId), detail);
    if (result != st::SeatResult::Success) {
        return ActionResult::failure(QStringLiteral("No free seat on this route"), qs(detail));
    }
    return ActionResult::success(QStringLiteral("First free seat assigned on %1.")
                                     .arg(routeLabel(routeId)));
}

bool SeatController::isFree(const QString& routeId, int seatNumber) const {
    if (routeId.isEmpty() || seatNumber <= 0) return false;
    return context_.routes().isSeatFree(toStd(routeId), seatNumber);
}

int SeatController::freeSeats(const QString& routeId) const {
    if (routeId.isEmpty()) return 0;
    return context_.routes().loadFor(toStd(routeId)).freeSeats();
}

QString SeatController::routeIdOf(const QString& studentId) const {
    const st::Student* student = context_.auth().findStudent(toStd(studentId));
    return student ? qs(student->routeId()) : QString();
}

int SeatController::seatOf(const QString& studentId) const {
    const st::Student* student = context_.auth().findStudent(toStd(studentId));
    return student ? student->seatNumber() : 0;
}

bool SeatController::hasSeat(const QString& studentId) const {
    const st::Student* student = context_.auth().findStudent(toStd(studentId));
    return student && student->seatAssigned();
}

}  // namespace gui