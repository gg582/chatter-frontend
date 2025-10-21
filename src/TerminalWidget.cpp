#include "TerminalWidget.h"

#include <QEvent>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
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
        m_display->setContentsMargins(0, 0, 0, 0);
        m_display->setViewportMargins(0, 0, 0, 0);
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

    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (baseFont.pointSizeF() <= 0) {
        baseFont.setPointSizeF(10.0);
    }
    setTerminalFont(baseFont);
}

QTextBrowser *TerminalWidget::display() const
{
    return m_display.data();
}

QLineEdit *TerminalWidget::input() const
{
    return m_input.data();
}

void TerminalWidget::setTerminalFont(const QFont &font)
{
    if (m_display) {
        m_display->setFont(font);
        if (auto *document = m_display->document()) {
            document->setDefaultFont(font);

            QTextCursor cursor(document);
            cursor.select(QTextCursor::Document);

            QTextCharFormat format;
            format.setFontFamily(font.family());
            if (font.pointSizeF() > 0) {
                format.setFontPointSize(font.pointSizeF());
            }
            cursor.mergeCharFormat(format);
        }
    }

    if (m_input) {
        m_input->setFont(font);
    }
}

QFont TerminalWidget::terminalFont() const
{
    if (m_display) {
        return m_display->font();
    }
    return font();
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
