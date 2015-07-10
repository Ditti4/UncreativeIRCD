#ifndef __CALLBACK_FUNCTIONS_H
#define __CALLBACK_FUNCTIONS_H

enum EXIT_TYPES {
    EXIT_SHUTDOWN,
    EXIT_RESTART,
};

extern int errid; // if == 0 then no error, if > 0 then we should gai_strerror() it, otherwise errno is important
extern int exit_code;

extern fd_set g_master;
extern fd_set g_read_fds;

extern int g_sockfd;
extern int g_max_sock;

extern user_t *users;
extern int usercount;

extern channel_t *channels;
extern int channelcount;

void callback_join(int sockfd, char *message);
void callback_part(int sockfd, char *message);
void callback_notice(int sockfd, char *message);
void callback_privmsg(int sockfd, char *message);
void callback_quit(int sockfd, char *message);
void callback_nick(int sockfd, char *message);
void callback_chghost(int sockfd, char *message);
void callback_who(int sockfd, char *message);
void callback_user(int sockfd, char *message);

#endif
