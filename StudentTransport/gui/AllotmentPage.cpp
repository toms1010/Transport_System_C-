#include "AppContext.hpp"
#include "AllotmentPage.hpp"

#include "Theme.hpp"
#include "utils/TextUtils.hpp"

#include <QFrame>
#include <QPainter>
#include <QPen>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QVBoxLayout>

namespace gui {

AllotmentPage::AllotmentPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void AllotmentPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Transport allotment card"),
        QStringLiteral("Carry this card when you board. It shows your route, seat and fee "
                       "position."));
    auto* printButton = primaryButton(QStringLiteral("Print card"), QStringLiteral("print"));
    header->addAction(printButton);
    layout_->addWidget(header);

    auto* card = new QFrame;
    card->setObjectName("Card");
    card->setMaximumWidth(560);

    auto* outer = new QVBoxLayout(card);
    outer->setContentsMargins(28, 26, 28, 26);
    outer->setSpacing(14);

    auto* banner = new QLabel(QStringLiteral("STUDENT TRANSPORT CARD"));
    banner->setObjectName("Banner");
    banner->setAlignment(Qt::AlignCenter);
    outer->addWidget(banner);

    name_ = new QLabel;
    name_->setAlignment(Qt::AlignCenter);
    name_->setStyleSheet("font-size: 19px; font-weight: 700;");
    studentId_ = new QLabel;
    studentId_->setAlignment(Qt::AlignCenter);
    studentId_->setObjectName("Muted");
    outer->addWidget(name_);
    outer->addWidget(studentId_);

    auto* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: " + color::border().name() + ";");
    outer->addWidget(divider);

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(10);

    const auto addRow = [&](int row, const QString& caption, QLabel** target) {
        *target = new QLabel;
        (*target)->setObjectName("FieldValue");
        (*target)->setWordWrap(true);
        grid->addWidget(rowField(caption, QString()), row, 0);
        grid->addWidget(*target, row, 1);
    };

    addRow(0, QStringLiteral("Username"), &username_);
    addRow(1, QStringLiteral("Route"), &route_);
    addRow(2, QStringLiteral("From"), &from_);
    addRow(3, QStringLiteral("To"), &to_);
    addRow(4, QStringLiteral("Via"), &via_);
    addRow(5, QStringLiteral("Seat number"), &seat_);
    addRow(6, QStringLiteral("Transport fee"), &fare_);
    addRow(7, QStringLiteral("Payment status"), &status_);
    grid->setColumnStretch(1, 1);
    outer->addLayout(grid);

    auto* divider2 = new QFrame;
    divider2->setFrameShape(QFrame::HLine);
    divider2->setStyleSheet("color: " + color::border().name() + ";");
    outer->addWidget(divider2);

    validity_ = new QLabel;
    validity_->setObjectName("Muted");
    validity_->setAlignment(Qt::AlignCenter);
    outer->addWidget(validity_);

    layout_->addWidget(card);
    layout_->addStretch();

    connect(printButton, &QPushButton::clicked, this, &AllotmentPage::printCard);
}

void AllotmentPage::refresh() {
    const StatementView statement = controllers_.payments().statement(qs(session_.userId));
    if (!statement.valid) return;

    StudentRow student;
    controllers_.students().findRow(qs(session_.userId), student);

    name_->setText(statement.studentName);
    studentId_->setText(QStringLiteral("Student ID: %1").arg(statement.studentId));
    username_->setText(statement.username);
    route_->setText(QStringLiteral("%1 · %2").arg(student.routeId, student.routeName));
    seat_->setText(student.seatLabel);
    fare_->setText(money(statement.transport.charge));
    status_->setText(statement.statusLabel);
    validity_->setText(QStringLiteral("%1  ·  issued %2")
                           .arg(controllers_.context().institutionName(),
                                qs(st::util::dateStamp())));

    const QString routeId = student.routeId;
    QString fromText = QStringLiteral("—");
    QString toText = QStringLiteral("—");
    QString viaText = QStringLiteral("—");
    for (const RouteRow& route : controllers_.routes().rows()) {
        if (route.id != routeId) continue;
        fromText = route.from;
        toText = route.to;
        viaText = route.via;
        break;
    }
    from_->setText(fromText);
    to_->setText(toText);
    via_->setText(viaText);
}

void AllotmentPage::printCard() {
    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(QStringLiteral("Transport allotment — %1").arg(qs(session_.displayName)));

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(QStringLiteral("Print allotment card"));
    if (dialog.exec() != QDialog::Accepted) return;

    QPainter painter;
    if (!painter.begin(&printer)) {
        warn(QStringLiteral("Print failed"), QStringLiteral("The printer could not be opened."));
        return;
    }

    const QString institution = controllers_.context().institutionName();
    const StatementView statement = controllers_.payments().statement(qs(session_.userId));
    StudentRow student;
    controllers_.students().findRow(qs(session_.userId), student);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(20, 20, 20)));
    painter.drawText(QRect(0, 40, static_cast<int>(printer.pageRect(QPrinter::DevicePixel).width()), 40),
                     Qt::AlignHCenter, institution.toUpper());
    painter.setFont(QFont(painter.font().family(), 20, QFont::Bold));
    painter.drawText(QRect(0, 90, static_cast<int>(printer.pageRect(QPrinter::DevicePixel).width()), 40),
                     Qt::AlignHCenter, QStringLiteral("Student Transport Card"));
    painter.setFont(QFont(painter.font().family(), 12));

    int y = 180;
    const auto line = [&](const QString& label, const QString& value) {
        painter.drawText(60, y, label);
        painter.drawText(260, y, value);
        y += 34;
    };

    line(QStringLiteral("Student"), statement.studentName);
    line(QStringLiteral("Student ID"), statement.studentId);
    line(QStringLiteral("Username"), statement.username);
    line(QStringLiteral("Route"), QStringLiteral("%1 · %2").arg(student.routeId, student.routeName));
    line(QStringLiteral("Seat"), student.seatLabel);
    line(QStringLiteral("Transport fee"), money(statement.transport.charge));
    line(QStringLiteral("Payment status"), statement.statusLabel);
    line(QStringLiteral("Issued"), qs(st::util::dateStamp()));

    painter.drawText(60, y + 30,
                     QStringLiteral("This card must be carried when boarding. Report a lost card "
                                    "to the transport office immediately."));
    painter.end();

    toast(QStringLiteral("Allotment card sent to the printer."));
}

}  // namespace gui
