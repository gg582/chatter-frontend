#pragma once

#include <QByteArray>
#include <QFont>
#include <QPointer>
#include <QWidget>

class QTextBrowser;
class TerminalWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);

    QTextBrowser *display() const;

    void setTerminalFont(const QFont &font);
    QFont terminalFont() const;

signals:
    void bytesGenerated(const QByteArray &data);
    void terminalSizeChanged(int columns, int rows);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private:
    void scheduleTerminalSizeUpdate();
    void emitTerminalSize();
    bool handleKeyPress(QKeyEvent *event);
    QByteArray translateKeyEvent(QKeyEvent *event) const;

    QPointer<QTextBrowser> m_display;
    bool m_pendingSizeUpdate = false;
};
