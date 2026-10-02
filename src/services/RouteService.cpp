#include "services/RouteService.hpp"

#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>

namespace st {

std::string RouteLoad::occupancyLabel() const {
    return std::to_string(occupiedCount) + "/" + std::to_string(seatCapacity);
}

void RouteService::load() {
    routeCache_ = routeRepo_.loadAll();
    routeRepo_.seedDefaults(routeCache_);
}

const Route* RouteService::find(const std::string& routeId) const {
    for (const Route& r : routeCache_) {
        if (r.routeId() == routeId) return &r;
    }
    return nullptr;
}

Route* RouteService::findMutable(const std::string& routeId) {
    for (Route& r : routeCache_) {
        if (r.routeId() == routeId) return &r;
    }
    return nullptr;
}

RouteLoad RouteService::loadFor(const std::string& routeId) const {
    RouteLoad load;
    const Route* route = find(routeId);
    if (!route) return load;
    load.seatCapacity = route->capacity();
    load.occupiedCount = studentCountOnRoute(routeId);
    return load;
}

std::vector<std::string> RouteService::studentIdsOnRoute(const std::string& routeId) const {
    std::vector<std::string> out;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (!s) continue;
        if (s->routeId() == routeId && s->seatNumber() > 0) out.push_back(s->userId());
    }
    return out;
}

int RouteService::studentCountOnRoute(const std::string& routeId) const {
    return static_cast<int>(studentIdsOnRoute(routeId).size());
}

std::vector<int> RouteService::seatNumbersOnRoute(const std::string& routeId) const {
    std::vector<int> seats;
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (!s) continue;
        if (s->routeId() == routeId && s->seatNumber() > 0) seats.push_back(s->seatNumber());
    }
    std::sort(seats.begin(), seats.end());
    return seats;
}

bool RouteService::addRoute(const Route& route, std::string& reason) {
    if (!validate::routeId(route.routeId(), reason)) return false;
    if (util::trim(route.routeName()).empty()) {
        reason = "Route name is required.";
        return false;
    }
    if (!validate::fare(route.fare(), reason)) return false;
    if (!validate::capacity(route.capacity(), reason)) return false;
    if (routeExists(route.routeId())) {
        reason = "A route with id " + route.routeId() + " already exists.";
        return false;
    }
    routeRepo_.save(route, routeCache_);
    return true;
}

bool RouteService::updateRoute(const Route& route, std::string& reason) {
    if (!validate::routeId(route.routeId(), reason)) return false;
    if (!validate::fare(route.fare(), reason)) return false;
    if (!validate::capacity(route.capacity(), reason)) return false;

    const Route* existing = find(route.routeId());
    if (!existing) {
        reason = "Route " + route.routeId() + " does not exist.";
        return false;
    }

    if (route.capacity() < existing->capacity()) {
        const int occupied = studentCountOnRoute(route.routeId());
        if (route.capacity() < occupied) {
            reason = "Capacity cannot drop below the " + std::to_string(occupied) +
                     " students already allotted to this route.";
            return false;
        }
    }

    routeRepo_.update(route, routeCache_);
    return true;
}

bool RouteService::deleteRoute(const std::string& routeId, std::string& reason) {
    if (!routeExists(routeId)) {
        reason = "Route " + routeId + " does not exist.";
        return false;
    }
    if (!studentIdsOnRoute(routeId).empty()) {
        reason = "Reassign the students on this route before deleting it.";
        return false;
    }
    routeRepo_.removeById(routeId, routeCache_);
    return true;
}

std::vector<int> RouteService::occupiedSeats(const std::string& routeId) const {
    return seatNumbersOnRoute(routeId);
}

std::vector<int> RouteService::availableSeats(const std::string& routeId) const {
    std::vector<int> free;
    const Route* route = find(routeId);
    if (!route) return free;
    const std::vector<int> taken = occupiedSeats(routeId);
    for (int seat = 1; seat <= route->capacity(); ++seat) {
        if (std::find(taken.begin(), taken.end(), seat) == taken.end()) free.push_back(seat);
    }
    return free;
}

bool RouteService::isSeatFree(const std::string& routeId, int seatNumber) const {
    const std::vector<int> taken = occupiedSeats(routeId);
    return std::find(taken.begin(), taken.end(), seatNumber) == taken.end();
}

bool RouteService::isSeatTakenByOther(const std::string& routeId, int seatNumber,
                                      const std::string& requesterId) const {
    for (const auto& user : auth_.userCache()) {
        const auto* s = dynamic_cast<const Student*>(user.get());
        if (!s) continue;
        if (s->routeId() != routeId || s->seatNumber() != seatNumber) continue;
        if (s->userId() != requesterId) return true;
    }
    return false;
}

}  // namespace st