#pragma once

#include <string>
#include <vector>

namespace st {

class Route {
public:
    Route() = default;

    Route(std::string id, std::string name, std::string start, std::string destination,
          std::string via, long long fare, int capacity)
        : routeId_(std::move(id)),
          routeName_(std::move(name)),
          startLocation_(std::move(start)),
          destination_(std::move(destination)),
          via_(std::move(via)),
          // Values are stored verbatim: clamping here would hide bad data from
          // RouteService::addRoute validation.
          fare_(fare),
          capacity_(capacity) {}

    const std::string& routeId() const { return routeId_; }
    const std::string& routeName() const { return routeName_; }
    const std::string& startLocation() const { return startLocation_; }
    const std::string& destination() const { return destination_; }
    const std::string& via() const { return via_; }
    long long fare() const { return fare_; }
    int capacity() const { return capacity_; }
    bool active() const { return active_; }

    void setRouteId(std::string v) { routeId_ = std::move(v); }
    void setRouteName(std::string v) { routeName_ = std::move(v); }
    void setStartLocation(std::string v) { startLocation_ = std::move(v); }
    void setDestination(std::string v) { destination_ = std::move(v); }
    void setVia(std::string v) { via_ = std::move(v); }
    void setFare(long long v) { fare_ = v < 0 ? 0 : v; }
    void setCapacity(int v) { capacity_ = v < 0 ? 0 : v; }
    void setActive(bool v) { active_ = v; }

    std::string endpoints() const;
    std::string label() const;

private:
    std::string routeId_;
    std::string routeName_;
    std::string startLocation_;
    std::string destination_;
    std::string via_;
    long long fare_ = 0;
    int capacity_ = 0;
    bool active_ = true;
};

}  // namespace st