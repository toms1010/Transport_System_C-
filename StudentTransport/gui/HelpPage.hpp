#pragma once

#include "Widgets.hpp"

namespace gui {

class HelpPage : public Page {
    Q_OBJECT
public:
    HelpPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private:
    void build();
};

}  // namespace gui
