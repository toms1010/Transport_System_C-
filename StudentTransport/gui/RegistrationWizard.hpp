#pragma once

#include "services/AuthService.hpp"

#include <QDialog>
#include <QVector>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;

namespace gui {

class Controllers;

// Registration wizard (spec section 10).
//
// Four steps: personal details, credentials, transport route and seat, then a
// summary. The account type is chosen by which link opened the wizard rather
// than by a step of its own. The account is written only on the last step, so an
// abandoned wizard leaves no half-created account behind.
class RegistrationWizard : public QDialog {
    Q_OBJECT
public:
    RegistrationWizard(Controllers& controllers, bool asStaff, QWidget* parent = nullptr);

    QString createdUsername() const { return createdUsername_; }

private slots:
    void goNext();
    void goBack();
    void chooseRoute();
    void chooseSeat();
    void finish();

private:
    enum Step { Personal = 0, Credentials, Transport, Summary };

    void updateStepBar();
    void buildSteps();
    void updateSummary();
    bool validateCurrentStep();
    Controllers& controllers_;
    bool asStaff_;
    QString createdUsername_;
    QStackedWidget* steps_ = nullptr;
    QWidget* stepBar_ = nullptr;
    QVector<QLabel*> stepLabels_;

    QLabel* error_ = nullptr;

    // Step 2/3 fields.
    QLineEdit* fullName_ = nullptr;
    QLineEdit* fatherName_ = nullptr;
    QLineEdit* phone_ = nullptr;
    QLineEdit* address_ = nullptr;
    QLineEdit* username_ = nullptr;
    QLineEdit* password_ = nullptr;
    QLineEdit* confirm_ = nullptr;
    QLineEdit* authorization_ = nullptr;

    // Step 4 fields.
    QLabel* routeSummary_ = nullptr;
    QLabel* seatSummary_ = nullptr;
    QString selectedRouteId_;
    int selectedSeat_ = 0;

    // Step 5 fields.
    QLabel* summaryName_ = nullptr;
    QLabel* summaryUsername_ = nullptr;
    QLabel* summaryRoute_ = nullptr;
    QLabel* summarySeat_ = nullptr;
    QLabel* summaryFees_ = nullptr;

    QPushButton* seatButton_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QPushButton* nextButton_ = nullptr;
    QPushButton* finishButton_ = nullptr;
};

}  // namespace gui