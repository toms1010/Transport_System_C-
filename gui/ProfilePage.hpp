#pragma once

#include "Widgets.hpp"

class QLabel;
class QLineEdit;
class QPushButton;

namespace gui {

class ProfilePage : public Page {
    Q_OBJECT
public:
    ProfilePage(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    void refresh() override;

private slots:
    void saveDetails();
    void changePassword();
    void toggleEditing();

private:
    void build();

    QLabel* fullName_ = nullptr;
    QLabel* username_ = nullptr;
    QLabel* role_ = nullptr;
    QLabel* father_ = nullptr;
    QLabel* phone_ = nullptr;
    QLabel* address_ = nullptr;
    QLabel* createdAt_ = nullptr;
    QLabel* route_ = nullptr;
    QLabel* seat_ = nullptr;

    QLineEdit* fullNameEdit_ = nullptr;
    QLineEdit* fatherEdit_ = nullptr;
    QLineEdit* phoneEdit_ = nullptr;
    QLineEdit* addressEdit_ = nullptr;
    QPushButton* saveButton_ = nullptr;
    QLabel* editMessage_ = nullptr;

    QLineEdit* currentSecret_ = nullptr;
    QLineEdit* newSecret_ = nullptr;
    QLineEdit* confirmSecret_ = nullptr;
    QPushButton* passwordButton_ = nullptr;
    QLabel* passwordMessage_ = nullptr;
};

}  // namespace gui
