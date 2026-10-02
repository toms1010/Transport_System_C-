#include "models/Complaint.hpp"
#include "models/Route.hpp"
#include "models/User.hpp"

#include "utils/TextUtils.hpp"

namespace st {

const char* roleName(Role role) {
    switch (role) {
        case Role::Staff: return "STAFF";
        case Role::Admin: return "ADMIN";
        case Role::Student:
        default: return "STUDENT";
    }
}

Role roleFromName(const std::string& name) {
    const std::string upper = util::upper(util::trim(name));
    if (upper == "STAFF") return Role::Staff;
    if (upper == "ADMIN") return Role::Admin;
    return Role::Student;
}

std::string Route::endpoints() const {
    if (startLocation_.empty() && destination_.empty()) return routeName_;
    return startLocation_ + " -> " + destination_;
}

std::string Route::label() const {
    return routeId_ + "  " + endpoints();
}

const char* complaintStatusName(ComplaintStatus status) {
    return status == ComplaintStatus::Resolved ? "RESOLVED" : "OPEN";
}

ComplaintStatus complaintStatusFromName(const std::string& name) {
    return util::upper(util::trim(name)) == "RESOLVED" ? ComplaintStatus::Resolved
                                                       : ComplaintStatus::Open;
}

}  // namespace st