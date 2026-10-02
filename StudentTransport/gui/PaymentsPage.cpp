#include "AppContext.hpp"
#include "PaymentsPage.hpp"

#include "Notify.hpp"
#include "Theme.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {

PaymentsPage::PaymentsPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void PaymentsPage::build() {
    auto* header = new PageHeader(
        isOperator() ? QStringLiteral("Payments & dues") : QStringLiteral("Payments & dues"),
        QStringLiteral("Registration and transport fees are separate balances, so a payment "
                       "always lands in the bucket you choose."));
    layout_->addWidget(header);

    if (isOperator()) {
        auto* picker = new QHBoxLayout;
        picker->setSpacing(10);
        auto* caption = new QLabel(QStringLiteral("Student"));
        caption->setObjectName("FieldLabel");
        studentBox_ = new QComboBox;
        studentBox_->setMinimumWidth(360);
        picker->addWidget(caption);
        picker->addWidget(studentBox_);
        picker->addStretch();
        layout_->addLayout(picker);
    }

    auto* columns = new QHBoxLayout;
    columns->setSpacing(14);

    auto* statement = new Card;
    auto* statementTitle = new QLabel(QStringLiteral("Fee summary"));
    statementTitle->setObjectName("SectionTitle");
    statement->body()->addWidget(statementTitle);

    studentName_ = new QLabel(QStringLiteral("—"));
    studentName_->setObjectName("SectionTitle");
    studentMeta_ = new QLabel;
    studentMeta_->setObjectName("Muted");
    studentMeta_->setWordWrap(true);
    statement->body()->addWidget(studentName_);
    statement->body()->addWidget(studentMeta_);
    statement->body()->addSpacing(8);

    totalCaption_ = new QLabel;
    totalCaption_->setObjectName("StatLabel");
    totalLabel_ = new QLabel(QStringLiteral("—"));
    totalLabel_->setObjectName("StatValue");
    statement->body()->addWidget(totalCaption_);
    statement->body()->addWidget(totalLabel_);
    statement->body()->addSpacing(12);

    registrationCaption_ = new QLabel;
    registrationLabel_ = new QLabel;
    registrationBar_ = new QProgressBar;
    transportCaption_ = new QLabel;
    transportLabel_ = new QLabel;
    transportBar_ = new QProgressBar;

    for (QLabel* caption : {registrationCaption_, transportCaption_}) {
        caption->setObjectName("FieldLabel");
    }
    for (QLabel* value : {registrationLabel_, transportLabel_}) {
        value->setObjectName("FieldValue");
    }
    for (QProgressBar* bar : {registrationBar_, transportBar_}) {
        bar->setTextVisible(false);
        bar->setFixedHeight(9);
    }

    const auto bucketLine = [&](QLabel* caption, QLabel* value, QProgressBar* bar) {
        auto* line = new QHBoxLayout;
        line->addWidget(caption);
        line->addStretch();
        line->addWidget(value);
        statement->body()->addLayout(line);
        statement->body()->addWidget(bar);
        statement->body()->addSpacing(10);
    };

    bucketLine(registrationCaption_, registrationLabel_, registrationBar_);
    bucketLine(transportCaption_, transportLabel_, transportBar_);
    statement->body()->addStretch();

    auto* form = new Card;
    auto* formTitle = new QLabel(QStringLiteral("Make a payment"));
    formTitle->setObjectName("SectionTitle");
    form->body()->addWidget(formTitle);

    auto* fields = new QFormLayout;
    fields->setSpacing(10);

    bucketBox_ = new QComboBox;
    bucketBox_->addItem(QStringLiteral("Registration fee"),
                        static_cast<int>(st::FeeKind::Registration));
    bucketBox_->addItem(QStringLiteral("Transport fee"),
                        static_cast<int>(st::FeeKind::Transport));

    amount_ = new QSpinBox;
    amount_->setRange(0, 100000000);
    amount_->setPrefix(QStringLiteral("₹ "));

    reference_ = new QLineEdit;
    reference_->setPlaceholderText(QStringLiteral("Cash, UPI, cheque number…"));
    reference_->setMaxLength(64);

    fields->addRow(QStringLiteral("Payment type"), bucketBox_);
    fields->addRow(QStringLiteral("Amount"), amount_);
    fields->addRow(QStringLiteral("Reference"), reference_);

    submitButton_ = primaryButton(QStringLiteral("Confirm payment"));
    submitButton_->setMinimumHeight(36);
    settleButton_ = ghostButton(QStringLiteral("Settle this fee in full"));

    message_ = new QLabel;
    message_->setWordWrap(true);
    message_->hide();

    form->body()->addLayout(fields);
    form->body()->addSpacing(10);
    form->body()->addWidget(submitButton_);
    form->body()->addWidget(settleButton_);
    form->body()->addWidget(message_);
    form->body()->addStretch();

    columns->addWidget(statement, 3);
    columns->addWidget(form, 2);
    layout_->addLayout(columns);

    auto* historyCard = new Card;
    auto* historyHeader = new QHBoxLayout;
    auto* historyTitle = new QLabel(QStringLiteral("Payment history"));
    historyTitle->setObjectName("SectionTitle");
    exportButton_ = ghostButton(QStringLiteral("Export"), QStringLiteral("download"));
    historyHeader->addWidget(historyTitle);
    historyHeader->addStretch();
    historyHeader->addWidget(exportButton_);
    historyCard->body()->addLayout(historyHeader);

    history_ = new QTableWidget;
    history_->setColumnCount(5);
    history_->setHorizontalHeaderLabels({QStringLiteral("Receipt"), QStringLiteral("Date"),
                                         QStringLiteral("Fee type"),
                                         QStringLiteral("Recorded by"),
                                         QStringLiteral("Amount")});
    history_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    history_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    history_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    history_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    history_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    configureTable(history_);
    history_->setMinimumHeight(230);
    historyCard->body()->addWidget(history_);
    layout_->addWidget(historyCard);
    layout_->addStretch();

    connect(submitButton_, &QPushButton::clicked, this, &PaymentsPage::submitPayment);
    connect(settleButton_, &QPushButton::clicked, this, &PaymentsPage::settleBucket);
    connect(bucketBox_, &QComboBox::currentIndexChanged, this, &PaymentsPage::onBucketChanged);
    connect(exportButton_, &QPushButton::clicked, this, &PaymentsPage::exportHistory);
    if (studentBox_) {
        connect(studentBox_, &QComboBox::currentIndexChanged, this, &PaymentsPage::onStudentChanged);
    }
}

QString PaymentsPage::currentStudentId() const {
    if (!isOperator()) return qs(session_.userId);
    const int index = studentBox_ ? studentBox_->currentIndex() : -1;
    if (index < 0 || index >= studentIds_.size()) return {};
    return studentIds_.at(index);
}

st::FeeKind PaymentsPage::currentBucket() const {
    return static_cast<st::FeeKind>(bucketBox_->currentData().toInt());
}

void PaymentsPage::populateStudents() {
    if (!studentBox_) return;

    const int previousIndex = studentBox_->currentIndex();
    const QString previous = previousIndex >= 0 && previousIndex < studentIds_.size()
                                 ? studentIds_.at(previousIndex)
                                 : QString();

    studentIds_ = controllers_.students().idList();

    studentBox_->blockSignals(true);
    studentBox_->clear();
    studentBox_->addItems(controllers_.students().namesWithIds());
    const int index = static_cast<int>(studentIds_.indexOf(previous));
    studentBox_->setCurrentIndex(index >= 0 ? index : 0);
    studentBox_->blockSignals(false);
}

void PaymentsPage::refresh() {
    populateStudents();
    updateStatement();
    updateHistory();
}

void PaymentsPage::selectStudent(const QString& userId) {
    if (!studentBox_) return;
    const int index = static_cast<int>(studentIds_.indexOf(userId));
    if (index >= 0) studentBox_->setCurrentIndex(index);
    updateStatement();
    updateHistory();
}

void PaymentsPage::onStudentChanged() {
    updateStatement();
    updateHistory();
}

void PaymentsPage::onBucketChanged() { updateStatement(); }

void PaymentsPage::updateStatement() {
    const StatementView statement = controllers_.payments().statement(currentStudentId());
    if (!statement.valid) {
        studentName_->setText(QStringLiteral("No student selected"));
        studentMeta_->setText(QStringLiteral("—"));
        totalLabel_->setText(QStringLiteral("—"));
        totalCaption_->setText(QString());
        registrationLabel_->setText(QStringLiteral("—"));
        registrationCaption_->setText(QStringLiteral("REGISTRATION"));
        registrationBar_->setValue(0);
        transportLabel_->setText(QStringLiteral("—"));
        transportCaption_->setText(QStringLiteral("TRANSPORT"));
        transportBar_->setValue(0);
        submitButton_->setEnabled(false);
        settleButton_->setEnabled(false);
        exportButton_->setEnabled(false);
        return;
    }

    studentName_->setText(statement.studentName);
    studentMeta_->setText(QStringLiteral("@%1 · %2 · %3")
                              .arg(statement.username, statement.routeName, statement.seatLabel));

    totalLabel_->setText(money(statement.outstanding));
    totalLabel_->setStyleSheet(statement.outstanding > 0
                                   ? QStringLiteral("color: ") + color::danger().name()
                                   : QStringLiteral("color: ") + color::success().name());
    totalCaption_->setText(QStringLiteral("%1 · total %2")
                               .arg(statement.statusLabel, money(statement.totalCharge)));

    registrationLabel_->setText(money(statement.registration.due));
    registrationCaption_->setText(QStringLiteral("REGISTRATION   %1 of %2")
                                      .arg(money(statement.registration.paid),
                                           money(statement.registration.charge)));
    registrationBar_->setValue(statement.registration.percent);

    transportLabel_->setText(money(statement.transport.due));
    transportCaption_->setText(QStringLiteral("TRANSPORT   %1 of %2")
                                   .arg(money(statement.transport.paid),
                                        money(statement.transport.charge)));
    transportBar_->setValue(statement.transport.percent);

    const FeeRow& active = currentBucket() == st::FeeKind::Registration ? statement.registration
                                                                        : statement.transport;
    if (amount_->value() <= 0 || amount_->value() > active.due) {
        amount_->setValue(static_cast<int>(active.due));
    }

    amount_->setEnabled(active.due > 0);
    submitButton_->setEnabled(active.due > 0);
    settleButton_->setEnabled(active.due > 0);
    exportButton_->setEnabled(true);
}

void PaymentsPage::updateHistory() {
    history_->setRowCount(0);
    const QString studentId = currentStudentId();
    if (studentId.isEmpty()) {
        showEmptyState(history_, QStringLiteral("No student selected."));
        return;
    }

    int row = 0;
    for (const PaymentRow& payment : controllers_.payments().history(studentId)) {
        history_->insertRow(row);
        history_->setItem(row, 0, textItem(payment.reference));
        history_->setItem(row, 1, textItem(payment.date));
        history_->setItem(row, 2, textItem(payment.kind));
        history_->setItem(row, 3, textItem(payment.recordedBy));
        history_->setItem(row, 4, moneyItem(payment.amount));
        ++row;
    }
    showEmptyState(history_, QStringLiteral("No payments recorded for this student yet."));
}

void PaymentsPage::setMessage(const QString& text, bool isError) {
    message_->setText(text);
    message_->setStyleSheet(isError ? QStringLiteral("color: ") + color::danger().name() +
                                          QStringLiteral("; font-size: 12px;")
                                    : QStringLiteral("color: ") + color::success().name() +
                                          QStringLiteral("; font-size: 12px;"));
    message_->show();
}

void PaymentsPage::submitPayment() {
    const QString studentId = currentStudentId();
    if (studentId.isEmpty()) return;

    ReceiptView receipt;
    const ActionResult result = controllers_.payments().record(
        studentId, currentBucket(), amount_->value(), qs(session_.userId), receipt);

    if (!result.ok) {
        setMessage(result.message, true);
        return;
    }

    reference_->clear();
    setMessage(QStringLiteral("Receipt %1 · %2 applied · %3 still outstanding")
                   .arg(receipt.reference, money(receipt.applied),
                        money(receipt.outstandingAfter)),
               false);
    toast(result);
    updateStatement();
    updateHistory();
}

void PaymentsPage::settleBucket() {
    const QString studentId = currentStudentId();
    if (studentId.isEmpty()) return;

    const StatementView statement = controllers_.payments().statement(studentId);
    const FeeRow& active = currentBucket() == st::FeeKind::Registration ? statement.registration
                                                                        : statement.transport;
    if (active.due <= 0) return;

    if (!confirm(QStringLiteral("Settle in full"),
                 QStringLiteral("Record %1 against the %2 fee?").arg(money(active.due),
                                                                   active.label),
                 QStringLiteral("Record payment"))) {
        return;
    }

    ReceiptView receipt;
    const ActionResult result =
        controllers_.payments().settleBucket(studentId, currentBucket(), qs(session_.userId), receipt);

    if (!result.ok) {
        setMessage(result.message, true);
        return;
    }

    setMessage(QStringLiteral("Receipt %1 · %2 still outstanding across both fees.")
                   .arg(receipt.reference, money(statement.outstanding - receipt.applied)),
               false);
    toast(result);
    updateStatement();
    updateHistory();
}

void PaymentsPage::exportHistory() {
    const QString studentId = currentStudentId();
    if (studentId.isEmpty()) return;

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export payment history"),
        QStringLiteral("payments-%1.csv").arg(studentId), QStringLiteral("CSV files (*.csv)"));
    if (path.isEmpty()) return;

    QString message;
    BusyOverlay overlay(this, QStringLiteral("Exporting…"));
    overlay.show();
    QCoreApplication::processEvents();
    const bool ok = controllers_.payments().exportHistory(studentId, path, message);
    overlay.hide();

    if (!ok) {
        warn(QStringLiteral("Export failed"), message);
        return;
    }
    toast(message);
}

}  // namespace gui