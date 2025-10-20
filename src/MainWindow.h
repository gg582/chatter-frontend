#pragma once

#include <QMainWindow>
#include <QPointer>

class QLineEdit;
class QTextBrowser;
class QLabel;
class QAction;
class QMenu;

class ChatterClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void handleSendRequested();
    void handleClientOutput(const QString &text);
    void handleClientError(const QString &text);
    void handleConnectionStateChanged(bool connected);
    void handleCommandActionTriggered();
    void initiateConnection();
    void stopConnection();

private:
    void createMenus();
    void populateCommandMenu(QMenu *menu);
    void appendMessage(const QString &text, bool isError = false);
    QString promptForArgument(const QString &hint) const;
    void applyRetroPalette();

    QPointer<QTextBrowser> m_display;
    QPointer<QLineEdit> m_input;
    QPointer<QLabel> m_statusLabel;
    QPointer<ChatterClient> m_client;
    QAction *m_connectAction;
    QAction *m_disconnectAction;
};
