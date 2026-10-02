#pragma once

#include "controllers/ActionResult.hpp"

#include "models/Route.hpp"

#include <QString>
#include <QVector>

namespace gui {

class AppContext;

struct RouteRow {
    QString id;
    QString name;
    QString from;
    QString to;
    QString via;
    QString endpoints;
    long long fare = 0;
    int capacity = 0;
    int occupied = 0;
    int free = 0;
    int occupancyPercent = 0;
    bool active = true;
};

struct RouteOption {
    QString id;
    QString name;
    QString endpoints;
    QString via;
    long long fare = 0;
    int capacity = 0;
    int free = 0;
};

struct RouteInput {
    QString id;
    QString name;
    QString from;
    QString to;
    QString via;
    long long fare = 0;
    int capacity = 0;
    bool active = true;
};

class RouteController {
public:
    explicit RouteController(AppContext& context);

    QVector<RouteRow> rows() const;
    QVector<RouteOption> options(bool onlyRoutesWithFreeSeats) const;

    ActionResult add(const RouteInput& input);
    ActionResult update(const RouteInput& input);
    ActionResult remove(const QString& routeId);
    ActionResult setActive(const QString& routeId, bool active);

    bool exists(const QString& routeId) const;
    QString nameOf(const QString& routeId) const;
    QString fareOf(const QString& routeId) const;
    int freeSeatsOf(const QString& routeId) const;
    int capacityOf(const QString& routeId) const;
    int occupiedOf(const QString& routeId) const;

private:
    RouteRow toRow(const st::Route& route) const;
    st::Route toRoute(const RouteInput& input) const;

    AppContext& context_;
};

}  // namespace gui