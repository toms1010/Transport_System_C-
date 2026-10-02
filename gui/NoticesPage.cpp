#include "AppContext.hpp"
#include "NoticesPage.hpp"

#include "Theme.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
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

class NoticeEditor : public QDialog {
public:
    NoticeEditor(const QString& title, const QString& content, QWidget* parent) : QDialog(parent) {
        const QString dialogTitle = title.isEmpty() ? QStringLiteral("Create notice")
                                                     : QStringLiteral("Edit notice");
        setWindowTitle(dialogTitle);
        setModal(true);
        setMinimumWidth(540);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(24, 22, 24, 18);
        root->setSpacing(12);

        auto* heading = new QLabel(dialogTitle);
        heading->setObjectName("PageTitle");
        auto* subtitle =
            new QLabel(QStringLiteral("Every signed-in user sees this on their dashboard."));
        subtitle->setObjectName("PageSubtitle");
        subtitle->setWordWrap(true);

        auto* form = new QFormLayout;
        form->setSpacing(10);

        title_ = new QLineEdit(title);
        title_->setPlaceholderText(QStringLiteral("e.g. Route 4 timing change"));
        title_->setMaxLength(120);

        content_ = new QPlainTextEdit(content);
        content_->setPlaceholderText(QStringLiteral("Write the notice…"));
        content_->setMinimumHeight(180);

        form->addRow(QStringLiteral("Title"), title_);
        form->addRow(QStringLiteral("Notice"), content_);

        auto* buttons = new QDialogButtonBox;
        auto* cancel = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
        auto* save = buttons->addButton(title.isEmpty() ? QStringLiteral("Publish")
                                                        : QStringLiteral("Save"),
                                        QDialogButtonBox::AcceptRole);
        save->setObjectName("PrimaryButton");

        root->addWidget(heading);
        root->addWidget(subtitle);
        root->addLayout(form);
        root->addWidget(buttons);

        connect(save, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    QString title() const { return title_->text().trimmed(); }
    QString content() const { return content_->toPlainText().trimmed(); }

private:
    QLineEdit* title_ = nullptr;
    QPlainTextEdit* content_ = nullptr;
};

}  // namespace

NoticesPage::NoticesPage(Controllers& controllers, const st::Session& session, QWidget* parent)
    : Page(controllers, session, parent) {
    build();
}

void NoticesPage::build() {
    auto* header = new PageHeader(
        QStringLiteral("Notice board"),
        isOperator() ? QStringLiteral("Publish announcements for students and staff.")
                     : QStringLiteral("Announcements from the transport office."));

    if (isOperator()) {
        auto* publishButton = primaryButton(QStringLiteral("New notice"),
                                            QStringLiteral("add"));
        header->addAction(publishButton);
        connect(publishButton, &QPushButton::clicked, this, &NoticesPage::publish);
    }
    layout_->addWidget(header);

    auto* card = new Card;
    table_ = new QTableWidget;
    table_->setColumnCount(4);
    table_->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Title"),
                                       QStringLiteral("Published by"),
                                       QStringLiteral("Posted")});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    configureTable(table_);
    table_->setMinimumHeight(240);
    card->body()->addWidget(table_);
    layout_->addWidget(card);

    auto* detailCard = new Card;
    auto* detailTitle = new QLabel(QStringLiteral("Notice"));
    detailTitle->setObjectName("SectionTitle");
    detail_ = new QLabel(QStringLiteral("Select a notice to read it."));
    detail_->setWordWrap(true);
    detailCard->body()->addWidget(detailTitle);
    detailCard->body()->addWidget(detail_);
    layout_->addWidget(detailCard);

    if (isOperator()) {
        auto* actions = new QHBoxLayout;
        actions->setSpacing(8);
        editButton_ = ghostButton(QStringLiteral("Edit"), QStringLiteral("edit"));
        deleteButton_ = dangerButton(QStringLiteral("Delete"), QStringLiteral("delete"));
        for (QPushButton* button : {editButton_, deleteButton_}) {
            actions->addWidget(button);
        }
        actions->addStretch();
        layout_->addLayout(actions);

        connect(editButton_, &QPushButton::clicked, this, &NoticesPage::editSelected);
        connect(deleteButton_, &QPushButton::clicked, this, &NoticesPage::deleteSelected);
        connect(table_, &QTableWidget::itemDoubleClicked, this, [this] {
            if (isOperator()) editSelected();
        });
    }

    connect(table_, &QTableWidget::itemSelectionChanged, this, &NoticesPage::showSelected);
    layout_->addStretch();
}

QString NoticesPage::selectedNoticeId() const {
    const int row = table_->currentRow();
    if (row < 0) return {};
    QTableWidgetItem* item = table_->item(row, 0);
    return item ? item->text() : QString();
}

void NoticesPage::refresh() {
    table_->setRowCount(0);
    int row = 0;
    for (const NoticeRow& notice : controllers_.notices().rows()) {
        table_->insertRow(row);
        table_->setItem(row, 0, textItem(notice.id));
        table_->setItem(row, 1, textItem(notice.title));
        table_->setItem(row, 2, textItem(notice.authorName));
        table_->setItem(row, 3, textItem(notice.createdAt));
        ++row;
    }
    showEmptyState(table_, QStringLiteral("Nothing has been published yet."));
    showSelected();
}

void NoticesPage::showSelected() {
    const QString id = selectedNoticeId();
    NoticeRow notice;
    if (id.isEmpty() || !controllers_.notices().findRow(id, notice)) {
        detail_->setText(QStringLiteral("Select a notice to read it."));
        if (editButton_) editButton_->setEnabled(false);
        if (deleteButton_) deleteButton_->setEnabled(false);
        return;
    }

    detail_->setText(QStringLiteral("%1\n\nPublished %2 by %3\n\n%4")
                         .arg(notice.title, notice.createdAt, notice.authorName, notice.content));
    if (editButton_) editButton_->setEnabled(true);
    if (deleteButton_) deleteButton_->setEnabled(true);
}

void NoticesPage::publish() {
    NoticeEditor editor(QString(), QString(), this);
    if (editor.exec() != QDialog::Accepted) return;
    toast(controllers_.notices().publish(editor.title(), editor.content(), qs(session_.userId)));
    refresh();
}

void NoticesPage::editSelected() {
    const QString id = selectedNoticeId();
    NoticeRow notice;
    if (id.isEmpty() || !controllers_.notices().findRow(id, notice)) {
        warn(QStringLiteral("No notice selected"),
             QStringLiteral("Select a notice in the table first."));
        return;
    }

    NoticeEditor editor(notice.title, notice.content, this);
    if (editor.exec() != QDialog::Accepted) return;
    toast(controllers_.notices().edit(id, editor.title(), editor.content()));
    refresh();
}

void NoticesPage::deleteSelected() {
    const QString id = selectedNoticeId();
    NoticeRow notice;
    if (id.isEmpty() || !controllers_.notices().findRow(id, notice)) {
        warn(QStringLiteral("No notice selected"),
             QStringLiteral("Select a notice in the table first."));
        return;
    }

    if (!confirm(QStringLiteral("Delete notice"),
                 QStringLiteral("Delete “%1”? Everyone will stop seeing it.").arg(notice.title),
                 QStringLiteral("Delete"))) {
        return;
    }

    toast(controllers_.notices().remove(id));
    refresh();
}

}  // namespace gui
