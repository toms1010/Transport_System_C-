#pragma once

#include "controllers/RouteController.hpp"
#include "Widgets.hpp"

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;

namespace gui {

// Form used for both creating and editing a route.
class RouteDialog : public QDialog {
    Q_OBJECT
public:
    RouteDialog(const RouteInput& input, bool editing, QWidget* parent = nullptr);

    RouteInput input() const;

private slots:
    void submit();

private:
    void setError(const QString& message);

    QLineEdit* id_ = nullptr;
    QLineEdit* name_ = nullptr;
    QLineEdit* from_ = nullptr;
    QLineEdit* to_ = nullptr;
    QLineEdit* via_ = nullptr;
    QSpinBox* fare_ = nullptr;
    QSpinBox* capacity_ = nullptr;
    QCheckBox* active_ = nullptr;
    QLabel* error_ = nullptr;
    bool editing_ = false;
};

class RoutesPage : public Page {
    Q_OBJECT
public:
    RoutesPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void addRoute();
    void editRoute();
    void deleteRoute();
    void toggleActive();

private:
    QString selectedRouteId() const;
    RouteInput selectedInput() const;

    QTableWidget* table_ = nullptr;
};

}  // namespace gui