#include "CommandCatalog.h"

#include <QRegularExpression>

namespace {
CommandDescriptor makeDescriptor(const QString &label,
                                 const QString &command,
                                 const QString &hint = QString())
{
    return CommandDescriptor{label, command, hint};
}
}

QVector<CommandDescriptor> CommandCatalog::allCommands()
{
    return {
        makeDescriptor(QObject::tr("Help"), QStringLiteral("/help")),
        makeDescriptor(QObject::tr("Exit"), QStringLiteral("/exit")),
        makeDescriptor(QObject::tr("Change Nickname"), QStringLiteral("/nick %1"), QObject::tr("nickname")),
        makeDescriptor(QObject::tr("Private Message"), QStringLiteral("/pm %1"), QObject::tr("username message")),
        makeDescriptor(QObject::tr("Message of the Day"), QStringLiteral("/motd")),
        makeDescriptor(QObject::tr("Set Status"), QStringLiteral("/status %1"), QObject::tr("message")),
        makeDescriptor(QObject::tr("Show Status"), QStringLiteral("/showstatus %1"), QObject::tr("username")),
        makeDescriptor(QObject::tr("List Users"), QStringLiteral("/users")),
        makeDescriptor(QObject::tr("Search Users"), QStringLiteral("/search %1"), QObject::tr("text")),
        makeDescriptor(QObject::tr("Show Chat Message"), QStringLiteral("/chat %1"), QObject::tr("message-id")),
        makeDescriptor(QObject::tr("Reply"), QStringLiteral("/reply %1"), QObject::tr("message-id text")),
        makeDescriptor(QObject::tr("Share Image"), QStringLiteral("/image %1"), QObject::tr("url [caption]")),
        makeDescriptor(QObject::tr("Share Video"), QStringLiteral("/video %1"), QObject::tr("url [caption]")),
        makeDescriptor(QObject::tr("Share Audio"), QStringLiteral("/audio %1"), QObject::tr("url [caption]")),
        makeDescriptor(QObject::tr("Share Files"), QStringLiteral("/files %1"), QObject::tr("url [caption]")),
        makeDescriptor(QObject::tr("Open ASCII Art Composer"), QStringLiteral("/asciiart")),
        makeDescriptor(QObject::tr("Start Game"), QStringLiteral("/game %1"), QObject::tr("tetris|liargame")),
        makeDescriptor(QObject::tr("Set Handle Color"), QStringLiteral("/color %1"), QObject::tr("text;highlight[;bold]")),
        makeDescriptor(QObject::tr("Set System Palette"), QStringLiteral("/systemcolor %1"), QObject::tr("fg;background[;highlight][;bold]")),
        makeDescriptor(QObject::tr("Set Translation Language"), QStringLiteral("/set-trans-lang %1"), QObject::tr("language|off")),
        makeDescriptor(QObject::tr("Set Target Language"), QStringLiteral("/set-target-lang %1"), QObject::tr("language|off")),
        makeDescriptor(QObject::tr("Toggle Translation"), QStringLiteral("/translate %1"), QObject::tr("on|off")),
        makeDescriptor(QObject::tr("Translation Scope"), QStringLiteral("/translate-scope %1"), QObject::tr("chat|chat-nohistory|all")),
        makeDescriptor(QObject::tr("Toggle Gemini"), QStringLiteral("/gemini %1"), QObject::tr("on|off")),
        makeDescriptor(QObject::tr("Gemini Unfreeze"), QStringLiteral("/gemini-unfreeze")),
        makeDescriptor(QObject::tr("Toggle Eliza"), QStringLiteral("/eliza %1"), QObject::tr("on|off")),
        makeDescriptor(QObject::tr("Chat Spacing"), QStringLiteral("/chat-spacing %1"), QObject::tr("0-5")),
        makeDescriptor(QObject::tr("Apply Palette"), QStringLiteral("/palette %1"), QObject::tr("name")),
        makeDescriptor(QObject::tr("Today's Function"), QStringLiteral("/today")),
        makeDescriptor(QObject::tr("Show Date"), QStringLiteral("/date %1"), QObject::tr("timezone")),
        makeDescriptor(QObject::tr("Register OS"), QStringLiteral("/os %1"), QObject::tr("name")),
        makeDescriptor(QObject::tr("Lookup OS"), QStringLiteral("/getos %1"), QObject::tr("username")),
        makeDescriptor(QObject::tr("Register Birthday"), QStringLiteral("/birthday %1"), QObject::tr("YYYY-MM-DD")),
        makeDescriptor(QObject::tr("Find Soulmate"), QStringLiteral("/soulmate")),
        makeDescriptor(QObject::tr("Find Pair"), QStringLiteral("/pair")),
        makeDescriptor(QObject::tr("Connected Users"), QStringLiteral("/connected")),
        makeDescriptor(QObject::tr("Grant Operator"), QStringLiteral("/grant %1"), QObject::tr("ip")),
        makeDescriptor(QObject::tr("Revoke Operator"), QStringLiteral("/revoke %1"), QObject::tr("ip")),
        makeDescriptor(QObject::tr("Start Poll"), QStringLiteral("/poll %1"), QObject::tr("question|options")),
        makeDescriptor(QObject::tr("Vote"), QStringLiteral("/vote %1"), QObject::tr("label question|option")),
        makeDescriptor(QObject::tr("Single Vote"), QStringLiteral("/vote-single %1"), QObject::tr("label question|option")),
        makeDescriptor(QObject::tr("Elect"), QStringLiteral("/elect %1"), QObject::tr("label choice")),
        makeDescriptor(QObject::tr("Poke User"), QStringLiteral("/poke %1"), QObject::tr("username")),
        makeDescriptor(QObject::tr("Kick User"), QStringLiteral("/kick %1"), QObject::tr("username")),
        makeDescriptor(QObject::tr("Ban User"), QStringLiteral("/ban %1"), QObject::tr("username")),
        makeDescriptor(QObject::tr("List Bans"), QStringLiteral("/banlist")),
        makeDescriptor(QObject::tr("Block"), QStringLiteral("/block %1"), QObject::tr("user|ip")),
        makeDescriptor(QObject::tr("Unblock"), QStringLiteral("/unblock %1"), QObject::tr("target|all")),
        makeDescriptor(QObject::tr("Pardon"), QStringLiteral("/pardon %1"), QObject::tr("user|ip")),
        makeDescriptor(QObject::tr("React Good"), QStringLiteral("/good %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("React Sad"), QStringLiteral("/sad %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("React Cool"), QStringLiteral("/cool %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("React Angry"), QStringLiteral("/angry %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("React Checked"), QStringLiteral("/checked %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("React Love"), QStringLiteral("/love %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("React WTF"), QStringLiteral("/wtf %1"), QObject::tr("id")),
        makeDescriptor(QObject::tr("Vote 1"), QStringLiteral("/1")),
        makeDescriptor(QObject::tr("Vote 2"), QStringLiteral("/2")),
        makeDescriptor(QObject::tr("Vote 3"), QStringLiteral("/3")),
        makeDescriptor(QObject::tr("Vote 4"), QStringLiteral("/4")),
        makeDescriptor(QObject::tr("Vote 5"), QStringLiteral("/5")),
        makeDescriptor(QObject::tr("Bulletin Board"), QStringLiteral("/bbs %1"), QObject::tr("list|read|post|comment|regen|delete")),
        makeDescriptor(QObject::tr("Suspend Game"), QStringLiteral("/suspend!"))
    };
}

bool CommandCatalog::requiresUserInput(const CommandDescriptor &descriptor)
{
    static const QRegularExpression placeholder(QStringLiteral("%1"));
    return descriptor.command.contains(placeholder);
}
