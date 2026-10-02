#pragma once

#include "models/Route.hpp"
#include "models/Staff.hpp"
#include "models/Student.hpp"
#include "models/User.hpp"

#include <memory>
#include <string>
#include <vector>

namespace st {

class CsvUserRepository {
public:
    explicit CsvUserRepository(std::string path) : path_(std::move(path)) {}

    using UserCache = std::vector<std::unique_ptr<User>>;

    UserCache loadAll() const;
    void saveAll(const UserCache& users) const;

    bool save(const User& user, UserCache& cache) const;
    bool update(const User& user, UserCache& cache) const;
    bool removeById(const std::string& userId, UserCache& cache) const;

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

class CsvRouteRepository {
public:
    explicit CsvRouteRepository(std::string path) : path_(std::move(path)) {}

    std::vector<Route> loadAll() const;
    void saveAll(const std::vector<Route>& routes) const;

    bool save(const Route& route, std::vector<Route>& cache) const;
    bool update(const Route& route, std::vector<Route>& cache) const;
    bool removeById(const std::string& routeId, std::vector<Route>& cache) const;

    void seedDefaults(std::vector<Route>& cache) const;

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

}  // namespace st