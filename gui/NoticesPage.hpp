#pragma once

#include "Widgets.hpp"

class QLabel;
class QPushButton;
class QTableWidget;

namespace gui {

class NoticesPage : public Page {
    Q_OBJECT
public:
    NoticesPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void publish();
    void editSelected();
    void deleteSelected();
    void showSelected();

private:
    void build();
    QString selectedNoticeId() const;

    QTableWidget* table_ = nullptr;
    QLabel* detail_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
};

}  // namespace gui
