enum class numeric_reply : int {

    /// example: "<nickname> :No such nick/channel"
    /// description:
    ///  Used to indicate the nickname parameter supplied to a
    ///  command is currently unused.
    ERR_NOSUCHNICK = 401,

    /// example: "<server name> :No such server"
    /// description:
    ///  Used to indicate the server name given currently
    ///  doesn't exist.
    ERR_NOSUCHSERVER = 402,

    /// example: "<channel name> :No such channel"
    /// description:
    ///  Used to indicate the given channel name is invalid.
    ERR_NOSUCHCHANNEL = 403,

    /// example: "<channel name> :Cannot send to channel"
    /// description:
    ///  Sent to a user who is either (a) not on a channel
    ///  which is mode +n or (b) not a chanop (or mode +v) on
    ///  a channel which has mode +m set and is trying to send
    ///  a PRIVMSG message to that channel.
    ERR_CANNOTSENDTOCHAN = 404,

    /// example: "<channel name> :You have joined too many channels"
    /// description:
    ///  Sent to a user when they have joined the maximum
    ///  number of allowed channels and they try to join
    ///  another channel.
    ERR_TOOMANYCHANNELS = 405,

    /// example: "<nickname> :There was no such nickname"
    /// description:
    ///  Returned by WHOWAS to indicate there is no history
    ///  information for that nickname.
    ERR_WASNOSUCHNICK = 406,

    /// example: "<target> :Duplicate recipients. No message delivered"
    /// description:
    ///  Returned to a client which is attempting to send a
    ///  PRIVMSG/NOTICE using the user@host destination format
    ///  and for a user@host which has several occurrences.
    ERR_TOOMANYTARGETS = 407,

    /// example: ":No origin specified"
    /// description:
    ///  PING or PONG message missing the originator parameter
    ///  which is required since these commands must work
    ///  without valid prefixes.
    ERR_NOORIGIN = 409,

    /// example: ":No recipient given (<command>)"
    ERR_NORECIPIENT = 411,

    /// example: ":No text to send"
    ERR_NOTEXTTOSEND = 412,

    /// example: "<mask> :No toplevel domain specified"
    ERR_NOTOPLEVEL = 413,

    /// example: "<mask> :Wildcard in toplevel domain"
    /// description:
    ///  412 - 414 are returned by PRIVMSG to indicate that
    ///  the message wasn't delivered for some reason.
    ///  ERR_NOTOPLEVEL and ERR_WILDTOPLEVEL are errors that
    ///  are returned when an invalid use of
    ///  "PRIVMSG $<server>" or "PRIVMSG #<host>" is attempted.
    ERR_WILDTOPLEVEL = 414,

    /// example: "<command> :Unknown command"
    /// description:
    ///  Returned to a registered client to indicate that the
    ///  command sent is unknown by the server.
    ERR_UNKNOWNCOMMAND = 421,

    /// example: ":MOTD File is missing"
    /// description:
    ///  Server's MOTD file could not be opened by the server.
    ERR_NOMOTD = 422,

    /// example: "<server> :No administrative info available"
    /// description:
    ///  Returned by a server in response to an ADMIN message
    ///  when there is an error in finding the appropriate
    ///  information.
    ERR_NOADMININFO = 423,

    /// example: ":File error doing <file op> on <file>"
    /// description:
    ///  Generic error message used to report a failed file
    ///  operation during the processing of a message.
    ERR_FILEERROR = 424,

    /// example: ":No nickname given"
    /// description:
    ///  Returned when a nickname parameter expected for a
    ///  command and isn't found.
    ERR_NONICKNAMEGIVEN = 431,

    /// example: "<nick> :Erroneus nickname"
    /// description:
    ///  Returned after receiving a NICK message which contains
    ///  characters which do not fall in the defined set.  See
    ///  section x.x.x for details on valid nicknames.
    ERR_ERRONEUSNICKNAME = 432,

    /// example: "<nick> :Nickname is already in use"
    /// description:
    ///  Returned when a NICK message is processed that results
    ///  in an attempt to change to a currently existing
    ///  nickname.
    ERR_NICKNAMEINUSE = 433,

    /// example: "<nick> :Nickname collision KILL"
    /// description:
    ///  Returned by a server to a client when it detects a
    ///  nickname collision (registered of a NICK that
    ///  already exists by another server).
    ERR_NICKCOLLISION = 436,

    /// example: "<nick> <channel> :They aren't on that channel"
    /// description:
    ///  Returned by the server to indicate that the target
    ///  user of the command is not on the given channel.
    ERR_USERNOTINCHANNEL = 441,

    /// example: "<channel> :You're not on that channel"
    /// description:
    ///  Returned by the server whenever a client tries to
    ///  perform a channel effecting command for which the
    ///  client isn't a member.
    ERR_NOTONCHANNEL = 442,

    /// example: "<user> <channel> :is already on channel"
    /// description:
    ///  Returned when a client tries to invite a user to a
    ///  channel they are already on.
    ERR_USERONCHANNEL = 443,

    /// example: "<user> :User not logged in"
    /// description:
    ///  Returned by the summon after a SUMMON command for a
    ///  user was unable to be performed since they were not
    ///  logged in.
    ERR_NOLOGIN = 444,

    /// example: ":SUMMON has been disabled"
    /// description:
    ///  Returned as a response to the SUMMON command.  Must be
    ///  returned by any server which does not implement it.
    ERR_SUMMONDISABLED = 445,

    /// example: ":USERS has been disabled"
    /// description:
    ///  Returned as a response to the USERS command.  Must be
    ///  returned by any server which does not implement it.
    ERR_USERSDISABLED = 446,

    /// example: ":You have not registered"
    /// description:
    ///  Returned by the server to indicate that the client
    ///  must be registered before the server will allow it
    ///  to be parsed in detail.
    ERR_NOTREGISTERED = 451,

    /// example: "<command> :Not enough parameters"
    /// description:
    ///  Returned by the server by numerous commands to
    ///  indicate to the client that it didn't supply enough
    ///  parameters.
    ERR_NEEDMOREPARAMS = 461,

    /// example: ":You may not reregister"
    /// description:
    ///  Returned by the server to any link which tries to
    ///  change part of the registered details (such as
    ///  password or user details from second USER message).
    ///  
    ERR_ALREADYREGISTRED = 462,

    /// example: ":Your host isn't among the privileged"
    /// description:
    ///  Returned to a client which attempts to register with
    ///  a server which does not been setup to allow
    ///  connections from the host the attempted connection
    ///  is tried.
    ERR_NOPERMFORHOST = 463,

    /// example: ":Password incorrect"
    /// description:
    ///  Returned to indicate a failed attempt at registering
    ///  a connection for which a password was required and
    ///  was either not given or incorrect.
    ERR_PASSWDMISMATCH = 464,

    /// example: ":You are banned from this server"
    /// description:
    ///  Returned after an attempt to connect and register
    ///  yourself with a server which has been setup to
    ///  explicitly deny connections to you.
    ERR_YOUREBANNEDCREEP = 465,

    /// example: "<channel> :Channel key already set"
    ERR_KEYSET = 467,

    /// example: "<channel> :Cannot join channel (+l)"
    ERR_CHANNELISFULL = 471,

    /// example: "<char> :is unknown mode char to me"
    ERR_UNKNOWNMODE = 472,

    /// example: "<channel> :Cannot join channel (+i)"
    ERR_INVITEONLYCHAN = 473,

    /// example: "<channel> :Cannot join channel (+b)"
    ERR_BANNEDFROMCHAN = 474,

    /// example: "<channel> :Cannot join channel (+k)"
    ERR_BADCHANNELKEY = 475,

    /// example: ":Permission Denied- You're not an IRC operator"
    /// description:
    ///  Any command requiring operator privileges to operate
    ///  must return this error to indicate the attempt was
    ///  unsuccessful.
    ERR_NOPRIVILEGES = 481,

    /// example: "<channel> :You're not channel operator"
    /// description:
    ///  Any command requiring 'chanop' privileges (such as
    ///  MODE messages) must return this error if the client
    ///  making the attempt is not a chanop on the specified
    ///  channel.
    ERR_CHANOPRIVSNEEDED = 482,

    /// example: ":You cant kill a server!"
    /// description:
    ///  Any attempts to use the KILL command on a server
    ///  are to be refused and this error returned directly
    ///  to the client.
    ERR_CANTKILLSERVER = 483,

    /// example: ":No O-lines for your host"
    /// description:
    ///  If a client sends an OPER message and the server has
    ///  not been configured to allow connections from the
    ///  client's host as an operator, this error must be
    ///  returned.
    ERR_NOOPERHOST = 491,

    /// example: ":Unknown MODE flag"
    /// description:
    ///  Returned by the server to indicate that a MODE
    ///  message was sent with a nickname parameter and that
    ///  the a mode flag sent was not recognized.
    ERR_UMODEUNKNOWNFLAG = 501,

    /// example: ":Cant change mode for other users"
    /// description:
    ///  Error sent to any user trying to view or change the
    ///  user mode for a user other than themselves
    ERR_USERSDONTMATCH = 502,

    /// example: Dummy reply number. Not used.
    RPL_NONE = 300,

    /// example: ":[<reply>{<space><reply>}]"
    /// description:
    ///  Reply format used by USERHOST to list replies to
    ///  the query list.  The reply string is composed as
    ///  follows:
    ///  
    ///  <reply> ::= <nick>['*'] '=' <'+'|'-'><hostname>
    ///  
    ///  The '*' indicates whether the client has registered
    ///  as an Operator.  The '-' or '+' characters represent
    ///  whether the client has set an AWAY message or not
    ///  respectively.
    RPL_USERHOST = 302,

    /// example: ":[<nick> {<space><nick>}]"
    /// description:
    ///  Reply format used by ISON to list replies to the
    ///  query list.
    RPL_ISON = 303,

    /// example: "<nick> :<away message>"
    RPL_AWAY = 301,

    /// example: ":You are no longer marked as being away"
    RPL_UNAWAY = 305,

    /// example: ":You have been marked as being away"
    /// description:
    ///  These replies are used with the AWAY command (if
    ///  allowed).  RPL_AWAY is sent to any client sending a
    ///  PRIVMSG to a client which is away.  RPL_AWAY is only
    ///  sent by the server to which the client is connected.
    ///  Replies RPL_UNAWAY and RPL_NOWAWAY are sent when the
    ///  client removes and sets an AWAY message.
    RPL_NOWAWAY = 306,

    /// example: "<nick> <user> <host> * :<real name>"
    RPL_WHOISUSER = 311,

    /// example: "<nick> <server> :<server info>"
    RPL_WHOISSERVER = 312,

    /// example: "<nick> :is an IRC operator"
    RPL_WHOISOPERATOR = 313,

    /// example: "<nick> <integer> :seconds idle"
    RPL_WHOISIDLE = 317,

    /// example: "<nick> :End of /WHOIS list"
    RPL_ENDOFWHOIS = 318,

    /// example: "<nick> :{[@|+]<channel><space>}"
    /// description:
    ///  Replies 311 - 313, 317 - 319 are all replies
    ///  generated in response to a WHOIS message.  Given that
    ///  there are enough parameters present, the answering
    ///  server must either formulate a reply out of the above
    ///  numerics (if the query nick is found) or return an
    ///  error reply.  The '*' in RPL_WHOISUSER is there as
    ///  the literal character and not as a wild card.  For
    ///  each reply set, only RPL_WHOISCHANNELS may appear
    ///  more than once (for long lists of channel names).
    ///  The '@' and '+' characters next to the channel name
    ///  indicate whether a client is a channel operator or
    ///  has been granted permission to speak on a moderated
    ///  channel.  The RPL_ENDOFWHOIS reply is used to mark
    ///  the end of processing a WHOIS message.
    RPL_WHOISCHANNELS = 319,

    /// example: "<nick> <user> <host> * :<real name>"
    RPL_WHOWASUSER = 314,

    /// example: "<nick> :End of WHOWAS"
    /// description:
    ///  When replying to a WHOWAS message, a server must use
    ///  the replies RPL_WHOWASUSER, RPL_WHOISSERVER or
    ///  ERR_WASNOSUCHNICK for each nickname in the presented
    ///  list.  At the end of all reply batches, there must
    ///  be RPL_ENDOFWHOWAS (even if there was only one reply
    ///  and it was an error).
    RPL_ENDOFWHOWAS = 369,

    /// example: "Channel :Users  Name"
    RPL_LISTSTART = 321,

    /// example: "<channel> <# visible> :<topic>"
    RPL_LIST = 322,

    /// example: ":End of /LIST"
    /// description:
    ///  Replies RPL_LISTSTART, RPL_LIST, RPL_LISTEND mark
    ///  the start, actual replies with data and end of the
    ///  server's response to a LIST command.  If there are
    ///  no channels available to return, only the start
    ///  and end reply must be sent.
    RPL_LISTEND = 323,

    /// example: "<channel> <mode> <mode params>"
    RPL_CHANNELMODEIS = 324,

    /// example: "<channel> :No topic is set"
    RPL_NOTOPIC = 331,

    /// example: "<channel> :<topic>"
    /// description:
    ///  When sending a TOPIC message to determine the
    ///  channel topic, one of two replies is sent.  If
    ///  the topic is set, RPL_TOPIC is sent back else
    ///  RPL_NOTOPIC.
    RPL_TOPIC = 332,

    /// example: "<channel> <nick>"
    /// description:
    ///  Returned by the server to indicate that the
    ///  attempted INVITE message was successful and is
    ///  being passed onto the end client.
    RPL_INVITING = 341,

    /// example: "<user> :Summoning user to IRC"
    /// description:
    ///  Returned by a server answering a SUMMON message to
    ///  indicate that it is summoning that user.
    RPL_SUMMONING = 342,

    /// example: "<version>.<debuglevel> <server> :<comments>"
    /// description:
    ///  Reply by the server showing its version details.
    ///  The <version> is the version of the software being
    ///  used (including any patchlevel revisions) and the
    ///  <debuglevel> is used to indicate if the server is
    ///  running in "debug mode".
    ///  
    ///  The "comments" field may contain any comments about
    ///  the version or further version details.
    RPL_VERSION = 351,

    /// example: "<channel> <user> <host> <server> <nick> <H|G>[*][@|+] :<hopcount> <real name>"
    RPL_WHOREPLY = 352,

    /// example: "<name> :End of /WHO list"
    /// description:
    ///  The RPL_WHOREPLY and RPL_ENDOFWHO pair are used
    ///  to answer a WHO message.  The RPL_WHOREPLY is only
    ///  sent if there is an appropriate match to the WHO
    ///  query.  If there is a list of parameters supplied
    ///  with a WHO message, a RPL_ENDOFWHO must be sent
    ///  after processing each list item with <name> being
    ///  the item.
    RPL_ENDOFWHO = 315,

    /// example: "<channel> :[[@|+]<nick> [[@|+]<nick> [...]]]"
    RPL_NAMREPLY = 353,

    /// example: "<channel> :End of /NAMES list"
    /// description:
    ///  To reply to a NAMES message, a reply pair consisting
    ///  of RPL_NAMREPLY and RPL_ENDOFNAMES is sent by the
    ///  server back to the client.  If there is no channel
    ///  found as in the query, then only RPL_ENDOFNAMES is
    ///  returned.  The exception to this is when a NAMES
    ///  message is sent with no parameters and all visible
    ///  channels and contents are sent back in a series of
    ///  RPL_NAMEREPLY messages with a RPL_ENDOFNAMES to mark
    ///  the end.
    RPL_ENDOFNAMES = 366,

    /// example: "<mask> <server> :<hopcount> <server info>"
    RPL_LINKS = 364,

    /// example: "<mask> :End of /LINKS list"
    /// description:
    ///  In replying to the LINKS message, a server must send
    ///  replies back using the RPL_LINKS numeric and mark the
    ///  end of the list using an RPL_ENDOFLINKS reply.
    RPL_ENDOFLINKS = 365,

    /// example: "<channel> <banid>"
    RPL_BANLIST = 367,

    /// example: "<channel> :End of channel ban list"
    /// description:
    ///  When listing the active 'bans' for a given channel,
    ///  a server is required to send the list back using the
    ///  RPL_BANLIST and RPL_ENDOFBANLIST messages.  A separate
    ///  RPL_BANLIST is sent for each active banid.  After the
    ///  banids have been listed (or if none present) a
    ///  RPL_ENDOFBANLIST must be sent.
    RPL_ENDOFBANLIST = 368,

    /// example: ":<string>"
    RPL_INFO = 371,

    /// example: ":End of /INFO list"
    /// description:
    ///  A server responding to an INFO message is required to
    ///  send all its 'info' in a series of RPL_INFO messages
    ///  with a RPL_ENDOFINFO reply to indicate the end of the
    ///  replies.
    RPL_ENDOFINFO = 374,

    /// example: ":- <server> Message of the day - "
    RPL_MOTDSTART = 375,

    /// example: ":- <text>"
    RPL_MOTD = 372,

    /// example: ":End of /MOTD command"
    /// description:
    ///  When responding to the MOTD message and the MOTD file
    ///  is found, the file is displayed line by line, with
    ///  each line no longer than 80 characters, using
    ///  RPL_MOTD format replies.  These should be surrounded
    ///  by a RPL_MOTDSTART (before the RPL_MOTDs) and an
    ///  RPL_ENDOFMOTD (after).
    RPL_ENDOFMOTD = 376,

    /// example: ":You are now an IRC operator"
    /// description:
    ///  RPL_YOUREOPER is sent back to a client which has
    ///  just successfully issued an OPER message and gained
    ///  operator status.
    RPL_YOUREOPER = 381,

    /// example: "<config file> :Rehashing"
    /// description:
    ///  If the REHASH option is used and an operator sends
    ///  a REHASH message, an RPL_REHASHING is sent back to
    ///  the operator.
    RPL_REHASHING = 382,

    /// example: "<server> :<string showing server's local time>"
    /// description:
    ///  When replying to the TIME message, a server must send
    ///  the reply using the RPL_TIME format above.  The string
    ///  showing the time need only contain the correct day and
    ///  time there.  There is no further requirement for the
    ///  time string.
    RPL_TIME = 391,

    /// example: ":UserID   Terminal  Host"
    RPL_USERSSTART = 392,

    /// example: ":%-8s %-9s %-8s"
    RPL_USERS = 393,

    /// example: ":End of users"
    RPL_ENDOFUSERS = 394,

    /// example: ":Nobody logged in"
    /// description:
    ///  If the USERS message is handled by a server, the
    ///  replies RPL_USERSTART, RPL_USERS, RPL_ENDOFUSERS and
    ///  RPL_NOUSERS are used.  RPL_USERSSTART must be sent
    ///  first, following by either a sequence of RPL_USERS
    ///  or a single RPL_NOUSER.  Following this is
    ///  RPL_ENDOFUSERS.
    RPL_NOUSERS = 395,

    /// example: "Link <version & debug level> <destination> <next server>"
    RPL_TRACELINK = 200,

    /// example: "Try. <class> <server>"
    RPL_TRACECONNECTING = 201,

    /// example: "H.S. <class> <server>"
    RPL_TRACEHANDSHAKE = 202,

    /// example: "???? <class> [<client IP address in dot form>]"
    RPL_TRACEUNKNOWN = 203,

    /// example: "Oper <class> <nick>"
    RPL_TRACEOPERATOR = 204,

    /// example: "User <class> <nick>"
    RPL_TRACEUSER = 205,

    /// example: "Serv <class> <int>S <int>C <server> <nick!user|*!*>@<host|server>"
    RPL_TRACESERVER = 206,

    /// example: "<newtype> 0 <client name>"
    RPL_TRACENEWTYPE = 208,

    /// example: "File <logfile> <debug level>"
    /// description:
    ///  The RPL_TRACE* are all returned by the server in
    ///  response to the TRACE message.  How many are
    ///  returned is dependent on the the TRACE message and
    ///  whether it was sent by an operator or not.  There
    ///  is no predefined order for which occurs first.
    ///  Replies RPL_TRACEUNKNOWN, RPL_TRACECONNECTING and
    ///  RPL_TRACEHANDSHAKE are all used for connections
    ///  which have not been fully established and are either
    ///  unknown, still attempting to connect or in the
    ///  process of completing the 'server handshake'.
    ///  RPL_TRACELINK is sent by any server which handles
    ///  a TRACE message and has to pass it on to another
    ///  server.  The list of RPL_TRACELINKs sent in
    ///  response to a TRACE command traversing the IRC
    ///  network should reflect the actual connectivity of
    ///  the servers themselves along that path.
    ///  RPL_TRACENEWTYPE is to be used for any connection
    ///  which does not fit in the other categories but is
    ///  being displayed anyway.
    RPL_TRACELOG = 261,

    /// example: "<linkname> <sendq> <sent messages> <sent bytes> <received messages> <received bytes> <time open>"
    RPL_STATSLINKINFO = 211,

    /// example: "<command> <count>"
    RPL_STATSCOMMANDS = 212,

    /// example: "C <host> * <name> <port> <class>"
    RPL_STATSCLINE = 213,

    /// example: "N <host> * <name> <port> <class>"
    RPL_STATSNLINE = 214,

    /// example: "I <host> * <host> <port> <class>"
    RPL_STATSILINE = 215,

    /// example: "K <host> * <username> <port> <class>"
    RPL_STATSKLINE = 216,

    /// example: "Y <class> <ping frequency> <connect frequency> <max sendq>"
    RPL_STATSYLINE = 218,

    /// example: "<stats letter> :End of /STATS report"
    RPL_ENDOFSTATS = 219,

    /// example: "L <hostmask> * <servername> <maxdepth>"
    RPL_STATSLLINE = 241,

    /// example: ":Server Up %d days %d:%02d:%02d"
    RPL_STATSUPTIME = 242,

    /// example: "O <hostmask> * <name>"
    RPL_STATSOLINE = 243,

    /// example: "H <hostmask> * <servername>"
    RPL_STATSHLINE = 244,

    /// example: "<user mode string>"
    /// description:
    ///  To answer a query about a client's own mode,
    ///  RPL_UMODEIS is sent back.
    RPL_UMODEIS = 221,

    /// example: ":There are <integer> users and <integer> invisible on <integer> servers"
    RPL_LUSERCLIENT = 251,

    /// example: "<integer> :operator(s) online"
    RPL_LUSEROP = 252,

    /// example: "<integer> :unknown connection(s)"
    RPL_LUSERUNKNOWN = 253,

    /// example: "<integer> :channels formed"
    RPL_LUSERCHANNELS = 254,

    /// example: ":I have <integer> clients and <integer> servers"
    /// description:
    ///  In processing an LUSERS message, the server
    ///  sends a set of replies from RPL_LUSERCLIENT,
    ///  RPL_LUSEROP, RPL_USERUNKNOWN,
    ///  RPL_LUSERCHANNELS and RPL_LUSERME.  When
    ///  replying, a server must send back
    ///  RPL_LUSERCLIENT and RPL_LUSERME.  The other
    ///  replies are only sent back if a non-zero count
    ///  is found for them.
    RPL_LUSERME = 255,

    /// example: "<server> :Administrative info"
    RPL_ADMINME = 256,

    /// example: ":<admin info>"
    RPL_ADMINLOC1 = 257,

    /// example: ":<admin info>"
    RPL_ADMINLOC2 = 258,

    /// example: ":<admin info>"
    /// description:
    ///  When replying to an ADMIN message, a server
    ///  is expected to use replies RLP_ADMINME
    ///  through to RPL_ADMINEMAIL and provide a text
    ///  message with each.  For RPL_ADMINLOC1 a
    ///  description of what city, state and country
    ///  the server is in is expected, followed by
    ///  details of the university and department
    ///  (RPL_ADMINLOC2) and finally the administrative
    ///  contact for the server (an email address here
    ///  is required) in RPL_ADMINEMAIL.
    ///  
    RPL_ADMINEMAIL = 259,

};