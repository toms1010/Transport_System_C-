#include "controllers/RouteController.hpp"

#include "AppContext.hpp"
#include "Widgets.hpp"

#include <algorithm>

namespace gui {

RouteController::RouteController(AppContext& context) : context_(context) {}

RouteRow RouteController::toRow(const st::Route& route) const {
    const st::RouteLoad load = context_.routes().loadFor(route.routeId());

    RouteRow row;
    row.id = qs(route.routeId());
    row.name = qs(route.routeName());
    row.from = qs(route.startLocation());
    row.to = qs(route.destination());
    row.via = route.via().empty() ? QStringLiteral("Direct") : qs(route.via());
    row.endpoints = QStringLiteral("%1 → %2").arg(row.from, row.to);
    row.fare = route.fare();
    row.capacity = route.capacity();
    row.occupied = load.occupiedCount;
    row.free = load.freeSeats();
    row.active = route.active();
    row.occupancyPercent =
        route.capacity() <= 0
            ? 0
            : static_cast<int>(load.occupiedCount * 100 / route.capacity());
    return row;
}

QVector<RouteRow> RouteController::rows() const {
    QVector<RouteRow> result;
    for (const st::Route& route : context_.routes().cache()) {
        result.push_back(toRow(route));
    }
    return result;
}

QVector<RouteOption> RouteController::options(bool onlyRoutesWithFreeSeats) const {
    QVector<RouteOption> result;
    for (const st::Route& route : context_.routes().cache()) {
        if (!route.active()) continue;

        const st::RouteLoad load = context_.routes().loadFor(route.routeId());
        if (onlyRoutesWithFreeSeats && load.freeSeats() <= 0) continue;

        RouteOption option;
        option.id = qs(route.routeId());
        option.name = qs(route.routeName());
        option.endpoints = QStringLiteral("%1 → %2")
                               .arg(qs(route.startLocation()), qs(route.destination()));
        option.via = route.via().empty() ? QStringLiteral("Direct") : qs(route.via());
        option.fare = route.fare();
        option.capacity = route.capacity();
        option.free = load.freeSeats();
        result.push_back(option);
    }
    return result;
}

st::Route RouteController::toRoute(const RouteInput& input) const {
    st::Route route(toStd(input.id.trimmed()), toStd(input.name.trimmed()),
                    toStd(input.from.trimmed()), toStd(input.to.trimmed()),
                    toStd(input.via.trimmed()), input.fare, input.capacity);
    route.setActive(input.active);
    return route;
}

ActionResult RouteController::add(const RouteInput& input) {
    std::string reason;
    if (!context_.routes().addRoute(toRoute(input), reason)) {
        return ActionResult::failure(QStringLiteral("Could not add route"), qs(reason));
    }
    return ActionResult::success(QStringLiteral("Route %1 created.").arg(input.id));
}

ActionResult RouteController::update(const RouteInput& input) {
    std::string reason;
    if (!context_.routes().updateRoute(toRoute(input), reason)) {
        return ActionResult::failure(QStringLiteral("Could not save route"), qs(reason));
    }
    return ActionResult::success(QStringLiteral("Route %1 updated.").arg(input.id));
}

ActionResult RouteController::remove(const QString& routeId) {
    std::string reason;
    if (!context_.routes().deleteRoute(toStd(routeId), reason)) {
        return ActionResult::failure(QStringLiteral("Could not delete route"), qs(reason));
    }
    return ActionResult::success(QStringLiteral("Route %1 deleted.").arg(routeId));
}

ActionResult RouteController::setActive(const QString& routeId, bool active) {
    st::Route* route = context_.routes().findMutable(toStd(routeId));
    if (!route) {
        return ActionResult::failure(QStringLiteral("Could not update route"),
                                     QStringLiteral("That route no longer exists."));
    }

    st::Route updated = *route;
    updated.setActive(active);

    std::string reason;
    if (!context_.routes().updateRoute(updated, reason)) {
        return ActionResult::failure(QStringLiteral("Could not update route"), qs(reason));
    }
    return ActionResult::success(active ? QStringLiteral("Route %1 is now active.").arg(routeId)
                                        : QStringLiteral("Route %1 is now inactive.").arg(routeId));
}

bool RouteController::exists(const QString& routeId) const {
    return context_.routes().routeExists(toStd(routeId));
}

QString RouteController::nameOf(const QString& routeId) const {
    if (routeId.isEmpty()) return QStringLiteral("Unassigned");
    const st::Route* route = context_.routes().find(toStd(routeId));
    return route ? qs(route->routeName()) : QStringLiteral("Unknown route");
}

QString RouteController::fareOf(const QString& routeId) const {
    const st::Route* route = context_.routes().find(toStd(routeId));
    return route ? money(route->fare()) : QStringLiteral("—");
}

int RouteController::freeSeatsOf(const QString& routeId) const {
    if (routeId.isEmpty()) return 0;
    return context_.routes().loadFor(toStd(routeId)).freeSeats();
}

int RouteController::capacityOf(const QString& routeId) const {
    const st::Route* route = context_.routes().find(toStd(routeId));
    return route ? route->capacity() : 0;
}

int RouteController::occupiedOf(const QString& routeId) const {
    if (routeId.isEmpty()) return 0;
    return context_.routes().loadFor(toStd(routeId)).occupiedCount;
}

}  // namespace gui