#include "MainWindow.h"

#include "ChatterClient.h"
#include "CommandCatalog.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QList>
#include <QPalette>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextOption>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>

namespace {

struct FormattedFragment {
    QString text;
    QTextCharFormat format;
};

QColor basicAnsiColor(int index, bool bright)
{
    static const QColor normal[] = {
        QColor(0, 0, 0),       // black
        QColor(170, 0, 0),     // red
        QColor(0, 170, 0),     // green
        QColor(170, 85, 0),    // yellow
        QColor(0, 0, 170),     // blue
        QColor(170, 0, 170),   // magenta
        QColor(0, 170, 170),   // cyan
        QColor(170, 170, 170)  // white
    };
    static const QColor brightColors[] = {
        QColor(85, 85, 85),    // bright black / gray
        QColor(255, 85, 85),   // bright red
        QColor(85, 255, 85),   // bright green
        QColor(255, 255, 85),  // bright yellow
        QColor(85, 85, 255),   // bright blue
        QColor(255, 85, 255),  // bright magenta
        QColor(85, 255, 255),  // bright cyan
        QColor(255, 255, 255)  // bright white
    };

    index = qBound(0, index, 7);
    return bright ? brightColors[index] : normal[index];
}

QColor colorFrom256Palette(int index)
{
    if (index < 0) {
        index = 0;
    }
    if (index > 255) {
        index = 255;
    }

    if (index < 16) {
        const bool bright = index >= 8;
        return basicAnsiColor(index % 8, bright);
    }

    if (index < 232) {
        const int base = index - 16;
        const int r = base / 36;
        const int g = (base / 6) % 6;
        const int b = base % 6;
        auto component = [](int value) {
            if (value == 0) {
                return 0;
            }
            return 55 + (value * 40);
        };
        return QColor(component(r), component(g), component(b));
    }

    const int gray = 8 + ((index - 232) * 10);
    return QColor(gray, gray, gray);
}

void applyExtendedColor(QTextCharFormat &format,
                        bool isForeground,
                        const QTextCharFormat &baseFormat,
                        int mode,
                        const QList<int> &params,
                        int &i)
{
    if (mode == 5) {
        if (i + 1 >= params.size()) {
            return;
        }
        const QColor color = colorFrom256Palette(params.at(++i));
        if (isForeground) {
            format.setForeground(color);
        } else {
            format.setBackground(color);
        }
    } else if (mode == 2) {
        if (i + 3 >= params.size()) {
            return;
        }
        const int r = params.at(++i);
        const int g = params.at(++i);
        const int b = params.at(++i);
        const QColor color(r, g, b);
        if (!color.isValid()) {
            return;
        }
        if (isForeground) {
            format.setForeground(color);
        } else {
            format.setBackground(color);
        }
    } else {
        if (isForeground) {
            format.setForeground(baseFormat.foreground());
        } else {
            format.setBackground(baseFormat.background());
        }
    }
}

void applySgr(const QList<int> &params,
              QTextCharFormat &currentFormat,
              const QTextCharFormat &baseFormat)
{
    if (params.isEmpty()) {
        currentFormat = baseFormat;
        return;
    }

    for (int i = 0; i < params.size(); ++i) {
        const int code = params.at(i);
        switch (code) {
        case 0:
            currentFormat = baseFormat;
            break;
        case 1:
            currentFormat.setFontWeight(QFont::Bold);
            break;
        case 3:
            currentFormat.setFontItalic(true);
            break;
        case 4:
            currentFormat.setFontUnderline(true);
            break;
        case 22:
            currentFormat.setFontWeight(baseFormat.fontWeight());
            break;
        case 23:
            currentFormat.setFontItalic(baseFormat.fontItalic());
            break;
        case 24:
            currentFormat.setFontUnderline(baseFormat.fontUnderline());
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            currentFormat.setForeground(basicAnsiColor(code - 30, false));
            break;
        case 39:
            currentFormat.setForeground(baseFormat.foreground());
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            currentFormat.setBackground(basicAnsiColor(code - 40, false));
            break;
        case 49:
            currentFormat.setBackground(baseFormat.background());
            break;
        case 90: case 91: case 92: case 93:
        case 94: case 95: case 96: case 97:
            currentFormat.setForeground(basicAnsiColor(code - 90, true));
            break;
        case 100: case 101: case 102: case 103:
        case 104: case 105: case 106: case 107:
            currentFormat.setBackground(basicAnsiColor(code - 100, true));
            break;
        case 38:
        case 48:
            if (i + 1 < params.size()) {
                const bool isForeground = (code == 38);
                const int mode = params.at(++i);
                applyExtendedColor(currentFormat, isForeground, baseFormat, mode, params, i);
            }
            break;
        default:
            break;
        }
    }
}

void insertFragmentWithLinks(QTextCursor &cursor,
                             const QString &text,
                             const QTextCharFormat &format)
{
    if (text.isEmpty()) {
        return;
    }

    static const QRegularExpression urlRegex(
        QStringLiteral(R"((https?://[^\s<>"]+))"));

    int lastIndex = 0;
    auto it = urlRegex.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const int start = match.capturedStart();
        if (start > lastIndex) {
            cursor.insertText(text.mid(lastIndex, start - lastIndex), format);
        }

        const QString url = match.captured();
        QTextCharFormat linkFormat = format;
        linkFormat.setAnchor(true);
        linkFormat.setAnchorHref(url);
        linkFormat.setFontUnderline(true);
        linkFormat.setForeground(QBrush(qApp->palette().color(QPalette::Link)));
        cursor.insertText(url, linkFormat);
        lastIndex = match.capturedEnd();
    }

    if (lastIndex < text.size()) {
        cursor.insertText(text.mid(lastIndex), format);
    }
}

QVector<FormattedFragment> parseAnsiText(const QString &input,
                                         const QTextCharFormat &baseFormat)
{
    QVector<FormattedFragment> fragments;
    QTextCharFormat currentFormat = baseFormat;
    QString buffer;

    auto flushBuffer = [&]() {
        if (buffer.isEmpty()) {
            return;
        }
        fragments.append({buffer, currentFormat});
        buffer.clear();
    };

    for (int i = 0; i < input.size(); ++i) {
        const QChar ch = input.at(i);
        if (ch == QLatin1Char('\x1b')) {
            flushBuffer();
            if (i + 1 >= input.size()) {
                continue;
            }
            if (input.at(i + 1) != QLatin1Char('[')) {
                continue;
            }

            int j = i + 2;
            QString number;
            QList<int> params;
            for (; j < input.size(); ++j) {
                const QChar c = input.at(j);
                if (c.isDigit()) {
                    number.append(c);
                    continue;
                }
                if (c == QLatin1Char(';')) {
                    params.append(number.isEmpty() ? 0 : number.toInt());
                    number.clear();
                    continue;
                }
                if (c == QLatin1Char('?')) {
                    continue;
                }
                if (!number.isEmpty() || params.isEmpty()) {
                    params.append(number.isEmpty() ? 0 : number.toInt());
                    number.clear();
                }
                if (c == QLatin1Char('m')) {
                    applySgr(params, currentFormat, baseFormat);
                }
                break;
            }
            i = j;
            continue;
        }

        if (ch == QLatin1Char('\r')) {
            if (i + 1 < input.size() && input.at(i + 1) == QLatin1Char('\n')) {
                continue;
            }
            buffer.append(QLatin1Char('\n'));
            flushBuffer();
            continue;
        }

        if (ch == QLatin1Char('\b')) {
            if (!buffer.isEmpty()) {
                buffer.chop(1);
            }
            continue;
        }

        if (ch == QLatin1Char('\a')) {
            continue;
        }

        buffer.append(ch);
    }

    flushBuffer();
    return fragments;
}

QString formatCommand(const CommandDescriptor &descriptor, const QString &argument)
{
    if (CommandCatalog::requiresUserInput(descriptor)) {
        return descriptor.command.arg(argument);
    }
    return descriptor.command;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_connectAction(nullptr)
    , m_disconnectAction(nullptr)
    , m_isConnected(false)
    , m_nicknameConfirmed(false)
{
    m_client = new ChatterClient(this);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    const QFont retroFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    setFont(retroFont);

    m_display = new QTextBrowser(central);
    m_display->setReadOnly(true);
    m_display->setFont(retroFont);
    m_display->setOpenLinks(true);
    m_display->setOpenExternalLinks(true);
    m_display->setLineWrapMode(QTextEdit::NoWrap);
    m_display->setWordWrapMode(QTextOption::NoWrap);
    m_display->setUndoRedoEnabled(false);

    m_input = new QLineEdit(central);
    m_input->setPlaceholderText(tr("Type a message or pick a command from the menu"));
    m_input->setFont(retroFont);

    layout->addWidget(m_display);
    layout->addWidget(m_input);
    setCentralWidget(central);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setFont(retroFont);
    statusBar()->addWidget(m_statusLabel);

    createMenus();
    applyRetroPalette();

    connect(m_input.data(), &QLineEdit::returnPressed,
            this, &MainWindow::handleSendRequested);
    connect(m_client.data(), &ChatterClient::outputReceived,
            this, &MainWindow::handleClientOutput);
    connect(m_client.data(), &ChatterClient::errorReceived,
            this, &MainWindow::handleClientError);
    connect(m_client.data(), &ChatterClient::connectionStateChanged,
            this, &MainWindow::handleConnectionStateChanged);

    QTimer::singleShot(0, this, [this]() {
        if (ensureNickname(true)) {
            initiateConnection();
        } else if (m_statusLabel) {
            m_statusLabel->setText(tr("Set a nickname to connect"));
        }
    });
}

void MainWindow::handleSendRequested()
{
    if (!m_client) {
        return;
    }

    const QString text = m_input->text();
    if (text.trimmed().isEmpty()) {
        return;
    }

    m_client->sendCommand(text);
    m_input->clear();
}

void MainWindow::handleClientOutput(const QString &text)
{
    appendMessage(text);
}

void MainWindow::handleClientError(const QString &text)
{
    appendMessage(text, true);
}

void MainWindow::handleConnectionStateChanged(bool connected)
{
    if (connected) {
        m_statusLabel->setText(tr("Connected as %1").arg(m_client->username()));
    } else {
        m_statusLabel->setText(tr("Disconnected"));
    }
    m_isConnected = connected;
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);
}

void MainWindow::handleCommandActionTriggered()
{
    if (!m_client) {
        return;
    }

    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    const QVariant data = action->data();
    if (!data.canConvert<CommandDescriptor>()) {
        return;
    }

    const CommandDescriptor descriptor = data.value<CommandDescriptor>();
    QString argument;
    if (CommandCatalog::requiresUserInput(descriptor)) {
        argument = promptForArgument(descriptor.placeholderHint);
        if (argument.isEmpty()) {
            return;
        }
    }

    m_client->sendCommand(formatCommand(descriptor, argument));
}

void MainWindow::initiateConnection()
{
    if (qEnvironmentVariableIsSet("CHATTER_FRONTEND_DISABLE_AUTOSTART")) {
        return;
    }
    if (!ensureNickname()) {
        if (m_statusLabel) {
            m_statusLabel->setText(tr("Set a nickname to connect"));
        }
        return;
    }
    if (m_client) {
        m_client->start();
    }
}

void MainWindow::stopConnection()
{
    if (m_client) {
        m_client->stop();
    }
}

void MainWindow::changeNickname()
{
    const QString previous = m_client ? m_client->username() : QString();
    if (!ensureNickname(true)) {
        return;
    }

    if (!m_client) {
        return;
    }

    const QString updated = m_client->username();
    if (updated == previous) {
        return;
    }

    if (m_statusLabel && !m_isConnected) {
        m_statusLabel->setText(tr("Ready as %1").arg(updated));
    }

    if (m_isConnected) {
        stopConnection();
        QTimer::singleShot(0, this, &MainWindow::initiateConnection);
    }
}

void MainWindow::createMenus()
{
    auto *sessionMenu = menuBar()->addMenu(tr("Session"));
    m_connectAction = sessionMenu->addAction(tr("Connect"), this, &MainWindow::initiateConnection);
    sessionMenu->addAction(tr("Set Nickname..."), this, &MainWindow::changeNickname);
    m_disconnectAction = sessionMenu->addAction(tr("Disconnect"), this, &MainWindow::stopConnection);
    m_disconnectAction->setEnabled(false);

    auto *commandsMenu = menuBar()->addMenu(tr("Commands"));
    populateCommandMenu(commandsMenu);
}

void MainWindow::populateCommandMenu(QMenu *menu)
{
    const auto descriptors = CommandCatalog::allCommands();
    for (const auto &descriptor : descriptors) {
        QAction *action = menu->addAction(descriptor.label);
        action->setData(QVariant::fromValue(descriptor));
        connect(action, &QAction::triggered, this, &MainWindow::handleCommandActionTriggered);
    }
}

void MainWindow::appendMessage(const QString &text, bool isError)
{
    if (!m_display) {
        return;
    }

    QString sanitized = text;
    if (!sanitized.isEmpty() && !sanitized.endsWith('\n')) {
        sanitized.append('\n');
    }

    QTextCharFormat baseFormat;
    baseFormat.setForeground(isError ? QBrush(Qt::red)
                                    : QBrush(m_display->palette().color(QPalette::Text)));
    baseFormat.setBackground(QBrush(m_display->palette().color(QPalette::Base)));
    baseFormat.setFont(m_display->font());
    baseFormat.setFontWeight(QFont::Normal);
    baseFormat.setFontItalic(false);
    baseFormat.setFontUnderline(false);

    const QVector<FormattedFragment> fragments = parseAnsiText(sanitized, baseFormat);

    QTextCursor cursor = m_display->textCursor();
    cursor.movePosition(QTextCursor::End);
    for (const auto &fragment : fragments) {
        insertFragmentWithLinks(cursor, fragment.text, fragment.format);
    }
    m_display->setTextCursor(cursor);
    m_display->ensureCursorVisible();
}

QString MainWindow::promptForArgument(const QString &hint) const
{
    bool ok = false;
    const QString value = QInputDialog::getText(const_cast<MainWindow *>(this),
                                                tr("Command Argument"),
                                                hint,
                                                QLineEdit::Normal,
                                                QString(),
                                                &ok);
    if (ok) {
        return value;
    }
    return QString();
}

bool MainWindow::ensureNickname(bool forcePrompt)
{
    if (!m_client) {
        return false;
    }

    if (!forcePrompt && m_nicknameConfirmed && !m_client->username().trimmed().isEmpty()) {
        return true;
    }

    QString current = m_client->username().trimmed();
    bool ok = false;

    while (true) {
        const QString nickname = QInputDialog::getText(
            this,
            tr("Choose your nickname"),
            tr("Nickname:"),
            QLineEdit::Normal,
            current,
            &ok)
                                    .trimmed();

        if (!ok) {
            return false;
        }

        if (nickname.isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Nickname Required"),
                                 tr("Please enter a nickname to continue."));
            continue;
        }

        m_client->setUsername(nickname);
        m_nicknameConfirmed = true;
        if (m_statusLabel && !m_isConnected) {
            m_statusLabel->setText(tr("Ready as %1").arg(nickname));
        }
        return true;
    }
}

void MainWindow::applyRetroPalette()
{
    QPalette palette = qApp->palette();
    palette.setColor(QPalette::Base, QColor(8, 16, 32));
    palette.setColor(QPalette::Text, QColor(0, 255, 136));
    palette.setColor(QPalette::Window, QColor(4, 8, 16));
    palette.setColor(QPalette::WindowText, QColor(0, 255, 136));
    palette.setColor(QPalette::Highlight, QColor(0, 128, 255));
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    palette.setColor(QPalette::Link, QColor(0, 200, 255));
    palette.setColor(QPalette::LinkVisited, QColor(0, 160, 224));
    qApp->setPalette(palette);
}
