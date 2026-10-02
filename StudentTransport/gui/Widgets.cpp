#include "Widgets.hpp"

#include "Notify.hpp"
#include "Theme.hpp"
#include "utils/TextUtils.hpp"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace gui {
namespace {

// ToastKind lives in Notify.hpp; Page::toast takes it as an int so Widgets.hpp
// does not have to expose the notification header to every page.
int resolveKind(int kind) { return kind; }

}  // namespace

QString qs(const std::string& value) { return QString::fromStdString(value); }

std::string toStd(const QString& value) { return value.toStdString(); }

QString money(long long value) { return qs(st::util::money(value)); }

QString initialsOf(const QString& name) {
    const QStringList parts = name.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.isEmpty()) return QStringLiteral("?");
    if (parts.size() == 1) return parts.first().left(2).toUpper();
    return (parts.first().left(1) + parts.last().left(1)).toUpper();
}

Page::Page(Controllers& controllers, const st::Session& session, QWidget* parent)
    : QScrollArea(parent), controllers_(controllers), session_(session) {
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    content_ = new QWidget;
    layout_ = new QVBoxLayout(content_);
    layout_->setContentsMargins(26, 22, 26, 22);
    layout_->setSpacing(16);

    setWidget(content_);
}

void Page::activate(QWidget* page) {
    if (!page) return;
    if (auto* stack = qobject_cast<QStackedWidget*>(page->parentWidget())) {
        stack->setCurrentWidget(page);
    }
}

void Page::toast(const QString& message, int kind) {
    Toast::show(this, message, static_cast<ToastKind>(resolveKind(kind)));
}

void Page::toast(const ActionResult& result) { Toast::show(this, result); }

void Page::warn(const QString& title, const QString& message) {
    QMessageBox::warning(this, title, message);
}

bool Page::confirm(const QString& title, const QString& message, const QString& acceptLabel) {
    QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::Yes | QMessageBox::No,
                    this);
    box.setDefaultButton(QMessageBox::No);
    box.setTextFormat(Qt::PlainText);

    auto* yes = box.button(QMessageBox::Yes);
    if (yes) {
        yes->setText(acceptLabel);
        yes->setObjectName("DangerButton");
    }
    auto* no = box.button(QMessageBox::No);
    if (no) {
        no->setText(QStringLiteral("Cancel"));
        no->setObjectName("SecondaryButton");
    }
    return box.exec() == QMessageBox::Yes;
}

void Page::selectRowOf(QTableWidget* table, const QString& firstColumnValue) {
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = table->item(row, 0);
        if (item && item->text() == firstColumnValue) {
            table->selectRow(row);
            return;
        }
    }
}

Card::Card(QWidget* parent) : QFrame(parent) {
    setObjectName("Card");
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(18, 16, 18, 16);
    layout_->setSpacing(12);
}

StatCard::StatCard(const QString& label, QWidget* parent) : QFrame(parent) {
    setObjectName("StatCard");
    setMinimumHeight(96);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(3);

    auto* labelWidget = new QLabel(label.toUpper(), this);
    labelWidget->setObjectName("StatLabel");

    value_ = new QLabel(QStringLiteral("0"), this);
    value_->setObjectName("StatValue");

    caption_ = new QLabel(this);
    caption_->setObjectName("StatLabel");
    caption_->hide();

    layout->addWidget(labelWidget);
    layout->addWidget(value_);
    layout->addWidget(caption_);
}

void StatCard::setValue(const QString& value) { value_->setText(value); }

void StatCard::setAccent(const QString& accent) {
    value_->setStyleSheet("color: " + accent + ";");
}

void StatCard::setCaption(const QString& caption) {
    caption_->setText(caption);
    caption_->setVisible(!caption.isEmpty());
}

PageHeader::PageHeader(const QString& title, const QString& subtitle, QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto* textColumn = new QVBoxLayout();
    textColumn->setSpacing(2);

    title_ = new QLabel(title, this);
    title_->setObjectName("PageTitle");

    subtitle_ = new QLabel(subtitle, this);
    subtitle_->setObjectName("PageSubtitle");
    subtitle_->setWordWrap(true);

    textColumn->addWidget(title_);
    textColumn->addWidget(subtitle_);

    actions_ = new QWidget(this);
    actionsLayout_ = new QHBoxLayout(actions_);
    actionsLayout_->setContentsMargins(0, 0, 0, 0);
    actionsLayout_->setSpacing(8);

    layout->addLayout(textColumn);
    layout->addStretch();
    layout->addWidget(actions_);
}

void PageHeader::addAction(QWidget* button) { actionsLayout_->addWidget(button); }

void PageHeader::setTitle(const QString& title) { title_->setText(title); }

void PageHeader::setSubtitle(const QString& subtitle) { subtitle_->setText(subtitle); }

Avatar::Avatar(const QString& name, QWidget* parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setFixedSize(34, 34);
    setStyleSheet("background: " + color::primarySoft().name() + ";"
                  "color: " + color::primary().name() + ";"
                  "border-radius: 17px;"
                  "font-weight: 700; font-size: 12px;");
    setName(name);
}

void Avatar::setName(const QString& name) { setText(initialsOf(name)); }

void configureTable(QTableWidget* table) {
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(34);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setShowGrid(false);
    table->setWordWrap(false);
    table->setSortingEnabled(false);
}

void showEmptyState(QTableWidget* table, const QString& message) {
    if (table->rowCount() > 0) return;
    table->clear();
    table->setRowCount(1);
    auto* item = new QTableWidgetItem(message);
    item->setTextAlignment(Qt::AlignCenter);
    item->setForeground(QBrush(color::textMuted()));
    item->setFlags(Qt::NoItemFlags);
    table->setItem(0, 0, item);
    if (table->columnCount() > 0) table->setSpan(0, 0, 1, table->columnCount());
}

QTableWidgetItem* textItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setToolTip(text);
    return item;
}

QTableWidgetItem* moneyItem(long long value) {
    auto* item = new QTableWidgetItem(money(value));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

QTableWidgetItem* centeredItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

QTableWidgetItem* statusItem(const QString& text, const QString& kind) {
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    if (kind == QStringLiteral("success")) {
        item->setForeground(QBrush(color::success()));
    } else if (kind == QStringLiteral("danger")) {
        item->setForeground(QBrush(color::danger()));
    } else if (kind == QStringLiteral("warning")) {
        item->setForeground(QBrush(color::warning()));
    }
    return item;
}

QLabel* badge(const QString& text, const QString& background, const QString& foreground) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("background: " + background + "; color: " + foreground +
                         "; border-radius: 9px; padding: 2px 9px; font-size: 11px;"
                         " font-weight: 700;");
    return label;
}

QWidget* rowField(const QString& label, const QString& value) {
    auto* column = new QVBoxLayout();
    column->setSpacing(1);

    auto* caption = new QLabel(label.toUpper());
    caption->setObjectName("FieldLabel");

    auto* content = new QLabel(value.isEmpty() ? QStringLiteral("—") : value);
    content->setObjectName("FieldValue");
    content->setTextInteractionFlags(Qt::TextSelectableByMouse);
    content->setWordWrap(true);

    column->addWidget(caption);
    column->addWidget(content);

    auto* holder = new QWidget;
    holder->setLayout(column);
    return holder;
}

QLabel* iconLabel(const QString& iconName, int size) {
    auto* label = new QLabel;
    label->setPixmap(icon(iconName).pixmap(size, size));
    label->setFixedSize(size + 6, size + 6);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

namespace {

QPushButton* makeButton(const QString& text, const QString& objectName,
                        const QString& iconName) {
    auto* button = new QPushButton;
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    if (iconName.isEmpty()) {
        button->setText(text);
    } else {
        button->setIcon(icon(iconName));
        button->setIconSize(QSize(16, 16));
        button->setText(text);
    }
    return button;
}

}  // namespace

QPushButton* primaryButton(const QString& text, const QString& iconName) {
    return makeButton(text, QStringLiteral("PrimaryButton"), iconName);
}

QPushButton* ghostButton(const QString& text, const QString& iconName) {
    return makeButton(text, QStringLiteral("GhostButton"), iconName);
}

QPushButton* dangerButton(const QString& text, const QString& iconName) {
    return makeButton(text, QStringLiteral("DangerButton"), iconName);
}

}  // namespace gui