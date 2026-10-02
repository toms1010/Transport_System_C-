#include "AppContext.hpp"
#include "ComplaintsPage.hpp"

#include "Theme.hpp"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui {
namespace {

class ResponseDialog : public QDialog {
public:
    ResponseDialog(QWidget* parent) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Resolve complaint"));
        setModal(true);
        setMinimumWidth(480);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(22, 20, 22, 18);
        root->setSpacing(12);

        auto* heading = new QLabel(QStringLiteral("Staff response"));
        heading->setObjectName("PageTitle");
        auto* subtitle =
            new QLabel(QStringLiteral("The student sees this text along with the resolution date."));
        subtitle->setObjectName("PageSubtitle");
        subtitle->setWordWrap(true);

        response = new QPlainTextEdit;
        response->setPlaceholderText(QStringLiteral("Describe what was done…"));
        response->setMinimumHeight(140);

        auto* buttons = new QDialogButtonBox;
        auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
        auto* confirm = buttons->addButton(QStringLiteral("Mark as resolved"),
                                          QDialogButtonBox::AcceptRole);
        confirm->setObjectName("PrimaryButton");

        root->addWidget(heading);
        root->addWidget(subtitle);
        root->addWidget(response);
        root->addWidget(buttons);

        connect(confirm, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    QPlainTextEdit* response = nullptr;
};

}  // namespace

ComplaintsPage::ComplaintsPage(Controllers& controllers, const st::Session& session,
                               QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void ComplaintsPage::build() {
    auto* header = new PageHeader(
        isOperator() ? QStringLiteral("Complaint management") : QStringLiteral("My complaints"),
        isOperator()
            ? QStringLiteral("Everything students have raised, with the queue for anything open.")
            : QStringLiteral("Raise a problem and follow it through to resolution."));

    auto* filters = new QHBoxLayout;
    filters->setSpacing(10);

    statusFilter_ = new QComboBox;
    statusFilter_->setMinimumWidth(200);
    statusFilter_->addItem(QStringLiteral("All complaints"),
                           static_cast<int>(ComplaintFilter::All));
    statusFilter_->addItem(QStringLiteral("Open only"), static_cast<int>(ComplaintFilter::Open));
    statusFilter_->addItem(QStringLiteral("Resolved only"),
                           static_cast<int>(ComplaintFilter::Resolved));
    filters->addWidget(statusFilter_);

    if (isOperator()) {
        search_ = new QLineEdit;
        search_->setPlaceholderText(QStringLiteral("Search subject, student or id"));
        search_->setMinimumWidth(260);
        search_->setClearButtonEnabled(true);
        filters->addWidget(search_);
        connect(search_, &QLineEdit::textChanged, this, &ComplaintsPage::applyFilters);
    }
    filters->addStretch();

    layout_->addWidget(header);
    layout_->addLayout(filters);

    auto* card = new Card;
    table_ = new QTableWidget;
    table_->setColumnCount(isOperator() ? 5 : 4);
    if (isOperator()) {
        table_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Subject"),
                                           QStringLiteral("Student"), QStringLiteral("Date"),
                                           QStringLiteral("Status")});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    } else {
        table_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Subject"),
                                           QStringLiteral("Date"),
                                           QStringLiteral("Status")});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    }
    configureTable(table_);
    table_->setMinimumHeight(260);
    card->body()->addWidget(table_);
    layout_->addWidget(card);

    // The view builders connect to table_, so the table must exist first.
    if (isOperator()) {
        buildStaffView();
    } else {
        buildStudentView();
    }

    connect(statusFilter_, &QComboBox::currentIndexChanged, this, &ComplaintsPage::applyFilters);
}

void ComplaintsPage::buildStudentView() {
    auto* form = new Card;
    auto* formTitle = new QLabel(QStringLiteral("Submit a complaint"));
    formTitle->setObjectName("SectionTitle");
    form->body()->addWidget(formTitle);

    subject_ = new QLineEdit;
    subject_->setPlaceholderText(QStringLiteral("One line summary"));
    subject_->setMaxLength(120);

    description_ = new QPlainTextEdit;
    description_->setPlaceholderText(QStringLiteral("What happened, and when?"));
    description_->setMinimumHeight(90);

    formError_ = new QLabel;
    formError_->setWordWrap(true);
    formError_->setStyleSheet("color: " + color::danger().name() + "; font-size: 12px;");
    formError_->hide();

    auto* submit = primaryButton(QStringLiteral("Submit complaint"));

    form->body()->addWidget(subject_);
    form->body()->addSpacing(6);
    form->body()->addWidget(description_);
    form->body()->addWidget(formError_);
    form->body()->addWidget(submit);
    layout_->addWidget(form);
    layout_->addStretch();

    connect(submit, &QPushButton::clicked, this, &ComplaintsPage::submitComplaint);
}

void ComplaintsPage::buildStaffView() {
    auto* actions = new QHBoxLayout;
    actions->setSpacing(8);
    resolveButton_ = primaryButton(QStringLiteral("Resolve"));
    reopenButton_ = ghostButton(QStringLiteral("Reopen"), QStringLiteral("refresh"));
    for (QPushButton* button : {resolveButton_, reopenButton_}) {
        actions->addWidget(button);
    }
    actions->addStretch();
    layout_->addLayout(actions);

    auto* detailCard = new Card;
    auto* detailTitle = new QLabel(QStringLiteral("Complaint detail"));
    detailTitle->setObjectName("SectionTitle");
    detail_ = new QLabel(QStringLiteral("Select a complaint to read the full conversation."));
    detail_->setObjectName("Muted");
    detail_->setWordWrap(true);
    detailCard->body()->addWidget(detailTitle);
    detailCard->body()->addWidget(detail_);
    layout_->addWidget(detailCard);
    layout_->addStretch();

    connect(resolveButton_, &QPushButton::clicked, this, &ComplaintsPage::resolveSelected);
    connect(reopenButton_, &QPushButton::clicked, this, &ComplaintsPage::reopenSelected);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &ComplaintsPage::showDetail);
    connect(table_, &QTableWidget::itemDoubleClicked, this, &ComplaintsPage::showDetail);
}

void ComplaintsPage::refresh() { applyFilters(); }

QString ComplaintsPage::selectedComplaintId() const {
    const int row = table_->currentRow();
    if (row < 0) return {};
    QTableWidgetItem* item = table_->item(row, 0);
    return item ? item->text() : QString();
}

void ComplaintsPage::applyFilters() {
    const auto filter =
        static_cast<ComplaintFilter>(statusFilter_->currentData().toInt());
    const QString search = search_ ? search_->text().trimmed() : QString();

    const QVector<ComplaintRow> rows =
        isOperator() ? controllers_.complaints().list(filter, search)
                     : controllers_.complaints().forStudent(qs(session_.userId));

    table_->setRowCount(0);
    int row = 0;
    int column = 0;
    for (const ComplaintRow& complaint : rows) {
        table_->insertRow(row);
        column = 0;
        table_->setItem(row, column++, textItem(complaint.id));
        table_->setItem(row, column++, textItem(complaint.subject));
        if (isOperator()) table_->setItem(row, column++, textItem(complaint.studentName));
        table_->setItem(row, column++, textItem(complaint.createdAt));
        table_->setItem(row, column, statusItem(complaint.statusLabel, complaint.statusKind));
        ++row;
    }

    showEmptyState(table_, isOperator() ? QStringLiteral("No complaints match this filter.")
                                        : QStringLiteral("You have not raised any complaints."));
    showDetail();
}

void ComplaintsPage::showDetail() {
    if (!isOperator() || !detail_) return;

    const QString id = selectedComplaintId();
    ComplaintRow complaint;
    if (id.isEmpty() || !controllers_.complaints().findRow(id, complaint)) {
        detail_->setText(QStringLiteral("Select a complaint to read the full conversation."));
        resolveButton_->setEnabled(false);
        reopenButton_->setEnabled(false);
        return;
    }

    QString text = QStringLiteral("%1 — %2\n\n%3\n\nRaised by %4 on %5")
                       .arg(complaint.subject, complaint.id, complaint.description,
                            complaint.studentName, complaint.createdAt);

    if (complaint.resolved) {
        text += QStringLiteral("\n\nSTAFF RESPONSE (%1, %2)\n%3")
                    .arg(complaint.resolvedBy, complaint.resolvedAt, complaint.response);
    }

    detail_->setText(text);
    resolveButton_->setEnabled(!complaint.resolved);
    reopenButton_->setEnabled(complaint.resolved);
}

void ComplaintsPage::submitComplaint() {
    QString createdId;
    const ActionResult result = controllers_.complaints().submit(
        qs(session_.userId), subject_->text().trimmed(), description_->toPlainText().trimmed(),
        createdId);

    if (!result.ok) {
        formError_->setText(result.message);
        formError_->show();
        return;
    }

    subject_->clear();
    description_->clear();
    formError_->hide();
    toast(result);
    applyFilters();
}

void ComplaintsPage::resolveSelected() {
    const QString id = selectedComplaintId();
    if (id.isEmpty()) {
        warn(QStringLiteral("No complaint selected"),
             QStringLiteral("Select a complaint in the table first."));
        return;
    }

    ResponseDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    toast(controllers_.complaints().resolve(id, qs(session_.displayName),
                                            dialog.response->toPlainText().trimmed()));
    applyFilters();
}

void ComplaintsPage::reopenSelected() {
    const QString id = selectedComplaintId();
    if (id.isEmpty()) {
        warn(QStringLiteral("No complaint selected"),
             QStringLiteral("Select a complaint in the table first."));
        return;
    }

    if (!confirm(QStringLiteral("Reopen complaint"),
                 QStringLiteral("Reopen %1? The student will see it as open again.")
                     .arg(id))) {
        return;
    }

    toast(controllers_.complaints().reopen(id, qs(session_.displayName)));
    applyFilters();
}

}  // namespace gui
