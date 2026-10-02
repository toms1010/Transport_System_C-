#include "AppContext.hpp"
#include "SeatMapPage.hpp"

#include "Theme.hpp"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace gui {
namespace {

constexpr int kColumns = 4;

QString stateToken(SeatState state) {
    switch (state) {
        case SeatState::Free:
            return QStringLiteral("free");
        case SeatState::Occupied:
            return QStringLiteral("taken");
        case SeatState::Mine:
            return QStringLiteral("mine");
        default:
            return QStringLiteral("selected");
    }
}

QWidget* legendSwatch(const QString& text, const QString& background, const QString& border) {
    auto* host = new QWidget;
    auto* row = new QHBoxLayout(host);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(7);

    auto* swatch = new QLabel;
    swatch->setFixedSize(15, 15);
    swatch->setStyleSheet("background: " + background + "; border: 1px solid " + border +
                          "; border-radius: 4px;");

    auto* caption = new QLabel(text);
    caption->setObjectName("Muted");

    row->addWidget(swatch);
    row->addWidget(caption);
    return host;
}

}  // namespace

SeatMapPage::SeatMapPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    auto* header = new PageHeader(
        isOperator() ? QStringLiteral("Seat map") : QStringLiteral("My transport"),
        isOperator() ? QStringLiteral("Allot seats to students and keep every route within "
                                      "capacity.")
                     : QStringLiteral("Pick a route, then click a free seat and claim it."));

    auto* controls = new QHBoxLayout;
    controls->setSpacing(10);

    auto* routeCaption = new QLabel(QStringLiteral("Route"));
    routeCaption->setObjectName("FieldLabel");
    routeBox_ = new QComboBox;
    routeBox_->setMinimumWidth(300);
    routeBox_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    controls->addWidget(routeCaption);
    controls->addWidget(routeBox_);

    if (isOperator()) {
        controls->addStretch();
        auto* studentCaption = new QLabel(QStringLiteral("Student"));
        studentCaption->setObjectName("FieldLabel");
        studentBox_ = new QComboBox;
        studentBox_->setMinimumWidth(320);
        studentBox_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        controls->addWidget(studentCaption);
        controls->addWidget(studentBox_);
    } else {
        controls->addStretch();
        changeRouteButton_ = ghostButton(QStringLiteral("Change route"));
        controls->addWidget(changeRouteButton_);
    }

    layout_->addWidget(header);
    layout_->addLayout(controls);

    auto* columns = new QHBoxLayout;
    columns->setSpacing(14);

    auto* seatCard = new Card;
    auto* seatTitle = new QLabel(QStringLiteral("Seat layout"));
    seatTitle->setObjectName("SectionTitle");
    routeInfo_ = new QLabel;
    routeInfo_->setObjectName("Muted");
    seatCard->body()->addWidget(seatTitle);
    seatCard->body()->addWidget(routeInfo_);

    auto* driver = new QFrame;
    driver->setObjectName("DriverBox");
    driver->setFixedHeight(30);
    auto* driverLayout = new QHBoxLayout(driver);
    driverLayout->setContentsMargins(0, 0, 0, 0);
    auto* driverLabel = new QLabel(QStringLiteral("F R O N T   /   D R I V E R"));
    driverLabel->setObjectName("DriverLabel");
    driverLayout->addWidget(driverLabel);
    seatCard->body()->addWidget(driver);
    seatCard->body()->addSpacing(10);

    gridHost_ = new QWidget;
    grid_ = new QGridLayout(gridHost_);
    grid_->setContentsMargins(0, 4, 0, 4);
    grid_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    grid_->setSpacing(10);
    seatCard->body()->addWidget(gridHost_);

    auto* legendRow = new QHBoxLayout;
    legendRow->setSpacing(18);
    legendRow->addWidget(legendSwatch(QStringLiteral("Free"), color::surface().name(),
                                      color::borderStrong().name()));
    legendRow->addWidget(legendSwatch(QStringLiteral("Occupied"), color::surfaceAlt().name(),
                                      color::border().name()));
    legendRow->addWidget(legendSwatch(QStringLiteral("Your seat"),
                                      color::successSoft().name(), color::success().name()));
    legendRow->addWidget(legendSwatch(QStringLiteral("Selected"), color::primary().name(),
                                      color::primary().name()));
    legendRow->addStretch();
    seatCard->body()->addSpacing(6);
    seatCard->body()->addLayout(legendRow);
    columns->addWidget(seatCard, 3);

    auto* side = new QVBoxLayout;
    side->setSpacing(14);

    auto* mineCard = new Card;
    auto* mineTitle = new QLabel(isOperator() ? QStringLiteral("Selected student")
                                              : QStringLiteral("My seat"));
    mineTitle->setObjectName("SectionTitle");
    summary_ = new QLabel(QStringLiteral("—"));
    summary_->setWordWrap(true);
    studentIdLabel_ = new QLabel;
    studentIdLabel_->setObjectName("Muted");
    mineCard->body()->addWidget(mineTitle);
    mineCard->body()->addWidget(summary_);
    mineCard->body()->addWidget(studentIdLabel_);

    auto* myButtons = new QVBoxLayout;
    myButtons->setSpacing(8);

    if (isOperator()) {
        assignButton_ = primaryButton(QStringLiteral("Assign the selected seat"));
        autoAssignButton_ = ghostButton(QStringLiteral("Auto-assign first free seat"));
        releaseStudentButton_ = dangerButton(QStringLiteral("Release this seat"));
        for (QPushButton* button : {assignButton_, autoAssignButton_, releaseStudentButton_}) {
            myButtons->addWidget(button);
        }
    } else {
        releaseMySeatButton_ = dangerButton(QStringLiteral("Release my seat"));
        myButtons->addWidget(releaseMySeatButton_);
        connect(releaseMySeatButton_, &QPushButton::clicked, this, &SeatMapPage::releaseMySeat);
        connect(changeRouteButton_, &QPushButton::clicked, this, [this] {
            pendingSeatNumber_ = 0;
            toast(QStringLiteral("Choose another route above, then click a free seat."));
        });
    }

    mineCard->body()->addLayout(myButtons);
    side->addWidget(mineCard);

    if (!isOperator()) {
        claimButton_ = primaryButton(QStringLiteral("Confirm the selected seat"));
        claimButton_->setMinimumHeight(38);
        claimButton_->hide();
        mineCard->body()->addWidget(claimButton_);
        connect(claimButton_, &QPushButton::clicked, this, &SeatMapPage::claimSelectedSeat);
    } else {
        connect(assignButton_, &QPushButton::clicked, this, &SeatMapPage::assignToStudent);
        connect(autoAssignButton_, &QPushButton::clicked, this, &SeatMapPage::autoAssign);
        connect(releaseStudentButton_, &QPushButton::clicked, this,
                &SeatMapPage::releaseStudentSeat);
    }

    auto* hintCard = new Card;
    auto* hintTitle = new QLabel(QStringLiteral("How this works"));
    hintTitle->setObjectName("SectionTitle");
    auto* hint = new QLabel(
        isOperator()
            ? QStringLiteral("A seat can only ever hold one student. Releasing a seat frees it "
                              "immediately for the next allotment.")
            : QStringLiteral("Your transport fee starts once you take a seat. Switching routes "
                             "releases your current seat first."));
    hint->setObjectName("Muted");
    hint->setWordWrap(true);
    hintCard->body()->addWidget(hintTitle);
    hintCard->body()->addWidget(hint);
    side->addWidget(hintCard);
    side->addStretch();

    columns->addLayout(side, 2);
    layout_->addLayout(columns);

    connect(routeBox_, &QComboBox::currentIndexChanged, this, &SeatMapPage::onRouteChanged);
    if (studentBox_) {
        connect(studentBox_, &QComboBox::currentIndexChanged, this, &SeatMapPage::onStudentChanged);
    }
}

QString SeatMapPage::currentRouteId() const {
    return routeBox_->currentIndex() < 0 ? QString() : routeBox_->currentData().toString();
}

QString SeatMapPage::currentStudentId() const {
    if (!isOperator()) return qs(session_.userId);
    if (!studentBox_) return {};
    const int index = studentBox_->currentIndex();
    if (index < 0 || index >= studentIds_.size()) return {};
    return studentIds_.at(index);
}

void SeatMapPage::populateRoutes() {
    const QString previous = currentRouteId();
    routeBox_->blockSignals(true);
    routeBox_->clear();

    for (const RouteOption& option : controllers_.routes().options(false)) {
        routeBox_->addItem(QStringLiteral("%1 · %2 — %3")
                               .arg(option.id, option.name, option.endpoints),
                           option.id);
    }

    const int index = routeBox_->findData(previous);
    if (index >= 0) routeBox_->setCurrentIndex(index);
    routeBox_->blockSignals(false);
}

void SeatMapPage::populateStudents() {
    if (!studentBox_) return;

    const int previousIndex = studentBox_->currentIndex();
    const QString previous = previousIndex >= 0 && previousIndex < studentIds_.size()
                                 ? studentIds_.at(previousIndex)
                                 : QString();

    studentIds_ = controllers_.students().idList();

    studentBox_->blockSignals(true);
    studentBox_->clear();
    for (const QString& label : controllers_.students().namesWithIds()) {
        studentBox_->addItem(label);
    }

    const int index = static_cast<int>(studentIds_.indexOf(previous));
    studentBox_->setCurrentIndex(index >= 0 ? index : 0);
    studentBox_->blockSignals(false);
}

void SeatMapPage::refresh() {
    populateRoutes();
    populateStudents();
    rebuildGrid();
}

void SeatMapPage::selectStudent(const QString& userId) {
    if (!studentBox_) return;
    const int index = static_cast<int>(studentIds_.indexOf(userId));
    if (index >= 0) studentBox_->setCurrentIndex(index);

    const QString routeId = controllers_.seats().routeIdOf(userId);
    if (routeId.isEmpty()) return;
    const int routeIndex = routeBox_->findData(routeId);
    if (routeIndex >= 0) routeBox_->setCurrentIndex(routeIndex);
}

void SeatMapPage::onRouteChanged() {
    pendingSeatNumber_ = 0;
    rebuildGrid();
}

void SeatMapPage::onStudentChanged() {
    pendingSeatNumber_ = 0;
    rebuildGrid();
}

void SeatMapPage::rebuildGrid() {
    while (QLayoutItem* item = grid_->takeAt(0)) {
        if (QWidget* widget = item->widget()) widget->deleteLater();
        delete item;
    }

    seats_.clear();
    pendingSeatNumber_ = 0;

    const QString routeId = currentRouteId();
    const QString viewerId = currentStudentId();

    if (routeId.isEmpty()) {
        routeInfo_->setText(QStringLiteral("No routes have been created yet."));
        updateLegend();
        return;
    }

    routeInfo_->setText(QStringLiteral("%1 seats  ·  %2 occupied  ·  %3 free  ·  fare %4")
                            .arg(QString::number(controllers_.routes().capacityOf(routeId)),
                                 QString::number(controllers_.routes().occupiedOf(routeId)),
                                 QString::number(controllers_.routes().freeSeatsOf(routeId)),
                                 controllers_.routes().fareOf(routeId)));

    const QVector<SeatView> views = controllers_.seats().seatMap(routeId, viewerId);
    seats_.assign(views.constBegin(), views.constEnd());

    for (const SeatView& view : seats_) {
        auto* button = new QPushButton(QString::number(view.number));
        button->setObjectName("SeatButton");
        button->setProperty("seatState", stateToken(view.state));
        button->setFixedSize(62, 48);
        button->setToolTip(view.tooltip);
        button->setEnabled(view.state == SeatState::Free || view.state == SeatState::Mine);
        button->setCursor(button->isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);

        const int number = view.number;
        connect(button, &QPushButton::clicked, this, [this, number] {
            pendingSeatNumber_ = number;
            updateLegend();
        });

        grid_->addWidget(button, (number - 1) / kColumns, (number - 1) % kColumns);
    }

    if (seats_.empty()) {
        auto* empty = new QLabel(QStringLiteral("This route has no seats configured."));
        empty->setObjectName("Muted");
        grid_->addWidget(empty, 0, 0);
    }

    updateLegend();
}

void SeatMapPage::updateLegend() {
    const QString routeId = currentRouteId();
    const QString viewerId = currentStudentId();

    int index = 0;
    while (QLayoutItem* item = grid_->itemAt(index)) {
        if (auto* button = qobject_cast<QPushButton*>(item->widget())) {
            const int number = button->text().toInt();
            SeatState state = SeatState::Free;
            QString tooltip = QStringLiteral("Seat %1").arg(number);

            for (const SeatView& view : seats_) {
                if (view.number != number) continue;
                state = view.state;
                tooltip = view.tooltip;
                break;
            }

            if (number == pendingSeatNumber_ && state == SeatState::Free) {
                state = SeatState::Selected;
                tooltip = QStringLiteral("Seat %1 · selected").arg(number);
            }

            button->setProperty("seatState", stateToken(state));
            button->setToolTip(tooltip);
            button->style()->unpolish(button);
            button->style()->polish(button);
            button->setCursor(state == SeatState::Occupied ? Qt::ArrowCursor
                                                           : Qt::PointingHandCursor);
        }
        ++index;
    }

    StudentRow student;
    const bool haveStudent = controllers_.students().findRow(viewerId, student);

    if (haveStudent) {
        summary_->setText(student.hasSeat
                              ? QStringLiteral("Holds seat %1 on %2.")
                                    .arg(student.seatLabel, student.routeName)
                              : QStringLiteral("No seat allotted yet."));
        studentIdLabel_->setText(QStringLiteral("%1 · @%2").arg(student.name, student.username));
    } else {
        summary_->setText(QStringLiteral("Select a student to see their allotment."));
        studentIdLabel_->clear();
    }

    const bool pendingIsFree =
        pendingSeatNumber_ > 0 && controllers_.seats().isFree(routeId, pendingSeatNumber_);

    if (claimButton_) claimButton_->setVisible(pendingIsFree);
    if (releaseMySeatButton_) releaseMySeatButton_->setEnabled(haveStudent && student.hasSeat);
    if (assignButton_) assignButton_->setEnabled(haveStudent && pendingIsFree);
    if (autoAssignButton_) autoAssignButton_->setEnabled(haveStudent);
    if (releaseStudentButton_) releaseStudentButton_->setEnabled(haveStudent && student.hasSeat);
}

void SeatMapPage::claimSelectedSeat() {
    if (pendingSeatNumber_ <= 0) return;
    toast(controllers_.seats().claim(qs(session_.userId), currentRouteId(), pendingSeatNumber_));
    refresh();
}

void SeatMapPage::releaseMySeat() {
    if (!confirm(QStringLiteral("Release seat"),
                 QStringLiteral("Give up your seat on this route? It becomes free immediately "
                                "for someone else."))) {
        return;
    }
    toast(controllers_.seats().release(qs(session_.userId)));
    refresh();
}

void SeatMapPage::assignToStudent() {
    if (pendingSeatNumber_ <= 0) return;
    toast(controllers_.seats().assign(currentStudentId(), currentRouteId(), pendingSeatNumber_));
    refresh();
}

void SeatMapPage::autoAssign() {
    toast(controllers_.seats().autoAssign(currentStudentId(), currentRouteId()));
    refresh();
}

void SeatMapPage::releaseStudentSeat() {
    const QString studentId = currentStudentId();
    StudentRow student;
    controllers_.students().findRow(studentId, student);
    if (!confirm(QStringLiteral("Release seat"),
                 QStringLiteral("Release %1's seat on %2?")
                     .arg(student.name.isEmpty() ? studentId : student.name, student.routeName))) {
        return;
    }
    toast(controllers_.seats().release(studentId));
    refresh();
}

}  // namespace gui