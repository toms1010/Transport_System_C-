#pragma once

#include "controllers/Controllers.hpp"

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace gui {

class LoginWindow : public QDialog {
    Q_OBJECT
public:
    LoginWindow(Controllers& controllers, QWidget* parent = nullptr);

    const st::Session& session() const { return controllers_.auth().session(); }

private slots:
    void attemptLogin();
    void registerStudent();
    void registerStaff();
    void showForgotPassword();
    void showPublicRoutes();
    void showPublicNotices();
    void showAbout();

private:
    void setError(const QString& message);

    Controllers& controllers_;

    QLineEdit* username_ = nullptr;
    QLineEdit* password_ = nullptr;
    QCheckBox* reveal_ = nullptr;
    QPushButton* signIn_ = nullptr;
    QLabel* error_ = nullptr;
};

}  // namespace gui