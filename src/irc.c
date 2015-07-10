#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <sys/types.h>
#include "connection.h"
#include "irc.h"
#include "main.h"
#include "replies.h"
#include "internal.h"
#include "error.h"

// TODO: send correct numeric replies to users so the clients know what's going on

int find_channel(char *name) {
    if(!name) {
        return -1;
    }
    int i;
    for(i = 0; i < channelcount; i++) {
        if(strcmp(channels[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int find_user(char *name) {
    if(!name) {
        return -1;
    }
    int i;
    for(i = 0; i < usercount; i++) {
        if(strcmp(users[i].nick, name) == 0) {
            return i;
        }
    }
    return -1;
}

int find_user_by_socket(int sockfd) {
    int i;
    for(i = 0; i < usercount; i++) {
        if(users[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}

int join_channel(int userindex, char *channel) {
    if(!channel || channel[0] == '\0' || channel[0] == ' ') {
        return ERR_NEEDMOREPARAMS;
    }
    // did the user send all the required information to participate in the irc network yet?
    if(ISVALIDUSER(userindex)) {
        int channelindex = find_channel(channel);
        if(is_user_in_channel(userindex, channelindex)) {
            return ERROR_USER_ALREADY_IN_CHANNEL;
        }
        char *send_message = malloc(LINE_LENGTH * sizeof(char));
        if(channelindex < 0) {
            printf("channel %s doesn't exist yet, creating it\n", channel);
            channelindex = create_channel(channel);
        }
        add_user_to_channel(userindex, channelindex);
        sprintf(send_message, "JOIN %s", channel);
        send_from_user_to_channel_include_user(userindex, channelindex, send_message);

        free(send_message);
        return ERROR_NONE;
    } else {
        return ERROR_INVALID_USER;
    }
}

// int channelindex instead of unsigned char *channel because we don't want to create the channel if it doesn't exist already
// this function is only meant for fake joins so other clients think the user joined, used for hostname change for example
int fake_join(int userindex, int channelindex) {
    if(ISVALIDUSER(userindex)) {
        if(!is_user_in_channel(userindex, channelindex)) {
            return ERROR_USER_NOT_IN_CHANNEL;
        }
        if(!ISVALIDCHANNEL(channelindex)) {
            return ERROR_INVALID_CHANNEL;
        }
        char *send_message = malloc(LINE_LENGTH * sizeof(char));
        sprintf(send_message, "JOIN %s", channels[channelindex].name);
        printf("send_from_user_to_channel(%d, %d) = %d", userindex, channelindex, send_from_user_to_channel(userindex, channelindex, send_message));
        free(send_message);
        return ERROR_NONE;
    } else {
        return ERROR_INVALID_USER;
    }
}

int part_channel(int userindex, int channelindex, char *partmessage) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    if(!is_user_in_channel(userindex, channelindex)) {
        return ERROR_USER_NOT_IN_CHANNEL;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    if(partmessage) {
        sprintf(send_message, "PART %s %s", channels[channelindex].name, partmessage);
    } else {
        sprintf(send_message, "PART %s %s", channels[channelindex].name, users[userindex].nick);
    }
    send_from_user_to_channel(userindex, channelindex, send_message);
    remove_user_from_channel(userindex, channelindex);
    free(send_message);
    return ERROR_NONE;
}

int kick_user_from_channel(int kicked_by, int userindex, int channelindex, char *reason) {
    if(!ISVALIDUSER(userindex) || !ISVALIDUSER(kicked_by)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    if(!is_user_in_channel(userindex, channelindex)) {
        return ERROR_USER_NOT_IN_CHANNEL;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    if(reason) {
        sprintf(send_message, "KICK %s %s %s", channels[channelindex].name, users[userindex].nick, reason);
    } else {
        sprintf(send_message, "KICK %s %s", channels[channelindex].name, users[userindex].nick);
    }
    send_from_user_to_channel(kicked_by, channelindex, send_message);
    remove_user_from_channel(userindex, channelindex);
    free(send_message);
    return ERROR_NONE;
}

int quit_user(int userindex, char *quitmessage) {
    // remove user from every channel he's in, then remove him from the network
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    if(quitmessage) {
        sprintf(send_message, "QUIT %s", quitmessage);
    } else {
        sprintf(send_message, "QUIT %s", users[userindex].nick);
    }
    int i;
    for(i = 0; i < usercount; i++) {
        if(is_user_in_channel_with_user(userindex, i)) {
            send_from_user_to_user(userindex, i, send_message);
        }
    }
    remove_user_from_every_channel(userindex);
    remove_user_from_network(userindex);
    free(send_message);
    return ERROR_NONE;
}

int fake_quit(int userindex, char *quitmessage) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    if(quitmessage) {
        sprintf(send_message, "QUIT %s", quitmessage);
    } else {
        sprintf(send_message, "QUIT %s", users[userindex].nick);
    }
    int i;
    for(i = 0; i < usercount; i++) {
        if(is_user_in_channel_with_user(userindex, i)) {
            send_from_user_to_user(userindex, i, send_message);
        }
    }
    free(send_message);
    return ERROR_NONE;
}

int privmsg(int userindex, char *destination, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    int channeldest = find_channel(destination), userdest = find_user(destination);
    if(!ISVALIDUSER(userdest) && !ISVALIDCHANNEL(channeldest)) {
        return ERROR_INVALID_DESTINATION;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, "PRIVMSG %s %s", destination, message);
    if(channeldest >= 0) {
        send_from_user_to_channel(userindex, channeldest, send_message);
    } else {
        send_from_user_to_user(userindex, userdest, send_message);
    }
    free(send_message);
    return ERROR_NONE;
}

int notice(int userindex, char *destination, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    int channeldest = find_channel(destination), userdest = find_user(destination);
    if(!ISVALIDUSER(userdest) && !ISVALIDCHANNEL(channeldest)) {
        return ERROR_INVALID_DESTINATION;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, "NOTICE %s %s", destination, message);
    if(channeldest >= 0) {
        send_from_user_to_channel(userindex, channeldest, send_message);
    } else {
        send_from_user_to_user(userindex, userdest, send_message);
    }
    free(send_message);
    return ERROR_NONE;
}

// only used on initial registration and when opers want to change it
// may be moved to internal.c since it needs additional checks if the user is oper
int set_ident(int userindex, char *ident, int force_flag) {
    if(ISVALIDUSER(userindex) && !force_flag) {
        return ERR_ALREADYREGISTERED;
    }
    if(!ident) {
        return ERR_NEEDMOREPARAMS;
    }
    strcpy(users[userindex].ident, ident);
    return ERROR_NONE;
}

// only used on initial registration and when opers want to change it
// may be moved to internal.c since it needs additional checks if the user is oper
int set_realname(int userindex, char *realname, int force_flag) {
    if(ISVALIDUSER(userindex) && !force_flag) {
        return ERR_ALREADYREGISTERED;
    }
    if(!realname) {
        return ERR_NEEDMOREPARAMS;
    }
    strcpy(users[userindex].realname, realname);
    return ERROR_NONE;
}

// only used when opers/services want to change it
int set_hostname(int userindex, char *hostname) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!hostname) {
        return ERROR_INVALID_HOSTNAME;
    }
    fake_quit(userindex, "Changing hostname");
    strcpy(users[userindex].hostname, hostname);
    int i;
    for(i = 0; i < users[userindex].channelcount; i++) {
        fake_join(userindex, i);
    }
    return ERROR_NONE;
}

int set_nick(int userindex, char *nick) {
    if(userindex < 0) {
        return ERROR_INVALID_USER;
    }
    if(find_user(nick) > 0) {
        return ERR_NICKNAMEINUSE;
    }
    if(!nick || nick[0] == '\0' || nick[0] == ' ') {
        return ERR_NONICKNAMEGIVEN;
    }
    if(!is_valid_nick(nick)) {
        return ERR_ERRONEUSNICKNAME;
    }
    int was_valid_user = (ISVALIDUSER(userindex) ? 1 : 0);
    // tell everyone that the user is changing his nick
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, "NICK %s", nick);
    int i;
    for(i = 0; i < usercount; i++) {
        if(is_user_in_channel_with_user(userindex, i)) {
            send_from_user_to_user(userindex, i, send_message);
        }
    }
    strcpy(users[userindex].nick, nick);
    if(!was_valid_user && ISVALIDUSER(userindex)) {
        // welcome him!
        send_welcome_replies(userindex);
    }
    free(send_message);
    return ERROR_NONE;
}

int who(int userindex, char *mask) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    // uncomment the following line if you're in the mood to complete the function
    //unsigned char *send_message = malloc(LINE_LENGTH * sizeof(unsigned char));
    if(!mask || mask[0] == '0' || mask[0] == ' ' || mask[0] == '*' || mask[0] == '?') {
        // in case of no mask we should just list every fucking user
        // why, rfc? why?! but okay, we want to stay rfc compliant
 
        return ERROR_NONE;
    }
    // dis gun' be funney now
    // according to the rfc we need to match against the channel first, that's the easiest part
    // but if we don't find a channel we need to match against hostmask, server, realname and nick
    // in that order
    return ERROR_NONE;
}

void send_welcome_replies(int userindex) {
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, "%03d %s :Welcome to the rdircd test network, %s!%s@%s", RPL_WELCOME, users[userindex].nick, users[userindex].nick, users[userindex].ident, users[userindex].hostname);
    send_from_server_to_user(userindex, send_message);
    sprintf(send_message, "%03d %s :Your host is %s running %s", RPL_YOURHOST, users[userindex].nick, g_hostname, VERSION);
    send_from_server_to_user(userindex, send_message);
    sprintf(send_message, "%03d %s :This server was created on Feb. 24 2015", RPL_CREATED, users[userindex].nick);
    send_from_server_to_user(userindex, send_message);
    sprintf(send_message, "%03d %s :%s %s %s %s", RPL_MYINFO, users[userindex].nick, g_hostname, VERSION, "", "");
    send_from_server_to_user(userindex, send_message);
    free(send_message);
}
