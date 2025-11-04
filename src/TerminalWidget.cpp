#include "TerminalWidget.h"

#include <QByteArray>
#include <QClipboard>
#include <QEvent>
#include <QFocusEvent>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QLineEdit>

#include <algorithm>
#include <cmath>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_display(new QTextBrowser(this))
    , m_entry(new QLineEdit(this))
{
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (m_display) {
        m_display->setObjectName(QStringLiteral("terminalDisplay"));
        m_display->installEventFilter(this);
        m_display->setFocusPolicy(Qt::ClickFocus);
        m_display->setLineWrapMode(QTextEdit::NoWrap);
        m_display->setContentsMargins(0, 0, 0, 0);
        m_display->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        if (auto *document = m_display->document()) {
            document->setDocumentMargin(0);
        }
        layout->addWidget(m_display);
    }

    if (m_entry) {
        m_entry->setObjectName(QStringLiteral("terminalEntry"));
        m_entry->setClearButtonEnabled(true);
        m_entry->setPlaceholderText(tr("Type a command and press Enter"));
        connect(m_entry, &QLineEdit::returnPressed, this, &TerminalWidget::submitEntryText);
        layout->addWidget(m_entry);
    }

    if (m_entry) {
        setFocusProxy(m_entry);
    } else if (m_display) {
        setFocusProxy(m_display);
    }

    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (baseFont.pointSizeF() <= 0) {
        baseFont.setPointSizeF(10.0);
    }
    setTerminalFont(baseFont);

    scheduleTerminalSizeUpdate();
}

QTextBrowser *TerminalWidget::display() const
{
    return m_display.data();
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

    if (m_entry) {
        m_entry->setFont(font);
    }

    scheduleTerminalSizeUpdate();
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
    if (!m_display || watched != m_display) {
        return QWidget::eventFilter(watched, event);
    }

    if (!event) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (!keyEvent) {
            return QWidget::eventFilter(watched, event);
        }

        const Qt::KeyboardModifiers mods = keyEvent->modifiers();
        if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier)) {
            const int key = keyEvent->key();
            if (key == Qt::Key_C || key == Qt::Key_V) {
                if (event->type() == QEvent::ShortcutOverride) {
                    keyEvent->accept();
                    return true;
                }

                if (key == Qt::Key_C) {
                    if (m_display) {
                        m_display->copy();
                    }
                } else if (key == Qt::Key_V) {
                    if (m_entry) {
                        const QClipboard *clipboard = QGuiApplication::clipboard();
                        if (clipboard) {
                            m_entry->insert(clipboard->text());
                            m_entry->setFocus(Qt::OtherFocusReason);
                        }
                    }
                }
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TerminalWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    scheduleTerminalSizeUpdate();
}

void TerminalWidget::focusInEvent(QFocusEvent *event)
{
    if (m_entry) {
        m_entry->setFocus(Qt::OtherFocusReason);
    }
    QWidget::focusInEvent(event);
}

void TerminalWidget::submitEntryText()
{
    if (!m_entry) {
        return;
    }

    const QString text = m_entry->text();

    QByteArray data = text.toUtf8();
    data.append('\r');

    emit bytesGenerated(data);

    m_entry->clear();
}

void TerminalWidget::scheduleTerminalSizeUpdate()
{
    if (m_pendingSizeUpdate) {
        return;
    }

    m_pendingSizeUpdate = true;
    QTimer::singleShot(0, this, [this]() {
        m_pendingSizeUpdate = false;
        emitTerminalSize();
    });
}

void TerminalWidget::emitTerminalSize()
{
    if (!m_display) {
        return;
    }

    QWidget *viewport = m_display->viewport();
    if (!viewport) {
        return;
    }

    const QSize viewportSize = viewport->size();
    if (viewportSize.width() <= 0 || viewportSize.height() <= 0) {
        return;
    }

    const QFontMetricsF metrics(m_display->font());
    const qreal charWidth = std::max<qreal>(1.0, metrics.horizontalAdvance(QLatin1Char('M')));
    const qreal charHeight = std::max<qreal>(1.0, metrics.lineSpacing());

    const int columns = std::max(1, static_cast<int>(std::floor(viewportSize.width() / charWidth)));
    const int rows = std::max(1, static_cast<int>(std::floor(viewportSize.height() / charHeight)));

    emit terminalSizeChanged(columns, rows);
}

