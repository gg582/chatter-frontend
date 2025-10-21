#include "ChatterClient.h"

#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <QtGlobal>

#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

namespace {

constexpr int kDefaultColumns = 80;
constexpr int kDefaultRows = 24;

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

QByteArray escapeErrorMessage(const QByteArray &message)
{
    QByteArray sanitized = message;
    sanitized.replace('\n', ' ');
    sanitized.replace('\r', ' ');
    return sanitized;
}

} // namespace

ChatterClient::ChatterClient(QObject *parent)
    : QObject(parent)
    , m_masterFd(-1)
    , m_childPid(-1)
    , m_readNotifier(nullptr)
    , m_username(defaultUsername())
    , m_host(defaultHost())
    , m_columns(kDefaultColumns)
    , m_rows(kDefaultRows)
    , m_connected(false)
{
}

ChatterClient::~ChatterClient()
{
    stop();
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
    if (isRunning()) {
        return;
    }

    const QStringList commandParts = buildCommand();
    if (commandParts.isEmpty()) {
        emit errorReceived(tr("Unable to determine connection command."));
        return;
    }

    struct winsize windowSize;
    windowSize.ws_col = m_columns > 0 ? static_cast<unsigned short>(m_columns) : kDefaultColumns;
    windowSize.ws_row = m_rows > 0 ? static_cast<unsigned short>(m_rows) : kDefaultRows;
    windowSize.ws_xpixel = 0;
    windowSize.ws_ypixel = 0;

    int masterFd = -1;
    pid_t childPid = ::forkpty(&masterFd, nullptr, nullptr, &windowSize);
    if (childPid < 0) {
        const QByteArray message = escapeErrorMessage(QByteArray(::strerror(errno)));
        emit errorReceived(tr("Failed to start terminal session: %1").arg(QString::fromLatin1(message)));
        return;
    }

    if (childPid == 0) {
        ::signal(SIGINT, SIG_DFL);
        ::signal(SIGTERM, SIG_DFL);
        ::signal(SIGHUP, SIG_DFL);
        ::setenv("TERM", "xterm-256color", 1);

        std::vector<QByteArray> argumentData;
        argumentData.reserve(commandParts.size());
        std::vector<char *> argv;
        argv.reserve(commandParts.size() + 1);

        for (const QString &part : commandParts) {
            argumentData.push_back(part.toLocal8Bit());
        }

        for (QByteArray &part : argumentData) {
            argv.push_back(part.data());
        }
        argv.push_back(nullptr);

        if (!argv.empty()) {
            ::execvp(argv.front(), argv.data());
        }

        const char *errorString = ::strerror(errno);
        if (errorString) {
            ::write(STDERR_FILENO, errorString, ::strlen(errorString));
            ::write(STDERR_FILENO, "\n", 1);
        }
        ::_exit(127);
    }

    m_masterFd = masterFd;
    m_childPid = childPid;
    m_outputBuffer.clear();

    const int currentFlags = ::fcntl(m_masterFd, F_GETFL, 0);
    if (currentFlags != -1) {
        ::fcntl(m_masterFd, F_SETFL, currentFlags | O_NONBLOCK);
    }

    installReadNotifier();
    updateConnectedState(true);
}

void ChatterClient::stop()
{
    if (!isRunning()) {
        return;
    }

    removeReadNotifier();

    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }

    if (m_childPid > 0) {
        ::kill(m_childPid, SIGTERM);
        ::waitpid(m_childPid, nullptr, 0);
    }

    m_childPid = -1;
    m_outputBuffer.clear();
    updateConnectedState(false);
}

void ChatterClient::sendCommand(const QString &command)
{
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    if (!isRunning()) {
        start();
    }

    if (m_masterFd < 0) {
        return;
    }

    writeBytes(trimmed.toUtf8() + '\r');
}

void ChatterClient::sendRawData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    if (!isRunning()) {
        start();
    }

    writeBytes(data);
}

void ChatterClient::setTerminalSize(int columns, int rows)
{
    columns = std::max(1, columns);
    rows = std::max(1, rows);

    if (m_columns == columns && m_rows == rows) {
        return;
    }

    m_columns = columns;
    m_rows = rows;
    applyTerminalSize();
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

void ChatterClient::handleMasterReadyRead()
{
    if (m_masterFd < 0) {
        return;
    }

    char buffer[4096];
    while (true) {
        const ssize_t bytesRead = ::read(m_masterFd, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            m_outputBuffer.append(buffer, bytesRead);

            QString text = takeDecodedOutput();
            if (!text.isEmpty()) {
                emit outputReceived(text);
            }
            continue;
        }

        if (bytesRead == 0) {
            handleChildFinished(true);
            return;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }

        handleChildFinished(true);
        const QByteArray message = escapeErrorMessage(QByteArray(::strerror(errno)));
        emit errorReceived(tr("Terminal read error: %1").arg(QString::fromLatin1(message)));
        return;
    }
}

void ChatterClient::handleChildFinished(bool emitErrorMessage)
{
    removeReadNotifier();

    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }

    if (!m_outputBuffer.isEmpty()) {
        const QString text = takeDecodedOutput();
        if (!text.isEmpty()) {
            emit outputReceived(text);
        }
    }

    int status = 0;
    if (m_childPid > 0) {
        const pid_t waited = ::waitpid(m_childPid, &status, WNOHANG);
        if (waited == 0) {
            ::waitpid(m_childPid, &status, 0);
        }
    }

    if (m_childPid > 0 && emitErrorMessage) {
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            emit errorReceived(tr("Session ended with exit code %1").arg(WEXITSTATUS(status)));
        } else if (WIFSIGNALED(status)) {
            emit errorReceived(tr("Session terminated by signal %1").arg(WTERMSIG(status)));
        }
    }

    m_childPid = -1;
    updateConnectedState(false);
}

void ChatterClient::updateConnectedState(bool connected)
{
    if (m_connected == connected) {
        return;
    }

    m_connected = connected;
    emit connectionStateChanged(connected);
}

void ChatterClient::installReadNotifier()
{
    removeReadNotifier();

    if (m_masterFd < 0) {
        return;
    }

    m_readNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated,
            this, [this]() { handleMasterReadyRead(); });
}

void ChatterClient::removeReadNotifier()
{
    if (!m_readNotifier) {
        return;
    }

    m_readNotifier->setEnabled(false);
    m_readNotifier->deleteLater();
    m_readNotifier = nullptr;
}

bool ChatterClient::isRunning() const
{
    return m_childPid > 0;
}

void ChatterClient::applyTerminalSize()
{
    if (m_masterFd < 0) {
        return;
    }

    struct winsize size;
    size.ws_col = static_cast<unsigned short>(std::max(1, m_columns));
    size.ws_row = static_cast<unsigned short>(std::max(1, m_rows));
    size.ws_xpixel = 0;
    size.ws_ypixel = 0;

    ::ioctl(m_masterFd, TIOCSWINSZ, &size);
}

QString ChatterClient::takeDecodedOutput()
{
    QString result;

    while (!m_outputBuffer.isEmpty()) {
        const int length = utf8BoundaryLength();
        if (length <= 0) {
            break;
        }

        result.append(QString::fromUtf8(m_outputBuffer.constData(), length));
        m_outputBuffer.remove(0, length);
    }

    return result;
}

int ChatterClient::utf8BoundaryLength() const
{
    if (m_outputBuffer.isEmpty()) {
        return 0;
    }

    const int size = m_outputBuffer.size();
    int index = 0;
    int lastComplete = 0;

    while (index < size) {
        const unsigned char byte = static_cast<unsigned char>(m_outputBuffer.at(index));

        int expected = 0;
        if ((byte & 0x80) == 0x00) {
            expected = 1;
        } else if ((byte & 0xE0) == 0xC0) {
            expected = 2;
        } else if ((byte & 0xF0) == 0xE0) {
            expected = 3;
        } else if ((byte & 0xF8) == 0xF0) {
            expected = 4;
        } else {
            return index + 1;
        }

        if (index + expected > size) {
            break;
        }

        bool valid = true;
        for (int i = 1; i < expected; ++i) {
            const unsigned char continuation = static_cast<unsigned char>(m_outputBuffer.at(index + i));
            if ((continuation & 0xC0) != 0x80) {
                valid = false;
                break;
            }
        }

        if (!valid) {
            return index + 1;
        }

        index += expected;
        lastComplete = index;
    }

    return lastComplete;
}

void ChatterClient::writeBytes(const QByteArray &data)
{
    if (m_masterFd < 0 || data.isEmpty()) {
        return;
    }

    const char *buffer = data.constData();
    qsizetype remaining = data.size();

    while (remaining > 0) {
        const ssize_t written = ::write(m_masterFd, buffer, static_cast<size_t>(remaining));
        if (written > 0) {
            buffer += written;
            remaining -= written;
            continue;
        }

        if (written < 0 && errno == EINTR) {
            continue;
        }

        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        }

        break;
    }
}
