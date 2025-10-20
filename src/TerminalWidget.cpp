#include "TerminalWidget.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_display(new QTextBrowser(this))
    , m_input(new QLineEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (m_display) {
        m_display->setObjectName(QStringLiteral("terminalDisplay"));
        m_display->installEventFilter(this);
        m_display->setLineWrapMode(QTextEdit::NoWrap);
        if (auto *doc = m_display->document()) {
            doc->setDocumentMargin(0);
        }
        layout->addWidget(m_display);
    }

    if (m_input) {
        m_input->setObjectName(QStringLiteral("terminalInput"));
        m_input->installEventFilter(this);
        layout->addWidget(m_input);
    }
}

QTextBrowser *TerminalWidget::display() const
{
    return m_display.data();
}

QLineEdit *TerminalWidget::input() const
{
    return m_input.data();
}

bool TerminalWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (!event || event->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(watched, event);
    }

    auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (!keyEvent) {
        return QWidget::eventFilter(watched, event);
    }

    const bool isArrowKey = keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down;
    if (!isArrowKey) {
        return QWidget::eventFilter(watched, event);
    }

    if ((watched == m_display || watched == m_input) && m_display) {
        if (auto *scrollBar = m_display->verticalScrollBar()) {
            const int direction = keyEvent->key() == Qt::Key_Up ? -1 : 1;
            const int step = scrollBar->singleStep();
            scrollBar->setValue(scrollBar->value() + direction * step);
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}
