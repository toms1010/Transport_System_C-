#pragma once

#include "models/User.hpp"

#include <memory>

namespace st {

class Staff : public User {
public:
    Staff() { role_ = Role::Staff; }

    Staff(std::string userId, std::string username, std::string fullName)
        : User(std::move(userId), std::move(username), Role::Staff, std::move(fullName)) {}

    const std::string& assignedRouteId() const { return assignedRouteId_; }
    bool hasRoute() const { return !assignedRouteId_.empty(); }

    void setAssignedRouteId(std::string v) { assignedRouteId_ = std::move(v); }

    std::string roleLabel() const override { return "STAFF"; }

    std::unique_ptr<User> clone() const override { return std::make_unique<Staff>(*this); }

private:
    std::string assignedRouteId_;
};

}  // namespace st