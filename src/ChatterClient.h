#pragma once

#include <QObject>
#include <QProcess>
#include <QByteArray>

class ChatterClient : public QObject
{
    Q_OBJECT
public:
    explicit ChatterClient(QObject *parent = nullptr);

    void setUsername(const QString &username);
    QString username() const;

    void start();
    void stop();
    void sendCommand(const QString &command);
    void sendRawData(const QByteArray &data);

signals:
    void outputReceived(const QString &text);
    void errorReceived(const QString &text);
    void connectionStateChanged(bool connected);

private slots:
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();
    void handleStateChanged(QProcess::ProcessState state);

private:
    QStringList buildCommand() const;

    QProcess m_process;
    QString m_username;
    QString m_host;
};
