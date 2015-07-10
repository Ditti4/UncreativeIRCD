#ifndef __INTERNAL_H
#define __INTERNAL_H

/*
 * +------------------+
 * |    internal.h    |
 * +------------------+
 * 
 * This module provides provides internal functions used by
 * other modules of the ircd. The ircd should never, ever
 * use these functions directly – they are not error safe and
 * thus are very likely to fail on some error. Don't ever use
 * these functions if you're not writing a module including
 * your own error checking.
 * 
 */

extern int errid; // if == 0 then no error, if > 0 then we should gai_strerror() it, otherwise errno is important

extern fd_set g_master;
extern fd_set g_read_fds;

extern int g_sockfd;
extern int g_max_sock;

extern user_t *users;
extern int usercount;

extern channel_t *channels;
extern int channelcount;

extern char g_hostname[];

int send_from_user_to_everyone(int userindex, char *message);
int send_from_user_to_everyone_include_user(int userindex, char *message);
int send_from_user_to_channel(int userindex, int channelindex,char *message);
int send_from_user_to_channel_include_user(int userindex, int channelindex, char *message);
int send_from_user_to_user(int userindex, int userindex2, char *message);
int send_from_server_to_everyone(char *message);
int send_from_server_to_channel(int channelindex, char *message);
int send_from_server_to_user(int userindex, char *message);
int create_channel(char *name);
int create_user(int sockfd, char *hostname);
int remove_empty_channel(int channelindex);
int remove_channel(int channelindex);
int remove_user_from_channel(int userindex, int channelindex);
int remove_user_from_network(int userindex);
int add_user_to_channel(int userindex, int channelindex);
int is_user_in_channel_with_user(int userindex, int userindex2);
int is_user_in_channel(int userindex, int channelindex);
int *get_matching_nicks(char *mask);
int *get_matching_hostnames(char *mask);
int *get_matching_realnames(char *mask);
int *get_matching_idents(char *mask);
int *get_matching_channels(char *mask);
int wildcard_match(char *string, char *mask);

#endif
