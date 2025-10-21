#include "TerminalWidget.h"

#include <QEvent>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QByteArray>

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

    const int key = keyEvent->key();
    const bool isArrowKey = key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right;
    if (!isArrowKey || watched != m_display) {
        return QWidget::eventFilter(watched, event);
    }

    QByteArray sequence;
    switch (key) {
    case Qt::Key_Up:
        sequence = "\x1b[A";
        break;
    case Qt::Key_Down:
        sequence = "\x1b[B";
        break;
    case Qt::Key_Right:
        sequence = "\x1b[C";
        break;
    case Qt::Key_Left:
        sequence = "\x1b[D";
        break;
    default:
        break;
    }

    if (sequence.isEmpty()) {
        return QWidget::eventFilter(watched, event);
    }

    emit keySequenceGenerated(sequence);
    return true;
}
