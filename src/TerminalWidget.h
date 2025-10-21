#pragma once

#include <QWidget>
#include <QPointer>
#include <QFont>
#include <QByteArray>

class QTextBrowser;
class QLineEdit;

class TerminalWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);

    QTextBrowser *display() const;
    QLineEdit *input() const;

    void setTerminalFont(const QFont &font);
    QFont terminalFont() const;

signals:
    void keySequenceGenerated(const QByteArray &sequence);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointer<QTextBrowser> m_display;
    QPointer<QLineEdit> m_input;
};
