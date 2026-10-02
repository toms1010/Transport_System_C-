#pragma once

#include "controllers/ActionResult.hpp"
#include "controllers/Controllers.hpp"

#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QString>
#include <QTableWidget>
#include <QWidget>

class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

namespace gui {

enum class ToastKind;

QString qs(const std::string& value);
std::string toStd(const QString& value);
QString money(long long value);
QString initialsOf(const QString& name);

// Base for every page in the application.
//
// A page is a QScrollArea whose content is built once in the constructor and
// repopulated by refresh(). It holds a Controllers bundle, never a service, so
// slots stay limited to turning user intent into a controller call.
class Page : public QScrollArea {
    Q_OBJECT
public:
    Page(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

    virtual void refresh() = 0;

    bool isStudent() const { return session_.role == st::Role::Student; }
    bool isStaff() const { return session_.role == st::Role::Staff; }
    bool isAdmin() const { return session_.role == st::Role::Admin; }
    bool isOperator() const { return session_.role != st::Role::Student; }

protected:
    // Every page lives in one QStackedWidget, so a page can hand control to a
    // sibling without knowing anything about MainWindow.
    template <typename T>
    T* siblingPage() const {
        if (!parentWidget()) return nullptr;
        return parentWidget()->findChild<T*>();
    }

    static void activate(QWidget* page);

    void toast(const QString& message, int kind = 0);
    void toast(const ActionResult& result);
    void warn(const QString& title, const QString& message);

    // Modal confirmation used before every destructive action.
    bool confirm(const QString& title, const QString& message,
                 const QString& acceptLabel = QStringLiteral("Confirm"));

    void selectRowOf(QTableWidget* table, const QString& firstColumnValue);

    Controllers& controllers_;
    st::Session session_;
    QWidget* content_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
};

class Card : public QFrame {
    Q_OBJECT
public:
    explicit Card(QWidget* parent = nullptr);
    QVBoxLayout* body() const { return layout_; }

private:
    QVBoxLayout* layout_ = nullptr;
};

class StatCard : public QFrame {
    Q_OBJECT
public:
    StatCard(const QString& label, QWidget* parent = nullptr);
    void setValue(const QString& value);
    void setAccent(const QString& color);
    void setCaption(const QString& caption);

private:
    QLabel* value_ = nullptr;
    QLabel* caption_ = nullptr;
};

class PageHeader : public QWidget {
    Q_OBJECT
public:
    explicit PageHeader(const QString& title, const QString& subtitle,
                        QWidget* parent = nullptr);
    void addAction(QWidget* button);
    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);

private:
    QLabel* title_ = nullptr;
    QLabel* subtitle_ = nullptr;
    QWidget* actions_ = nullptr;
    QHBoxLayout* actionsLayout_ = nullptr;
};

class Avatar : public QLabel {
    Q_OBJECT
public:
    explicit Avatar(const QString& name, QWidget* parent = nullptr);
    void setName(const QString& name);
};

void configureTable(QTableWidget* table);
void showEmptyState(QTableWidget* table, const QString& message);

QTableWidgetItem* textItem(const QString& text);
QTableWidgetItem* moneyItem(long long value);
QTableWidgetItem* centeredItem(const QString& text);
QTableWidgetItem* statusItem(const QString& text, const QString& kind);

QLabel* badge(const QString& text, const QString& background, const QString& foreground);
QWidget* rowField(const QString& label, const QString& value);
QLabel* iconLabel(const QString& iconName, int size = 20);

QPushButton* primaryButton(const QString& text, const QString& iconName = QString());
QPushButton* ghostButton(const QString& text, const QString& iconName = QString());
QPushButton* dangerButton(const QString& text, const QString& iconName = QString());

}  // namespace gui