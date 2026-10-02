#pragma once

#include "models/User.hpp"

#include <memory>

namespace st {

// Administrator: manages students, staff, routes, buses, payments and settings.
// Kept as a distinct role so that staff privileges are not implicitly absolute.
class Admin : public User {
public:
    Admin() { role_ = Role::Admin; }

    Admin(std::string userId, std::string username, std::string fullName)
        : User(std::move(userId), std::move(username), Role::Admin, std::move(fullName)) {}

    std::string roleLabel() const override { return "ADMIN"; }

    std::unique_ptr<User> clone() const override { return std::make_unique<Admin>(*this); }
};

}  // namespace st