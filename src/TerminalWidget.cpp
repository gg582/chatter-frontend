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

#include <algorithm>
#include <array>
#include <cmath>

namespace {

int modifierValue(Qt::KeyboardModifiers modifiers)
{
    int value = 1;
    if (modifiers & Qt::ShiftModifier) {
        value += 1;
    }
    if (modifiers & Qt::AltModifier) {
        value += 2;
    }
    if (modifiers & Qt::ControlModifier) {
        value += 4;
    }
    return value;
}

QByteArray cursorKeySequence(Qt::KeyboardModifiers modifiers, char finalChar)
{
    const int value = modifierValue(modifiers);
    if (value == 1) {
        return QByteArray("\x1b[") + finalChar;
    }
    return QByteArray("\x1b[1;") + QByteArray::number(value) + finalChar;
}

QByteArray specialKeySequence(Qt::KeyboardModifiers modifiers, char finalChar)
{
    const int value = modifierValue(modifiers);
    if (value == 1) {
        return QByteArray("\x1b[") + finalChar;
    }
    return QByteArray("\x1b[1;") + QByteArray::number(value) + finalChar;
}

QByteArray tildeSequence(Qt::KeyboardModifiers modifiers, int code)
{
    const int value = modifierValue(modifiers);
    if (value == 1) {
        return QByteArray("\x1b[") + QByteArray::number(code) + '~';
    }
    return QByteArray("\x1b[") + QByteArray::number(code) + ';' + QByteArray::number(value) + '~';
}

QByteArray functionKeySequence(Qt::KeyboardModifiers modifiers, int index)
{
    static const char *const baseSequences[] = {
        "\x1bOP", "\x1bOQ", "\x1bOR", "\x1bOS",
        "\x1b[15~", "\x1b[17~", "\x1b[18~", "\x1b[19~",
        "\x1b[20~", "\x1b[21~", "\x1b[23~", "\x1b[24~"
    };

    if (index < 0 || index >= static_cast<int>(std::size(baseSequences))) {
        return {};
    }

    QByteArray base(baseSequences[index]);
    const int value = modifierValue(modifiers);
    if (value == 1) {
        return base;
    }

    if (base.startsWith("\x1bO")) {
        // Convert SS3 sequences to CSI form for modified keys.
        base.replace(1, 1, "[");
        base.insert(2, QByteArray("1;") + QByteArray::number(value));
    } else if (base.startsWith("\x1b[")) {
        base.insert(base.size() - 1, ';' + QByteArray::number(value));
    }
    return base;
}

}

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_display(new QTextBrowser(this))
{
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (m_display) {
        m_display->setObjectName(QStringLiteral("terminalDisplay"));
        m_display->installEventFilter(this);
        m_display->setFocusPolicy(Qt::StrongFocus);
        m_display->setLineWrapMode(QTextEdit::NoWrap);
        m_display->setContentsMargins(0, 0, 0, 0);
        m_display->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        if (auto *document = m_display->document()) {
            document->setDocumentMargin(0);
        }
        layout->addWidget(m_display);
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

    if (event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (!keyEvent) {
            return QWidget::eventFilter(watched, event);
        }

        const Qt::KeyboardModifiers mods = keyEvent->modifiers();
        if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier)) {
            keyEvent->accept();
            return true;
        }
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (handleKeyPress(keyEvent)) {
            return true;
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
    if (m_display) {
        m_display->setFocus(Qt::OtherFocusReason);
    }
    QWidget::focusInEvent(event);
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

bool TerminalWidget::handleKeyPress(QKeyEvent *event)
{
    if (!event) {
        return false;
    }

    const Qt::KeyboardModifiers mods = event->modifiers();
    const int key = event->key();

    if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier)) {
        if (key == Qt::Key_C) {
            if (m_display) {
                m_display->copy();
            }
            return true;
        }
        if (key == Qt::Key_V) {
            const QClipboard *clipboard = QGuiApplication::clipboard();
            if (clipboard) {
                const QString text = clipboard->text();
                if (!text.isEmpty()) {
                    emit bytesGenerated(text.toUtf8());
                }
            }
            return true;
        }
    }

    QByteArray data = translateKeyEvent(event);
    if (data.isEmpty()) {
        return false;
    }

    emit bytesGenerated(data);
    return true;
}

QByteArray TerminalWidget::translateKeyEvent(QKeyEvent *event) const
{
    if (!event) {
        return {};
    }

    Qt::KeyboardModifiers modifiers = event->modifiers();
    const int key = event->key();

    QByteArray output;

    if (modifiers & Qt::AltModifier) {
        output.append('\x1b');
        modifiers &= ~Qt::AltModifier;
    }

    modifiers &= ~(Qt::MetaModifier | Qt::KeypadModifier);

    if ((modifiers & Qt::ControlModifier) && !(modifiers & Qt::ShiftModifier)) {
        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            output.append(char(key - Qt::Key_A + 1));
            return output;
        }

        switch (key) {
        case Qt::Key_At:
        case Qt::Key_Space:
        case Qt::Key_2:
            output.append(char(0));
            return output;
        case Qt::Key_Leftbrace:
        case Qt::Key_BracketLeft:
            output.append(char(27));
            return output;
        case Qt::Key_Backslash:
            output.append(char(28));
            return output;
        case Qt::Key_Rightbrace:
        case Qt::Key_BracketRight:
            output.append(char(29));
            return output;
        case Qt::Key_AsciiCircum:
            output.append(char(30));
            return output;
        case Qt::Key_Underscore:
        case Qt::Key_3:
            output.append(char(31));
            return output;
        case Qt::Key_Question:
            output.append(char(127));
            return output;
        default:
            break;
        }
    }

    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        output.append('\r');
        return output;
    case Qt::Key_Backspace:
        output.append(char(0x7f));
        return output;
    case Qt::Key_Tab:
        output.append('\t');
        return output;
    case Qt::Key_Escape:
        output.append('\x1b');
        return output;
    case Qt::Key_Left:
        output.append(cursorKeySequence(modifiers, 'D'));
        return output;
    case Qt::Key_Right:
        output.append(cursorKeySequence(modifiers, 'C'));
        return output;
    case Qt::Key_Up:
        output.append(cursorKeySequence(modifiers, 'A'));
        return output;
    case Qt::Key_Down:
        output.append(cursorKeySequence(modifiers, 'B'));
        return output;
    case Qt::Key_Home:
        output.append(specialKeySequence(modifiers, 'H'));
        return output;
    case Qt::Key_End:
        output.append(specialKeySequence(modifiers, 'F'));
        return output;
    case Qt::Key_PageUp:
        output.append(tildeSequence(modifiers, 5));
        return output;
    case Qt::Key_PageDown:
        output.append(tildeSequence(modifiers, 6));
        return output;
    case Qt::Key_Insert:
        output.append(tildeSequence(modifiers, 2));
        return output;
    case Qt::Key_Delete:
        output.append(tildeSequence(modifiers, 3));
        return output;
    case Qt::Key_F1:
    case Qt::Key_F2:
    case Qt::Key_F3:
    case Qt::Key_F4:
    case Qt::Key_F5:
    case Qt::Key_F6:
    case Qt::Key_F7:
    case Qt::Key_F8:
    case Qt::Key_F9:
    case Qt::Key_F10:
    case Qt::Key_F11:
    case Qt::Key_F12:
        output.append(functionKeySequence(modifiers, key - Qt::Key_F1));
        return output;
    default:
        break;
    }

    const QString text = event->text();
    if (!text.isEmpty()) {
        output.append(text.toUtf8());
    }

    return output;
}
