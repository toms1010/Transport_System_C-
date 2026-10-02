#include "repositories/UserRepository.hpp"

#include "models/Admin.hpp"
#include "models/Staff.hpp"
#include "models/Student.hpp"

#include "utils/CsvUtils.hpp"
#include "utils/FileUtils.hpp"
#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace st {
namespace {

std::string routeOf(const User& u) {
    if (const auto* staff = dynamic_cast<const Staff*>(&u)) return staff->assignedRouteId();
    if (const auto* student = dynamic_cast<const Student*>(&u)) return student->routeId();
    return std::string();
}

// Every field the domain needs is persisted. The previous version wrote only the
// common user columns, which silently dropped a student's seat, route and both
// fee balances on every save.
std::string encodeUser(const User& u) {
    const auto* student = dynamic_cast<const Student*>(&u);
    const auto* staff = dynamic_cast<const Staff*>(&u);

    std::vector<std::string> fields;
    fields.push_back(u.userId());
    fields.push_back(u.username());
    fields.push_back(roleName(u.role()));
    fields.push_back(u.fullName());
    fields.push_back(u.fatherName());
    fields.push_back(u.phone());
    fields.push_back(u.address());
    fields.push_back(u.passwordHash());
    fields.push_back(u.salt());
    fields.push_back(u.createdAt());
    fields.push_back(u.active() ? "1" : "0");
    fields.push_back(routeOf(u));
    fields.push_back(student ? std::to_string(student->seatNumber()) : "0");
    fields.push_back(student ? std::to_string(student->registrationFee()) : "0");
    fields.push_back(student ? std::to_string(student->registrationPaid()) : "0");
    fields.push_back(student ? std::to_string(student->transportFee()) : "0");
    fields.push_back(student ? std::to_string(student->transportPaid()) : "0");
    fields.push_back(staff ? staff->assignedRouteId() : std::string());
    return csvJoin(fields);
}

void applyCommon(User& u, const std::vector<std::string>& f) {
    u.setFatherName(f[4]);
    u.setPhone(f[5]);
    u.setAddress(f[6]);
    u.setPasswordHash(f[7]);
    u.setSalt(f[8]);
    u.setCreatedAt(f[9]);
    u.setActive(f.size() > 10 ? f[10] == "1" : true);
}

std::string fieldAt(const std::vector<std::string>& f, std::size_t index) {
    return index < f.size() ? f[index] : std::string();
}

}

CsvUserRepository::UserCache CsvUserRepository::loadAll() const {
    UserCache out;
    for (const std::string& line : readLines(path_)) {
        const std::vector<std::string> f = csvSplit(line);
        if (f.size() < 11) continue;

        const Role role = roleFromName(f[2]);

        if (role == Role::Staff) {
            auto staff = std::make_unique<Staff>(f[0], f[1], f[3]);
            applyCommon(*staff, f);
            staff->setAssignedRouteId(fieldAt(f, 17));
            out.push_back(std::move(staff));
            continue;
        }

        if (role == Role::Admin) {
            auto admin = std::make_unique<Admin>(f[0], f[1], f[3]);
            applyCommon(*admin, f);
            out.push_back(std::move(admin));
            continue;
        }

        auto student = std::make_unique<Student>(f[0], f[1], f[3]);
        applyCommon(*student, f);
        student->setRouteId(fieldAt(f, 11));
        long long seatNo = 0;
        util::parseWholeNumber(fieldAt(f, 12), seatNo);
        student->setSeatNumber(seatNo > 0 && seatNo < 100000 ? static_cast<int>(seatNo) : 0);
        long long regFee = 0;
        util::parseWholeNumber(fieldAt(f, 13), regFee);
        student->setRegistrationFee(regFee);
        long long regPaid = 0;
        util::parseWholeNumber(fieldAt(f, 14), regPaid);
        student->setRegistrationPaid(regPaid);
        long long trnFee = 0;
        util::parseWholeNumber(fieldAt(f, 15), trnFee);
        student->setTransportFee(trnFee);
        long long trnPaid = 0;
        util::parseWholeNumber(fieldAt(f, 16), trnPaid);
        student->setTransportPaid(trnPaid);
        out.push_back(std::move(student));
    }
    return out;
}

void CsvUserRepository::saveAll(const UserCache& users) const {
    std::vector<std::string> lines;
    lines.reserve(users.size());
    for (const auto& u : users) lines.push_back(encodeUser(*u));
    writeLines(path_, lines);
}

bool CsvUserRepository::save(const User& user, UserCache& cache) const {
    for (const auto& u : cache) {
        if (u->userId() == user.userId()) return update(user, cache);
    }
    cache.push_back(user.clone());
    saveAll(cache);
    return true;
}

bool CsvUserRepository::update(const User& user, UserCache& cache) const {
    for (auto& u : cache) {
        if (u->userId() != user.userId()) continue;
        // Replace the whole object so subclass state (fees, seat, route) survives.
        u = user.clone();
        saveAll(cache);
        return true;
    }
    return false;
}

bool CsvUserRepository::removeById(const std::string& userId, UserCache& cache) const {
    const auto it = std::find_if(cache.begin(), cache.end(),
                                 [&](const auto& u) { return u->userId() == userId; });
    if (it == cache.end()) return false;
    cache.erase(it);
    saveAll(cache);
    return true;
}

std::vector<Route> CsvRouteRepository::loadAll() const {
    std::vector<Route> out;
    for (const std::string& line : readLines(path_)) {
        const std::vector<std::string> f = csvSplit(line);
        if (f.size() < 8) continue;
        Route r(f[0], f[1], f[2], f[3], f[4], 0, 0);
        long long fare = 0;
        util::parseWholeNumber(f[5], fare);
        long long cap = 0;
        util::parseWholeNumber(f[6], cap);
        r.setFare(fare);
        r.setCapacity(cap > 0 && cap < 100000 ? static_cast<int>(cap) : 0);
        r.setActive(f[7] == "1");
        out.push_back(r);
    }
    return out;
}

void CsvRouteRepository::saveAll(const std::vector<Route>& routes) const {
    std::vector<std::string> lines;
    lines.reserve(routes.size());
    for (const Route& r : routes) {
        lines.push_back(csvJoin({r.routeId(), r.routeName(), r.startLocation(), r.destination(),
                                 r.via(), std::to_string(r.fare()), std::to_string(r.capacity()),
                                 r.active() ? "1" : "0"}));
    }
    writeLines(path_, lines);
}

bool CsvRouteRepository::save(const Route& route, std::vector<Route>& cache) const {
    for (const Route& r : cache) {
        if (r.routeId() == route.routeId()) return update(route, cache);
    }
    cache.push_back(route);
    saveAll(cache);
    return true;
}

bool CsvRouteRepository::update(const Route& route, std::vector<Route>& cache) const {
    for (Route& r : cache) {
        if (r.routeId() == route.routeId()) {
            r = route;
            saveAll(cache);
            return true;
        }
    }
    return false;
}

bool CsvRouteRepository::removeById(const std::string& routeId, std::vector<Route>& cache) const {
    const auto it = std::find_if(cache.begin(), cache.end(),
                                 [&](const Route& r) { return r.routeId() == routeId; });
    if (it == cache.end()) return false;
    cache.erase(it);
    saveAll(cache);
    return true;
}

void CsvRouteRepository::seedDefaults(std::vector<Route>& cache) const {
    if (!cache.empty()) return;
    cache.push_back(Route("R1", "Ameerpet - Uppal", "Ameerpet", "Uppal",
                          "Kachiguda, Secunderabad", 9000, 30));
    cache.push_back(Route("R2", "Gachibowli - Secunderabad", "Gachibowli", "Secunderabad",
                          "Nanakramguda, Banjara Hills", 8500, 30));
    cache.push_back(Route("R3", "Miyapur - L.B.Nagar", "Miyapur", "L.B.Nagar",
                          "Kukatpally, Moosarambagh", 9500, 25));
    cache.push_back(Route("R4", "Kukatpally - Charminar", "Kukatpally", "Charminar",
                          "Erragadda, Ameerpet", 8000, 25));
    cache.push_back(Route("R5", "Begumpet - Shamshabad", "Begumpet", "Shamshabad",
                          "Erragadda, Gachibowli", 7000, 20));
    saveAll(cache);
}

}  // namespace st