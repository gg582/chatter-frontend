#include "MainWindow.h"

#include "ChatterClient.h"
#include "CommandCatalog.h"
#include "TerminalWidget.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenuBar>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStatusBar>
#include <QMessageBox>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextEdit>
#include <QTextOption>
#include <QTextStream>
#include <QTextDocument>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QVBoxLayout>
#include <QWidget>
#include <QStandardPaths>
#include <QIODevice>
#include <QByteArray>

#include <algorithm>

namespace {

constexpr int kAsciiMaxLines = 64;

class AsciiComposerDialog : public QDialog
{
public:
    explicit AsciiComposerDialog(QWidget *parent = nullptr)
        : QDialog(parent)
        , m_editor(new QPlainTextEdit(this))
        , m_lineCountLabel(new QLabel(this))
    {
        setWindowTitle(tr("ASCII Art Composer"));
        setModal(true);

        m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        m_editor->setWordWrapMode(QTextOption::NoWrap);
        m_editor->setTabChangesFocus(false);
        m_editor->setPlaceholderText(tr("Compose up to 64 lines of ASCII art."));

        auto *buttonBox = new QDialogButtonBox(this);
        m_commitButton = buttonBox->addButton(tr("Commit"), QDialogButtonBox::AcceptRole);
        m_commitButton->setDefault(true);
        buttonBox->addButton(QDialogButtonBox::Cancel);

        auto *layout = new QVBoxLayout(this);
        layout->addWidget(m_editor);

        auto *footerLayout = new QHBoxLayout();
        footerLayout->addWidget(m_lineCountLabel);
        footerLayout->addStretch();
        footerLayout->addWidget(buttonBox);
        layout->addLayout(footerLayout);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
            enforceLineLimit();
            updateStatus();
        });

        updateStatus();
    }

    QStringList lines() const
    {
        const QString text = m_editor->toPlainText();
        if (text.isEmpty()) {
            return {};
        }
        QStringList collected = text.split('\n', Qt::KeepEmptyParts);
        if (collected.size() > kAsciiMaxLines) {
            collected = collected.mid(0, kAsciiMaxLines);
        }
        return collected;
    }

private:
    void enforceLineLimit()
    {
        QString text = m_editor->toPlainText();
        QStringList collected = text.split('\n', Qt::KeepEmptyParts);
        if (text.isEmpty() || collected.size() <= kAsciiMaxLines) {
            return;
        }

        collected = collected.mid(0, kAsciiMaxLines);
        const QString trimmed = collected.join('\n');

        QTextCursor cursor = m_editor->textCursor();
        const int position = cursor.position();

        m_editor->blockSignals(true);
        m_editor->setPlainText(trimmed);
        cursor = m_editor->textCursor();
        cursor.setPosition(std::min(position, int(trimmed.size())));
        m_editor->setTextCursor(cursor);
        m_editor->blockSignals(false);
    }

    void updateStatus()
    {
        const QString text = m_editor->toPlainText();
        QStringList collected = text.isEmpty() ? QStringList() : text.split('\n', Qt::KeepEmptyParts);
        const int count = text.isEmpty() ? 0 : collected.size();

        m_lineCountLabel->setText(tr("Lines: %1 / %2").arg(count).arg(kAsciiMaxLines));

        const bool hasContent = std::any_of(collected.cbegin(), collected.cend(), [](const QString &line) {
            return !line.isEmpty();
        });

        const bool withinLimit = count > 0 && count <= kAsciiMaxLines;
        m_commitButton->setEnabled(hasContent && withinLimit);
    }

    QPlainTextEdit *m_editor;
    QLabel *m_lineCountLabel;
    QPushButton *m_commitButton;
};

class AppearanceDialog : public QDialog
{
public:
    explicit AppearanceDialog(const QFont &initialFont, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_fontCombo(new QFontComboBox(this))
        , m_sizeSpin(new QDoubleSpinBox(this))
        , m_previewLabel(new QLabel(this))
    {
        setWindowTitle(tr("Appearance Settings"));
        setModal(true);

        m_fontCombo->setFontFilters(QFontComboBox::MonospacedFonts | QFontComboBox::ScalableFonts);
        m_fontCombo->setEditable(false);

        m_sizeSpin->setRange(6.0, 48.0);
        m_sizeSpin->setDecimals(1);
        m_sizeSpin->setSingleStep(0.5);

        QFont baseFont = initialFont;
        if (baseFont.pointSizeF() <= 0) {
            baseFont.setPointSizeF(10.0);
        }
        m_fontCombo->setCurrentFont(baseFont);
        m_sizeSpin->setValue(baseFont.pointSizeF());

        m_previewLabel->setFrameShape(QFrame::StyledPanel);
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->setWordWrap(true);
        m_previewLabel->setText(tr("0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz"));

        auto *formLayout = new QFormLayout();
        formLayout->addRow(tr("Font"), m_fontCombo);
        formLayout->addRow(tr("Size"), m_sizeSpin);

        auto *layout = new QVBoxLayout(this);
        layout->addLayout(formLayout);
        layout->addWidget(m_previewLabel);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);

        connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &) {
            updatePreview();
        });
        connect(m_sizeSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            updatePreview();
        });

        updatePreview();
    }

    QFont selectedFont() const
    {
        QFont font = m_fontCombo->currentFont();
        font.setPointSizeF(m_sizeSpin->value());
        font.setStyleHint(QFont::TypeWriter);
        return font;
    }

private:
    void updatePreview()
    {
        QFont preview = selectedFont();
        m_previewLabel->setFont(preview);
    }

    QFontComboBox *m_fontCombo;
    QDoubleSpinBox *m_sizeSpin;
    QLabel *m_previewLabel;
};

struct FormattedFragment {
    QString text;
    QTextCharFormat format;
};

QColor basicAnsiColor(int index, bool bright)
{
    static const QColor normal[] = {
        QColor(0, 0, 0),         // black
        QColor(128, 0, 0),       // red
        QColor(0, 128, 0),       // green
        QColor(128, 128, 0),     // yellow
        QColor(0, 0, 128),       // blue
        QColor(128, 0, 128),     // magenta
        QColor(0, 128, 128),     // cyan
        QColor(192, 192, 192)    // white
    };
    static const QColor brightColors[] = {
        QColor(128, 128, 128),   // bright black / gray
        QColor(255, 0, 0),       // bright red
        QColor(0, 255, 0),       // bright green
        QColor(255, 255, 0),     // bright yellow
        QColor(0, 0, 255),       // bright blue
        QColor(255, 0, 255),     // bright magenta
        QColor(0, 255, 255),     // bright cyan
        QColor(255, 255, 255)    // bright white
    };

    index = qBound(0, index, 7);
    return bright ? brightColors[index] : normal[index];
}

QColor colorFrom256Palette(int index)
{
    if (index < 0) {
        index = 0;
    }
    if (index > 255) {
        index = 255;
    }

    if (index < 16) {
        const bool bright = index >= 8;
        return basicAnsiColor(index % 8, bright);
    }

    if (index < 232) {
        const int base = index - 16;
        const int r = base / 36;
        const int g = (base / 6) % 6;
        const int b = base % 6;
        auto component = [](int value) {
            if (value == 0) {
                return 0;
            }
            return 55 + (value * 40);
        };
        return QColor(component(r), component(g), component(b));
    }

    const int gray = 8 + ((index - 232) * 10);
    return QColor(gray, gray, gray);
}

void applyExtendedColor(QTextCharFormat &format,
                        bool isForeground,
                        const QTextCharFormat &baseFormat,
                        int mode,
                        const QList<int> &params,
                        int &i)
{
    if (mode == 5) {
        if (i + 1 >= params.size()) {
            return;
        }
        const QColor color = colorFrom256Palette(params.at(++i));
        if (isForeground) {
            format.setForeground(color);
        } else {
            format.setBackground(color);
        }
    } else if (mode == 2) {
        if (i + 3 >= params.size()) {
            return;
        }
        const int r = params.at(++i);
        const int g = params.at(++i);
        const int b = params.at(++i);
        const QColor color(r, g, b);
        if (!color.isValid()) {
            return;
        }
        if (isForeground) {
            format.setForeground(color);
        } else {
            format.setBackground(color);
        }
    } else {
        if (isForeground) {
            format.setForeground(baseFormat.foreground());
        } else {
            format.setBackground(baseFormat.background());
        }
    }
}

void applySgr(const QList<int> &params,
              QTextCharFormat &currentFormat,
              const QTextCharFormat &baseFormat)
{
    if (params.isEmpty()) {
        currentFormat = baseFormat;
        return;
    }

    for (int i = 0; i < params.size(); ++i) {
        const int code = params.at(i);
        switch (code) {
        case 0:
            currentFormat = baseFormat;
            break;
        case 1:
            currentFormat.setFontWeight(QFont::Bold);
            break;
        case 3:
            currentFormat.setFontItalic(true);
            break;
        case 4:
            currentFormat.setFontUnderline(true);
            break;
        case 22:
            currentFormat.setFontWeight(baseFormat.fontWeight());
            break;
        case 23:
            currentFormat.setFontItalic(baseFormat.fontItalic());
            break;
        case 24:
            currentFormat.setFontUnderline(baseFormat.fontUnderline());
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            currentFormat.setForeground(basicAnsiColor(code - 30, false));
            break;
        case 39:
            currentFormat.setForeground(baseFormat.foreground());
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            currentFormat.setBackground(basicAnsiColor(code - 40, false));
            break;
        case 49:
            currentFormat.setBackground(baseFormat.background());
            break;
        case 90: case 91: case 92: case 93:
        case 94: case 95: case 96: case 97:
            currentFormat.setForeground(basicAnsiColor(code - 90, true));
            break;
        case 100: case 101: case 102: case 103:
        case 104: case 105: case 106: case 107:
            currentFormat.setBackground(basicAnsiColor(code - 100, true));
            break;
        case 38:
        case 48:
            if (i + 1 < params.size()) {
                const bool isForeground = (code == 38);
                const int mode = params.at(++i);
                applyExtendedColor(currentFormat, isForeground, baseFormat, mode, params, i);
            }
            break;
        default:
            break;
        }
    }
}

void insertFragmentWithLinks(QTextCursor &cursor,
                             const QString &text,
                             const QTextCharFormat &format)
{
    if (text.isEmpty()) {
        return;
    }

    static const QRegularExpression urlRegex(
        QStringLiteral(R"((https?://[^\s<>"]+))"));

    int lastIndex = 0;
    auto it = urlRegex.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const int start = match.capturedStart();
        if (start > lastIndex) {
            cursor.insertText(text.mid(lastIndex, start - lastIndex), format);
        }

        const QString url = match.captured();
        QTextCharFormat linkFormat = format;
        linkFormat.setAnchor(true);
        linkFormat.setAnchorHref(url);
        linkFormat.setFontUnderline(true);
        linkFormat.setForeground(QBrush(qApp->palette().color(QPalette::Link)));
        cursor.insertText(url, linkFormat);
        lastIndex = match.capturedEnd();
    }

    if (lastIndex < text.size()) {
        cursor.insertText(text.mid(lastIndex), format);
    }
}

QVector<FormattedFragment> parseAnsiText(const QString &input,
                                         const QTextCharFormat &baseFormat)
{
    QVector<FormattedFragment> fragments;
    QTextCharFormat currentFormat = baseFormat;
    QString buffer;

    auto flushBuffer = [&]() {
        if (buffer.isEmpty()) {
            return;
        }
        fragments.append({buffer, currentFormat});
        buffer.clear();
    };

    for (int i = 0; i < input.size(); ++i) {
        const QChar ch = input.at(i);
        if (ch == QLatin1Char('\x1b')) {
            flushBuffer();
            if (i + 1 >= input.size()) {
                continue;
            }
            if (input.at(i + 1) != QLatin1Char('[')) {
                continue;
            }

            int j = i + 2;
            QString number;
            QList<int> params;
            for (; j < input.size(); ++j) {
                const QChar c = input.at(j);
                if (c.isDigit()) {
                    number.append(c);
                    continue;
                }
                if (c == QLatin1Char(';')) {
                    params.append(number.isEmpty() ? 0 : number.toInt());
                    number.clear();
                    continue;
                }
                if (c == QLatin1Char('?')) {
                    continue;
                }
                if (!number.isEmpty() || params.isEmpty()) {
                    params.append(number.isEmpty() ? 0 : number.toInt());
                    number.clear();
                }
                if (c == QLatin1Char('m')) {
                    applySgr(params, currentFormat, baseFormat);
                }
                break;
            }
            i = j;
            continue;
        }

        if (ch == QLatin1Char('\r')) {
            if (i + 1 < input.size() && input.at(i + 1) == QLatin1Char('\n')) {
                continue;
            }
            buffer.append(QLatin1Char('\n'));
            flushBuffer();
            continue;
        }

        if (ch == QLatin1Char('\b')) {
            if (!buffer.isEmpty()) {
                buffer.chop(1);
            }
            continue;
        }

        if (ch == QLatin1Char('\a')) {
            continue;
        }

        buffer.append(ch);
    }

    flushBuffer();
    return fragments;
}

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
    , m_isConnected(false)
    , m_nicknameConfirmed(false)
    , m_pendingLineBreak(false)
{
    m_client = new ChatterClient(this);

    QFont retroFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    retroFont.setPointSizeF(10.0);
    setFont(retroFont);

    m_terminal = new TerminalWidget(this);
    if (m_terminal) {
        m_terminal->setTerminalFont(retroFont);
    }
    setCentralWidget(m_terminal);

    m_display = m_terminal ? m_terminal->display() : nullptr;
    if (m_display) {
        m_display->setReadOnly(true);
        m_display->setOpenLinks(true);
        m_display->setOpenExternalLinks(true);
        m_display->setLineWrapMode(QTextEdit::NoWrap);
        m_display->setWordWrapMode(QTextOption::NoWrap);
        m_display->setUndoRedoEnabled(false);

        QTextOption textOption = m_display->document()->defaultTextOption();
        textOption.setFlags(textOption.flags() & ~QTextOption::AddSpaceForLineAndParagraphSeparators);
        m_display->document()->setDefaultTextOption(textOption);
    }

    m_input = m_terminal ? m_terminal->input() : nullptr;
    if (m_input) {
        m_input->setPlaceholderText(tr("Type a message or pick a command from the menu"));
    }

    if (m_terminal) {
        connect(m_terminal.data(), &TerminalWidget::keySequenceGenerated,
                this, [this](const QByteArray &sequence) {
                    if (!sequence.isEmpty() && m_client) {
                        m_client->sendRawData(sequence);
                    }
                });
    }

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);

    createMenus();
    applyRetroPalette();

    if (m_input) {
        connect(m_input.data(), &QLineEdit::returnPressed,
                this, &MainWindow::handleSendRequested);
    }
    connect(m_client.data(), &ChatterClient::outputReceived,
            this, &MainWindow::handleClientOutput);
    connect(m_client.data(), &ChatterClient::errorReceived,
            this, &MainWindow::handleClientError);
    connect(m_client.data(), &ChatterClient::connectionStateChanged,
            this, &MainWindow::handleConnectionStateChanged);

    QTimer::singleShot(0, this, [this]() {
        if (ensureNickname(true)) {
            initiateConnection();
        } else if (m_statusLabel) {
            m_statusLabel->setText(tr("Set a nickname to connect"));
        }
    });
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
    m_isConnected = connected;
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
    if (descriptor.command == QStringLiteral("/asciiart")) {
        openAsciiArtComposer();
        return;
    }

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
    if (!ensureNickname()) {
        if (m_statusLabel) {
            m_statusLabel->setText(tr("Set a nickname to connect"));
        }
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

void MainWindow::changeNickname()
{
    const QString previous = m_client ? m_client->username() : QString();
    if (!ensureNickname(true)) {
        return;
    }

    if (!m_client) {
        return;
    }

    const QString updated = m_client->username();
    if (updated == previous) {
        return;
    }

    if (m_statusLabel && !m_isConnected) {
        m_statusLabel->setText(tr("Ready as %1").arg(updated));
    }

    if (m_isConnected) {
        stopConnection();
        QTimer::singleShot(0, this, &MainWindow::initiateConnection);
    }
}

void MainWindow::openAppearanceSettings()
{
    if (!m_terminal) {
        return;
    }

    QFont currentFont = m_terminal->terminalFont();
    AppearanceDialog dialog(currentFont, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QFont selectedFont = dialog.selectedFont();
    m_terminal->setTerminalFont(selectedFont);
    setFont(selectedFont);

    if (m_statusLabel) {
        m_statusLabel->setFont(selectedFont);
    }

    statusBar()->showMessage(tr("Font updated"), 2000);
}

void MainWindow::createMenus()
{
    auto *sessionMenu = menuBar()->addMenu(tr("Session"));
    m_connectAction = sessionMenu->addAction(tr("Connect"), this, &MainWindow::initiateConnection);
    sessionMenu->addAction(tr("Set Nickname..."), this, &MainWindow::changeNickname);
    m_disconnectAction = sessionMenu->addAction(tr("Disconnect"), this, &MainWindow::stopConnection);
    m_disconnectAction->setEnabled(false);

    auto *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(tr("Font && Appearance..."), this, &MainWindow::openAppearanceSettings);

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
    sanitized.replace("\r\n", "\n");

    const bool hadTrailingNewline = sanitized.endsWith(QLatin1Char('\n'));
    if (hadTrailingNewline) {
        sanitized.chop(1);
    }

    const bool hasContent = !sanitized.isEmpty();

    QTextCharFormat baseFormat;
    baseFormat.setForeground(isError ? QBrush(Qt::red)
                                    : QBrush(m_display->palette().color(QPalette::Text)));
    baseFormat.setBackground(QBrush(m_display->palette().color(QPalette::Base)));
    baseFormat.setFont(m_display->font());
    baseFormat.setFontWeight(QFont::Normal);
    baseFormat.setFontItalic(false);
    baseFormat.setFontUnderline(false);

    const QVector<FormattedFragment> fragments = parseAnsiText(sanitized, baseFormat);

    QTextCursor cursor = m_display->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(0);
    blockFormat.setBottomMargin(0);
    const QFontMetricsF metrics(m_display->font());
    const qreal lineHeight = std::max<qreal>(1.0, metrics.ascent() + metrics.descent());
    blockFormat.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);

    auto applyBlockFormat = [&]() {
        cursor.setBlockFormat(blockFormat);
    };

    cursor.beginEditBlock();

    if (hasContent && m_pendingLineBreak) {
        cursor.insertBlock();
        m_pendingLineBreak = false;
    }

    applyBlockFormat();

    bool insertedAnyText = false;

    for (const auto &fragment : fragments) {
        const QString &fragmentText = fragment.text;
        int position = 0;

        while (position < fragmentText.size()) {
            const int newlineIndex = fragmentText.indexOf(QLatin1Char('\n'), position);
            const int carriageIndex = fragmentText.indexOf(QLatin1Char('\r'), position);

            int breakIndex = fragmentText.size();
            enum class ControlType { None, Newline, Carriage };
            ControlType control = ControlType::None;

            if (newlineIndex != -1 && (carriageIndex == -1 || newlineIndex < carriageIndex)) {
                breakIndex = newlineIndex;
                control = ControlType::Newline;
            } else if (carriageIndex != -1) {
                breakIndex = carriageIndex;
                control = ControlType::Carriage;
            }

            if (breakIndex > position) {
                const QString chunk = fragmentText.mid(position, breakIndex - position);
                applyBlockFormat();
                insertFragmentWithLinks(cursor, chunk, fragment.format);
                if (!chunk.isEmpty()) {
                    insertedAnyText = true;
                }
            }

            if (control == ControlType::Newline) {
                cursor.insertBlock();
                applyBlockFormat();
                position = breakIndex + 1;
                continue;
            }

            if (control == ControlType::Carriage) {
                cursor.movePosition(QTextCursor::StartOfBlock);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                applyBlockFormat();
                position = breakIndex + 1;
                continue;
            }

            position = breakIndex;
            break;
        }
    }

    cursor.endEditBlock();
    m_display->setTextCursor(cursor);
    m_display->ensureCursorVisible();

    if (hadTrailingNewline) {
        m_pendingLineBreak = true;
    } else if (insertedAnyText) {
        m_pendingLineBreak = false;
    }
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

bool MainWindow::ensureNickname(bool forcePrompt)
{
    if (!m_client) {
        return false;
    }

    if (!forcePrompt && m_nicknameConfirmed && !m_client->username().trimmed().isEmpty()) {
        return true;
    }

    QString current = m_client->username().trimmed();
    bool ok = false;

    while (true) {
        const QString nickname = QInputDialog::getText(
            this,
            tr("Choose your nickname"),
            tr("Nickname:"),
            QLineEdit::Normal,
            current,
            &ok)
                                    .trimmed();

        if (!ok) {
            return false;
        }

        if (nickname.isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Nickname Required"),
                                 tr("Please enter a nickname to continue."));
            continue;
        }

        m_client->setUsername(nickname);
        m_nicknameConfirmed = true;
        if (m_statusLabel && !m_isConnected) {
            m_statusLabel->setText(tr("Ready as %1").arg(nickname));
        }
        return true;
    }
}

void MainWindow::openAsciiArtComposer()
{
    AsciiComposerDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QStringList lines = dialog.lines();
    if (lines.isEmpty()) {
        return;
    }

    statusBar()->showMessage(tr("Submitting ASCII art"), 2000);
    sendAsciiArtLines(lines);
    saveAsciiArtLocally(lines);
}

void MainWindow::sendAsciiArtLines(const QStringList &lines)
{
    if (!m_client || lines.isEmpty()) {
        return;
    }

    m_client->sendCommand(QStringLiteral("/asciiart"));
    for (const QString &line : lines) {
        m_client->sendCommand(line);
    }
    m_client->sendCommand(QStringLiteral(">/__ARTWORK_END>"));
}

void MainWindow::saveAsciiArtLocally(const QStringList &lines)
{
    if (lines.isEmpty()) {
        return;
    }

    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString basePath = documentsDir.isEmpty() ? QDir::homePath() : documentsDir;
    if (basePath.isEmpty()) {
        basePath = QDir::currentPath();
    }

    const QString suggestedPath = QDir(basePath).filePath(QStringLiteral("ascii-art.txt"));
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                         tr("Save ASCII Art"),
                                                         suggestedPath,
                                                         tr("Text Files (*.txt);;All Files (*.*)"));
    if (filePath.isEmpty()) {
        statusBar()->showMessage(tr("Save canceled"), 3000);
        return;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("Couldn't open %1 for writing.").arg(QDir::toNativeSeparators(filePath)));
        return;
    }

    QTextStream stream(&file);
    for (int i = 0; i < lines.size(); ++i) {
        stream << lines.at(i);
        if (i + 1 < lines.size()) {
            stream << '\n';
        }
    }

    if (stream.status() != QTextStream::Ok) {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("An error occurred while writing %1.")
                                 .arg(QDir::toNativeSeparators(filePath)));
        file.cancelWriting();
        return;
    }

    if (!file.commit()) {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("Couldn't finalize %1.").arg(QDir::toNativeSeparators(filePath)));
        return;
    }

    statusBar()->showMessage(tr("Saved ASCII art to %1").arg(QDir::toNativeSeparators(filePath)), 5000);
}

void MainWindow::applyRetroPalette()
{
    QPalette palette = qApp->palette();
    const QColor background = basicAnsiColor(0, false);
    const QColor foreground = basicAnsiColor(7, false);

    palette.setColor(QPalette::Base, background);
    palette.setColor(QPalette::AlternateBase, basicAnsiColor(0, true));
    palette.setColor(QPalette::Text, foreground);
    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::WindowText, foreground);
    palette.setColor(QPalette::Button, background);
    palette.setColor(QPalette::ButtonText, foreground);
    palette.setColor(QPalette::BrightText, basicAnsiColor(7, true));
    palette.setColor(QPalette::Highlight, basicAnsiColor(4, true));
    palette.setColor(QPalette::HighlightedText, basicAnsiColor(7, true));
    palette.setColor(QPalette::Link, basicAnsiColor(6, true));
    palette.setColor(QPalette::LinkVisited, basicAnsiColor(5, true));

    qApp->setPalette(palette);
}
