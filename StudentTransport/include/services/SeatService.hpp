#pragma once

#include "models/Seat.hpp"
#include "models/Student.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/RouteService.hpp"

#include <string>
#include <vector>

namespace st {

enum class SeatResult {
    Success,
    NoRoute,
    RouteNotFound,
    RouteFull,
    InvalidSeat,
    SeatTaken,
    StorageFailure,
};

const char* seatResultMessage(SeatResult result);

// Seat ownership lives on the Student record (routeId + seatNumber), and this
// service is the ONLY place allowed to change it. Centralising it here is what
// guarantees the invariant "one seat, one student" - the original project wrote
// seat data from several code paths and could hand the same seat to two people.
class SeatService {
public:
    SeatService(RouteService& routes, AuthService& auth, CsvUserRepository& users)
        : routes_(routes), auth_(auth), users_(users) {}

    std::vector<Seat> seatMap(const std::string& routeId, const std::string& viewerId) const;

    SeatResult assign(const std::string& studentId, const std::string& routeId, int seatNumber,
                      std::string& detail);
    SeatResult release(const std::string& studentId, std::string& detail);
    SeatResult changeRoute(const std::string& studentId, const std::string& newRouteId,
                           int newSeatNumber, std::string& detail);
    SeatResult autoAssignFirstFree(const std::string& studentId, const std::string& routeId,
                                   std::string& detail);

private:
    SeatResult applyAssignment(Student& student, const std::string& routeId, int seatNumber,
                               std::string& detail);

    RouteService& routes_;
    AuthService& auth_;
    CsvUserRepository& users_;
};

}  // namespace st