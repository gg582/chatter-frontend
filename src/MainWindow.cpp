#include "MainWindow.h"

#include "ChatterClient.h"
#include "CommandCatalog.h"

#include <QAction>
#include <QApplication>
#include <QFontDatabase>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProcessEnvironment>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>

namespace {
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
{
    m_client = new ChatterClient(this);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    m_display = new QPlainTextEdit(central);
    m_display->setReadOnly(true);
    const QFont retroFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_display->setFont(retroFont);

    m_input = new QLineEdit(central);
    m_input->setPlaceholderText(tr("Type a message or pick a command from the menu"));

    layout->addWidget(m_display);
    layout->addWidget(m_input);
    setCentralWidget(central);

    m_statusLabel = new QLabel(this);
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

    QTimer::singleShot(0, this, &MainWindow::initiateConnection);
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

void MainWindow::createMenus()
{
    auto *sessionMenu = menuBar()->addMenu(tr("Session"));
    m_connectAction = sessionMenu->addAction(tr("Connect"), this, &MainWindow::initiateConnection);
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
    if (!sanitized.endsWith('\n')) {
        sanitized.append('\n');
    }

    QTextCursor cursor = m_display->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (isError) {
        QTextCharFormat format;
        format.setForeground(Qt::red);
        cursor.insertText(sanitized, format);
    } else {
        cursor.insertText(sanitized);
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

void MainWindow::applyRetroPalette()
{
    QPalette palette = qApp->palette();
    palette.setColor(QPalette::Base, QColor(8, 16, 32));
    palette.setColor(QPalette::Text, QColor(0, 255, 136));
    palette.setColor(QPalette::Window, QColor(4, 8, 16));
    palette.setColor(QPalette::WindowText, QColor(0, 255, 136));
    palette.setColor(QPalette::Highlight, QColor(0, 128, 255));
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    qApp->setPalette(palette);
}
