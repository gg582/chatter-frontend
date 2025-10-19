#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QMetaType>

struct CommandDescriptor {
    QString label;
    QString command;
    QString placeholderHint;
};

Q_DECLARE_METATYPE(CommandDescriptor)

class CommandCatalog {
public:
    static QVector<CommandDescriptor> allCommands();
    static bool requiresUserInput(const CommandDescriptor &descriptor);
};
