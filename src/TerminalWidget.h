#pragma once

#include <QWidget>
#include <QPointer>

class QTextBrowser;
class QLineEdit;

class TerminalWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);

    QTextBrowser *display() const;
    QLineEdit *input() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointer<QTextBrowser> m_display;
    QPointer<QLineEdit> m_input;
};
