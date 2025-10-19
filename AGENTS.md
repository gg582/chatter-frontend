 > /help
Available commands:
/help                 - show this message
/exit                 - leave the chat
/nick <name>          - change your display name
/pm <username> <message> - send a private message
/motd                - view the message of the day
/status <message|clear> - set your profile status
/showstatus <username> - view someone else's status
/users               - announce the number of connected users
/search <text>       - search for users whose name matches text
/chat <message-id>   - show a past message by its identifier
/reply <message-id|r<reply-id>> <text> - reply to a message or reply
/image <url> [caption] - share an image link
/video <url> [caption] - share a video link
/audio <url> [caption] - share an audio clip
/files <url> [caption] - share a downloadable file
/asciiart           - open the ASCII art composer (max 64 lines, 1/min)
/game <tetris|liargame> - start a minigame in the chat (use /suspend! or Ctrl+Z to exit)
Up/Down arrows           - scroll recent chat history
/color (text;highlight[;bold]) - style your handle
/systemcolor (fg;background[;highlight][;bold]) - style the interface (third value may be highlight or bold; use /systemcolor reset to restore defaults)
/set-trans-lang <language|off> - translate terminal output to a target language
/set-target-lang <language|off> - translate your outgoing messages
/weather <region> <city> - show the weather for a region and city
/translate <on|off>    - enable or disable translation after configuring languages
/translate-scope <chat|chat-nohistory|all> - limit translation to chat/BBS, optionally skipping scrollback (operator only)
/gemini <on|off>       - toggle Gemini provider (operator only)
/gemini-unfreeze      - clear automatic Gemini cooldown (operator only)
/eliza <on|off>        - toggle the eliza moderator persona (operator only)
/chat-spacing <0-5>    - reserve blank lines before translated captions in chat
/palette <name>        - apply a predefined interface palette (/palette list)
/today               - discover today's function (once per day)
/date <timezone>     - view the server time in another timezone
/os <name>           - record the operating system you use
/getos <username>    - look up someone else's recorded operating system
/birthday YYYY-MM-DD - register your birthday
/soulmate            - list users sharing your birthday
/pair                - list users sharing your recorded OS
/connected           - privately list everyone connected
/grant <ip>          - grant operator access to an IP (LAN only)
/revoke <ip>         - revoke an IP's operator access (LAN top admin)
/poll <question>|<option...> - start or view a poll
/vote <label> <question>|<option...> - start or inspect a multiple-choice named poll (use /vote @close <label> to end it)
/vote-single <label> <question>|<option...> - start or inspect a single-choice named poll
/elect <label> <choice> - vote in a named poll by label
/poke <username>      - send a bell to call a user
/kick <username>      - disconnect a user (operator only)
/ban <username>       - ban a user (operator only)
/banlist             - list active bans (operator only)
/block <user|ip>      - hide messages from a user or IP locally (/block list to review)
/unblock <target|all> - remove a local block entry
/pardon <user|ip>     - remove a ban (operator only)
/good|/sad|/cool|/angry|/checked|/love|/wtf <id> - react to a message by number
/1 .. /5             - vote for an option in the active poll
/bbs [list|read|post|comment|regen|delete] - open the bulletin board system (see /bbs for details, finish >/__BBS_END> to post)
/suspend!            - suspend the active game (Ctrl+Z while playing)
Regular messages are shared with everyone.


these are full command lists of chatter service.
You should route these in GUI application code that supports cross-building:
- auto connect with username instead of doing ssh username@chat.korokorok.com
- slash based commands are done with gui wrapper's menu -> write down constructed command to shell
- keep ansi palette, but use better fonts
- don't make it scary; the user should feel this as retro-concept normal chatting app/bbs although it uses 100% TUI
