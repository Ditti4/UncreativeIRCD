#ifndef __ERROR_H
#define __ERROR_H

enum ERROR_GENERAL {
    ERROR_NONE = 0,
    ERROR_REALLOC_FAILED = -320,
    ERROR_MALLOC_FAILED,
};

enum ERROR_INTERNAL_C {
    ERROR_INVALID_USER = -256,
    ERROR_INVALID_CHANNEL,
    ERROR_EMPTY_MESSAGE,
    ERROR_CHANNEL_NOT_IN_USER,
    ERROR_USER_NOT_IN_CHANNEL,
};

enum ERROR_IRC_C {
    ERROR_INVALID_DESTINATION = -192,
    ERROR_OPER_ONLY,
    ERROR_USER_ALREADY_IN_CHANNEL,
    ERROR_NICK_ALREADY_IN_USE,
    ERROR_INVALID_NICK,
    ERROR_INVALID_HOSTNAME,
    ERROR_INVALID_IDENT,
    ERROR_INVALID_REALNAME,
    ERROR_INVALID_MASK,
};

enum ERROR_CALLBACK_C {
    ERROR_INVALID_CALLBACK_NAME = -128,
    ERROR_INVALID_CALLBACK_FUNCTION,
    ERROR_INVALID_CALLBACK_MESSAGE,
    ERROR_NO_CALLBACK_FOUND,
};

enum ERROR_CONNECTION_C {
    ERROR_GETADDRINFO = -64,
    ERROR_SOCKET,
    ERROR_CONNECT,
    ERROR_BIND,
    ERROR_LISTEN,
    ERROR_ACCEPT,
    ERROR_SELECT,
    ERROR_SETSOCKOPT,
};

#endif