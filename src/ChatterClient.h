#pragma once

#include <QByteArray>
#include <QObject>
#include <QStringList>

#include <sys/types.h>

class QSocketNotifier;

class ChatterClient : public QObject
{
    Q_OBJECT
public:
    explicit ChatterClient(QObject *parent = nullptr);
    ~ChatterClient() override;

    void setUsername(const QString &username);
    QString username() const;

    void start();
    void stop();
    void sendCommand(const QString &command);
    void sendRawData(const QByteArray &data);
    void setTerminalSize(int columns, int rows);

signals:
    void outputReceived(const QString &text);
    void errorReceived(const QString &text);
    void connectionStateChanged(bool connected);

private:
    QStringList buildCommand() const;
    void handleMasterReadyRead();
    void handleChildFinished(bool emitErrorMessage = true);
    void updateConnectedState(bool connected);
    void installReadNotifier();
    void removeReadNotifier();
    bool isRunning() const;
    void applyTerminalSize();
    QString takeDecodedOutput();
    int utf8BoundaryLength() const;
    void writeBytes(const QByteArray &data);

    int m_masterFd;
    pid_t m_childPid;
    QSocketNotifier *m_readNotifier;
    QString m_username;
    QString m_host;
    QByteArray m_outputBuffer;
    int m_columns;
    int m_rows;
    bool m_connected;
};
