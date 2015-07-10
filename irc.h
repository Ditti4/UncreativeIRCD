#ifndef __IRC_H
#define __IRC_H

/*
 * +-------------+
 * |    irc.h    |
 * +-------------+
 *
 * This module provides a more or less user friendly function
 * collection which is used directly by the ircd. The functions
 * in here wrap the functions found in internal.h and include
 * some important error checking and thus should be the only
 * functions to ever use the stuff found in internal.h.
 *
 */

#define LINE_LENGTH 513
// 513 because RFC says 512 including the \r and \n (in that order), now add the usual \0 for strings in C to that and we're at 513

#define ISVALIDUSER(A)      (A >= 0 && users[A].nick[0] != '\0' && users[A].ident[0] != '\0' && users[A].realname[0] != '\0')
#define ISVALIDCHANNEL(A)   (A >= 0 && channels[A].name[0] != '\0')

enum PART_TYPE { // I have no clue what I thought when adding this and at this point I'm too afraid to remove it … just gonna keep it, I guess, maybe nobody will ever notice
    TYPE_QUIT,
    TYPE_PART,
    TYPE_KICK,
};

typedef struct __user user_t;
typedef struct __channel channel_t;

struct __user {
    char *nick;
    char *realname;
    char *ident; // aka. user, should use a static size, too
    char *hostname;
    int sockfd; // for iteration
    int channelcount;
    long int last_ping;
    long int last_pong;
    channel_t *channels; // for whois data
};

struct __channel {
    char *name;
    char *topic; // use static size instead of pointer here
    char *modes;
    fd_set fds; // for sending and receiving
    int usercount;
    user_t *users; // for user list of channel
};

extern int errid; // if == 0 then no error, if > 0 then we should gai_strerror() it, otherwise errno is important

extern fd_set g_master;
extern fd_set g_read_fds;

extern int g_sockfd;
extern int g_max_sock;

extern user_t *users;
extern int usercount;

extern channel_t *channels;
extern int channelcount;

int find_channel(char *name);
int find_user(char *name);
int find_user_by_socket(int sockfd);
int join_channel(int userindex, char *channel);
int fake_join(int userindex, int channelindex);
int part_channel(int userindex, int channelindex, char *partmessage);
int kick_user_from_channel(int kicked_by, int userindex, int channelindex, char *reason);
int quit_user(int userindex, char *quitmessage);
int fake_quit(int userindex, char *quitmessage);
int privmsg(int userindex, char *destination, char *message);
int notice(int userindex, char *destination, char *message);
int set_ident(int userindex, char *ident, int force_flag);
int set_realname(int userindex, char *realname, int force_flag);
int set_hostname(int userindex, char *hostname);
int set_nick(int userindex, char *nick);
int who(int userindex, char *mask);
void send_welcome_replies(int userindex);

#endif
