#pragma once

#include "Widgets.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace gui {

class PaymentsPage : public Page {
    Q_OBJECT
public:
    PaymentsPage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

    // Used when the roster hands over a student whose dues need collecting.
    void selectStudent(const QString& userId);

private slots:
    void submitPayment();
    void settleBucket();
    void onStudentChanged();
    void onBucketChanged();
    void exportHistory();

private:
    void build();
    void populateStudents();
    QString currentStudentId() const;
    st::FeeKind currentBucket() const;
    void updateStatement();
    void updateHistory();
    void setMessage(const QString& text, bool isError);

    QComboBox* studentBox_ = nullptr;
    QComboBox* bucketBox_ = nullptr;
    QSpinBox* amount_ = nullptr;
    QLineEdit* reference_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QPushButton* settleButton_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QLabel* message_ = nullptr;

    QLabel* studentName_ = nullptr;
    QLabel* studentMeta_ = nullptr;
    QLabel* totalLabel_ = nullptr;
    QLabel* totalCaption_ = nullptr;
    QLabel* registrationLabel_ = nullptr;
    QLabel* registrationCaption_ = nullptr;
    QProgressBar* registrationBar_ = nullptr;
    QLabel* transportLabel_ = nullptr;
    QLabel* transportCaption_ = nullptr;
    QProgressBar* transportBar_ = nullptr;

    QTableWidget* history_ = nullptr;
    QStringList studentIds_;
};

}  // namespace gui