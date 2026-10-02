#pragma once

#include "controllers/Controllers.hpp"

#include <QDialog>

class QTabWidget;
class QTableWidget;

namespace gui {

// Read-only window reachable before signing in: the route list, the notice board
// and the about panel (spec section 1, "public pages").
class PublicWindow : public QDialog {
    Q_OBJECT
public:
    enum class Tab { Routes, Notices, About };

    PublicWindow(Controllers& controllers, Tab initial, QWidget* parent = nullptr);

private:
    void build();
    void buildRoutesTab();
    void buildNoticesTab();
    void buildAboutTab();

    Controllers& controllers_;
    QTabWidget* tabs_ = nullptr;
    QTableWidget* routes_ = nullptr;
    QTableWidget* notices_ = nullptr;
};

}  // namespace gui