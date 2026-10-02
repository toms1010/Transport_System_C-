#pragma once

#include "controllers/Controllers.hpp"

#include <QMainWindow>
#include <QStringList>
#include <QVector>

class QAbstractButton;
class QButtonGroup;
class QFrame;
class QLabel;
class QPushButton;
class QStackedWidget;

namespace gui {

class Page;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Controllers& controllers, const st::Session& session, QWidget* parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void onNavClicked(QAbstractButton* button);
    void onLogout();
    void onThemeToggled();
    void refreshCurrentPage();

private:
    void buildSidebar();
    void buildTopBar();
    void registerPages();
    Page* createPage(const QString& label);
    QStringList navLabels() const;
    void applyThemeIcon();

    Controllers& controllers_;
    st::Session session_;

    QFrame* sidebar_ = nullptr;
    QWidget* topBar_ = nullptr;
    QStackedWidget* stack_ = nullptr;

    QButtonGroup* navGroup_ = nullptr;
    QVector<QPushButton*> navButtons_;
    QStringList navLabels_;
    QVector<Page*> pages_;

    QLabel* windowTitle_ = nullptr;
    QLabel* nameLabel_ = nullptr;
    QPushButton* themeButton_ = nullptr;
};

}  // namespace gui