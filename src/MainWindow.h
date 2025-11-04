#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QPointer>
#include <QStringList>

class QTextBrowser;
class QLabel;
class QAction;
class QMenu;
class TerminalWidget;

class ChatterClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void handleClientOutput(const QString &text);
    void handleClientError(const QString &text);
    void handleConnectionStateChanged(bool connected);
    void handleCommandActionTriggered();
    void handleTerminalInput(const QByteArray &data);
    void handleTerminalSizeChanged(int columns, int rows);
    void initiateConnection();
    void stopConnection();
    void changeNickname();
    void openAppearanceSettings();

private:
    void createMenus();
    void populateCommandMenu(QMenu *menu);
    void appendMessage(const QString &text, bool isError = false);
    QString promptForArgument(const QString &hint) const;
    void applyRetroPalette();
    bool ensureNickname(bool forcePrompt = false);
    void openAsciiArtComposer();
    void sendAsciiArtLines(const QStringList &lines);
    void saveAsciiArtLocally(const QStringList &lines);

    QPointer<TerminalWidget> m_terminal;
    QPointer<QTextBrowser> m_display;
    QPointer<QLabel> m_statusLabel;
    QPointer<ChatterClient> m_client;
    QAction *m_connectAction;
    QAction *m_disconnectAction;
    bool m_isConnected;
    bool m_nicknameConfirmed;
};
