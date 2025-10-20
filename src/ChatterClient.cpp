#include "ChatterClient.h"

#include <QProcessEnvironment>

namespace {
QString defaultUsername()
{
    QString username = qEnvironmentVariable("CHATTER_USERNAME");
    if (username.isEmpty()) {
        username = qEnvironmentVariable("USER");
    }
    if (username.isEmpty()) {
        username = QStringLiteral("retro");
    }
    return username;
}

QString defaultHost()
{
    QString host = qEnvironmentVariable("CHATTER_HOST");
    if (host.isEmpty()) {
        host = QStringLiteral("chat.korokorok.com");
    }
    return host;
}
}

ChatterClient::ChatterClient(QObject *parent)
    : QObject(parent)
    , m_username(defaultUsername())
    , m_host(defaultHost())
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);

    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &ChatterClient::handleReadyReadStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &ChatterClient::handleReadyReadStandardError);
    connect(&m_process, &QProcess::stateChanged,
            this, &ChatterClient::handleStateChanged);
}

void ChatterClient::setUsername(const QString &username)
{
    if (m_username == username || username.isEmpty()) {
        return;
    }
    m_username = username;
}

QString ChatterClient::username() const
{
    return m_username;
}

void ChatterClient::start()
{
    if (m_process.state() != QProcess::NotRunning) {
        return;
    }

    const QStringList commandParts = buildCommand();
    if (commandParts.isEmpty()) {
        emit errorReceived(tr("Unable to determine connection command."));
        return;
    }

    const QString program = commandParts.front();
    QStringList arguments = commandParts;
    arguments.removeFirst();

    m_process.start(program, arguments);
}

void ChatterClient::stop()
{
    if (m_process.state() == QProcess::NotRunning) {
        return;
    }
    m_process.closeWriteChannel();
    m_process.terminate();
    if (!m_process.waitForFinished(3000)) {
        m_process.kill();
    }
}

void ChatterClient::sendCommand(const QString &command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }

    if (m_process.state() == QProcess::NotRunning) {
        start();
    }

    const QByteArray data = command.toUtf8() + '\n';
    m_process.write(data);
}

void ChatterClient::handleReadyReadStandardOutput()
{
    const QString text = QString::fromUtf8(m_process.readAllStandardOutput());
    if (!text.isEmpty()) {
        emit outputReceived(text);
    }
}

void ChatterClient::handleReadyReadStandardError()
{
    const QString text = QString::fromUtf8(m_process.readAllStandardError());
    if (!text.isEmpty()) {
        emit errorReceived(text);
    }
}

void ChatterClient::handleStateChanged(QProcess::ProcessState state)
{
    emit connectionStateChanged(state != QProcess::NotRunning);
}

QStringList ChatterClient::buildCommand() const
{
    const QString overrideCommand = qEnvironmentVariable("CHATTER_FRONTEND_COMMAND");
    if (!overrideCommand.isEmpty()) {
        return overrideCommand.split(' ', Qt::SkipEmptyParts);
    }

    const QString sshProgram = QStringLiteral("ssh");
    const QString target = QStringLiteral("%1@%2").arg(m_username, m_host);
    return QStringList{sshProgram, target};
}
