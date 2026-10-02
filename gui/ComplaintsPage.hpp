#pragma once

#include "controllers/ComplaintController.hpp"
#include "Widgets.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace gui {

class ComplaintsPage : public Page {
    Q_OBJECT
public:
    ComplaintsPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void submitComplaint();
    void resolveSelected();
    void reopenSelected();
    void showDetail();
    void applyFilters();

private:
    void build();
    void buildStudentView();
    void buildStaffView();
    QString selectedComplaintId() const;

    QTableWidget* table_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QLineEdit* search_ = nullptr;

    QLineEdit* subject_ = nullptr;
    QPlainTextEdit* description_ = nullptr;
    QLabel* formError_ = nullptr;

    QPushButton* resolveButton_ = nullptr;
    QPushButton* reopenButton_ = nullptr;
    QLabel* detail_ = nullptr;
};

}  // namespace gui
