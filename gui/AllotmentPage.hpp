#pragma once

#include "Widgets.hpp"

class QLabel;
class QPushButton;

namespace gui {

// Printable transport allotment card (spec section 15).
class AllotmentPage : public Page {
    Q_OBJECT
public:
    AllotmentPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void printCard();

private:
    void build();

    QLabel* name_ = nullptr;
    QLabel* studentId_ = nullptr;
    QLabel* username_ = nullptr;
    QLabel* route_ = nullptr;
    QLabel* from_ = nullptr;
    QLabel* to_ = nullptr;
    QLabel* via_ = nullptr;
    QLabel* seat_ = nullptr;
    QLabel* fare_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* validity_ = nullptr;
};

}  // namespace gui
