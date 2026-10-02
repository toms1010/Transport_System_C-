#include "services/SeatService.hpp"

#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>

namespace st {

const char* seatResultMessage(SeatResult result) {
    switch (result) {
        case SeatResult::Success: return "Seat updated.";
        case SeatResult::NoRoute: return "No route selected.";
        case SeatResult::RouteNotFound: return "That route does not exist.";
        case SeatResult::RouteFull: return "This route has no free seats left.";
        case SeatResult::InvalidSeat: return "That seat number is not valid for this route.";
        case SeatResult::SeatTaken: return "That seat is already allotted to another student.";
        case SeatResult::StorageFailure: return "Could not save the change. Please try again.";
        default: return "Seat operation failed.";
    }
}

std::vector<Seat> SeatService::seatMap(const std::string& routeId,
                                       const std::string& viewerId) const {
    std::vector<Seat> seats;
    const Route* route = routes_.find(routeId);
    if (!route) return seats;
    for (int n = 1; n <= route->capacity(); ++n) {
        Seat seat;
        seat.setRouteId(routeId);
        seat.setSeatNumber(n);
        for (int seatNo : routes_.occupiedSeats(routeId)) {
            if (seatNo == n) {
                for (const std::string& id : routes_.studentIdsOnRoute(routeId)) {
                    const Student* s = auth_.findStudent(id);
                    if (s && s->seatNumber() == n) {
                        seat.setStudentId(id);
                        break;
                    }
                }
                break;
            }
        }
        (void)viewerId;
        seats.push_back(seat);
    }
    return seats;
}

SeatResult SeatService::assign(const std::string& studentId, const std::string& routeId,
                               int seatNumber, std::string& detail) {
    Student* student = auth_.findStudent(studentId);
    if (!student) {
        detail = "Student record not found.";
        return SeatResult::StorageFailure;
    }
    return applyAssignment(*student, routeId, seatNumber, detail);
}

SeatResult SeatService::applyAssignment(Student& student, const std::string& routeId,
                                         int seatNumber, std::string& detail) {
    if (util::trim(routeId).empty()) {
        detail = seatResultMessage(SeatResult::NoRoute);
        return SeatResult::NoRoute;
    }
    const Route* route = routes_.find(routeId);
    if (!route) {
        detail = seatResultMessage(SeatResult::RouteNotFound);
        return SeatResult::RouteNotFound;
    }
    if (!validate::seatNumber(seatNumber, route->capacity(), detail)) {
        return SeatResult::InvalidSeat;
    }
    if (routes_.isSeatTakenByOther(routeId, seatNumber, student.userId())) {
        detail = seatResultMessage(SeatResult::SeatTaken);
        return SeatResult::SeatTaken;
    }

    const bool seatChanged = student.routeId() != routeId || student.seatNumber() != seatNumber;
    if (seatChanged) student.setTransportFeeFromFare(route->fare());

    student.setRouteId(routeId);
    student.setSeatNumber(seatNumber);

    if (!users_.update(student, auth_.userCache())) {
        detail = seatResultMessage(SeatResult::StorageFailure);
        return SeatResult::StorageFailure;
    }

    detail = "Seat " + std::to_string(seatNumber) + " allotted on route " + routeId + ".";
    return SeatResult::Success;
}

SeatResult SeatService::release(const std::string& studentId, std::string& detail) {
    Student* student = auth_.findStudent(studentId);
    if (!student) {
        detail = "Student record not found.";
        return SeatResult::StorageFailure;
    }
    student->releaseSeat();
    if (!users_.update(*student, auth_.userCache())) {
        detail = seatResultMessage(SeatResult::StorageFailure);
        return SeatResult::StorageFailure;
    }
    detail = "Seat released.";
    return SeatResult::Success;
}

SeatResult SeatService::changeRoute(const std::string& studentId, const std::string& newRouteId,
                                    int newSeatNumber, std::string& detail) {
    Student* student = auth_.findStudent(studentId);
    if (!student) {
        detail = "Student record not found.";
        return SeatResult::StorageFailure;
    }

    const std::string previousRoute = student->routeId();
    const int previousSeat = student->seatNumber();

    // Release first, then re-assign, so the old seat is immediately reusable and
    // a student switching routes can never collide with their own old record.
    if (!previousRoute.empty() && previousSeat > 0) {
        student->setRouteId("");
        student->setSeatNumber(0);
        if (!users_.update(*student, auth_.userCache())) {
            detail = seatResultMessage(SeatResult::StorageFailure);
            return SeatResult::StorageFailure;
        }
            student = auth_.findStudent(studentId);
        if (!student) {
            detail = seatResultMessage(SeatResult::StorageFailure);
            return SeatResult::StorageFailure;
        }
    }

    const SeatResult outcome = applyAssignment(*student, newRouteId, newSeatNumber, detail);
    if (outcome != SeatResult::Success && !previousRoute.empty() && previousSeat > 0) {
        // Roll the student back to the original seat so a failed switch is atomic
        // from the user's point of view.
        if (Student* restored = auth_.findStudent(studentId)) {
            restored->setRouteId(previousRoute);
            restored->setSeatNumber(previousSeat);
            users_.update(*restored, auth_.userCache());
                }
        detail += " Your previous seat was kept.";
    }
    return outcome;
}

SeatResult SeatService::autoAssignFirstFree(const std::string& studentId,
                                            const std::string& routeId, std::string& detail) {
    const std::vector<int> free = routes_.availableSeats(routeId);
    if (free.empty()) {
        detail = seatResultMessage(SeatResult::RouteFull);
        return SeatResult::RouteFull;
    }
    Student* student = auth_.findStudent(studentId);
    const bool hadSeat = student && student->seatNumber() > 0 && student->routeId() == routeId;
    const SeatResult outcome = hadSeat ? assign(studentId, routeId, student->seatNumber(), detail)
                                       : assign(studentId, routeId, free.front(), detail);
    return outcome;
}

}  // namespace st