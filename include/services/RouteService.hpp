#pragma once

#include "models/Route.hpp"
#include "models/Seat.hpp"
#include "models/Staff.hpp"
#include "models/Student.hpp"
#include "models/User.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"

#include <optional>
#include <string>
#include <vector>

namespace st {

struct RouteLoad {
    int occupiedCount = 0;
    int seatCapacity = 0;

    int freeSeats() const { return seatCapacity - occupiedCount; }
    std::string occupancyLabel() const;
};

class RouteService {
public:
    RouteService(AuthService& auth, CsvRouteRepository& routeRepo)
        : auth_(auth), routeRepo_(routeRepo) {}

    void load();
    void reload() { load(); }

    std::vector<Route>& cache() { return routeCache_; }
    const std::vector<Route>& cache() const { return routeCache_; }

    const Route* find(const std::string& routeId) const;
    Route* findMutable(const std::string& routeId);
    bool routeExists(const std::string& routeId) const { return find(routeId) != nullptr; }

    RouteLoad loadFor(const std::string& routeId) const;

    // Returns user ids, not Student pointers. A pointer into the user cache
    // would dangle as soon as any save replaced the entry.
    std::vector<std::string> studentIdsOnRoute(const std::string& routeId) const;
    int studentCountOnRoute(const std::string& routeId) const;
    std::vector<int> seatNumbersOnRoute(const std::string& routeId) const;

    bool addRoute(const Route& route, std::string& reason);
    bool updateRoute(const Route& route, std::string& reason);
    bool deleteRoute(const std::string& routeId, std::string& reason);

    std::vector<int> occupiedSeats(const std::string& routeId) const;
    std::vector<int> availableSeats(const std::string& routeId) const;
    bool isSeatFree(const std::string& routeId, int seatNumber) const;
    bool isSeatTakenByOther(const std::string& routeId, int seatNumber,
                            const std::string& requesterId) const;

private:
    AuthService& auth_;
    CsvRouteRepository& routeRepo_;
    std::vector<Route> routeCache_;
};

}  // namespace st