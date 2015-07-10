#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <sys/types.h>
#include "connection.h"
#include "irc.h"
#include "internal.h"
#include "error.h"

/*
 * Implementations of the functions defined in
 * internal.h. Be careful when editing them, they are
 * very sensitive and the chance of sudden errors popping
 * up out of nowhere is very high when modifying them.
 *
 */

int send_from_user_to_everyone(int userindex, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    int i;
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s!%s@%s %s\r\n", users[userindex].nick, users[userindex].ident, users[userindex].hostname, message);
    for(i = 0; i <= g_max_sock; i++) {
        if(i != g_sockfd && i != users[userindex].sockfd && FD_ISSET(i, &g_master)) {
            send_until_done(i, send_message, strlen(send_message), 0);
        }
    }
    free(send_message);
    return ERROR_NONE;
}

int send_from_user_to_everyone_include_user(int userindex, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    int i;
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s!%s@%s %s\r\n", users[userindex].nick, users[userindex].ident, users[userindex].hostname, message);
    for(i = 0; i <= g_max_sock; i++) {
        if(i != g_sockfd && FD_ISSET(i, &g_master)) {
            send_until_done(i, send_message, strlen(send_message), 0);
        }
    }
    free(send_message);
    return ERROR_NONE;
}

int send_from_user_to_channel(int userindex, int channelindex, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    int i;
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s!%s@%s %s\r\n", users[userindex].nick, users[userindex].ident, users[userindex].hostname, message);
    /*for(i = 0; i <= g_max_sock; i++) {
        if(i != g_sockfd && i != users[userindex].sockfd && FD_ISSET(i, &(channels[channelindex].fds))) {
            send_until_done(i, send_message, strlen(send_message), 0);
        }
    }*/
    for(i = 0; i < channels[channelindex].usercount; i++) {
        if(i != userindex && FD_ISSET(channels[channelindex].users[i].sockfd, &(channels[channelindex].fds))) {
            send_until_done(channels[channelindex].users[i].sockfd, send_message, strlen(send_message), 0);
        }
    }
    free(send_message);
    return ERROR_NONE;
}

int send_from_user_to_channel_include_user(int userindex, int channelindex, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    int i;
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s!%s@%s %s\r\n", users[userindex].nick, users[userindex].ident, users[userindex].hostname, message);
    /*for(i = 0; i <= g_max_sock; i++) { // TODO: I have trust in my stuff – modify this to use channels[channelindex].usercount and .users[i].sockfd
        if(i != g_sockfd && FD_ISSET(i, &(channels[channelindex].fds))) {
            send_until_done(i, send_message, strlen(send_message), 0);
        }
    }*/
    for(i = 0; i < channels[channelindex].usercount; i++) {
        send_until_done(channels[channelindex].users[i].sockfd, send_message, strlen(send_message), 0);
    }
    free(send_message);
    return ERROR_NONE;
}

int send_from_user_to_user(int userindex, int userindex2, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDUSER(userindex2)) {
        return ERROR_INVALID_USER;
    }
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s!%s@%s %s\r\n", users[userindex].nick, users[userindex].ident, users[userindex].hostname, message);
    if(FD_ISSET(users[userindex2].sockfd, &g_master)) {
        send_until_done(users[userindex2].sockfd, send_message, strlen(send_message), 0);
    }
    free(send_message);
    return ERROR_NONE;
}

int send_from_server_to_everyone(char *message) {
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    int i;
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s %s\r\n", g_hostname, message);
    for(i = 0; i < usercount; i++) {
        if(FD_ISSET(users[i].sockfd, &g_master)) {
            send_until_done(users[i].sockfd, send_message, strlen(send_message), 0);
        }
    }
    free(send_message);
    return ERROR_NONE;
}

int send_from_server_to_channel(int channelindex, char *message) {

}

int send_from_server_to_user(int userindex, char *message) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!message) {
        return ERROR_EMPTY_MESSAGE;
    }
    char *send_message = malloc(LINE_LENGTH * sizeof(char));
    sprintf(send_message, ":%s %s\r\n", g_hostname, message);
    if(FD_ISSET(users[userindex].sockfd, &g_master)) {
        send_until_done(users[userindex].sockfd, send_message, strlen(send_message), 0);
    }
    free(send_message);
    return ERROR_NONE;
}

int create_channel(char *name) {
    channel_t *new_channels = realloc(channels, ++channelcount * sizeof(channel_t));
    if(!new_channels) {
        --channelcount;
        return ERROR_REALLOC_FAILED;
    }
    channels = new_channels;
    channels[channelcount - 1].name = malloc(50 * sizeof(char));
    strcpy(channels[channelcount - 1].name, name);
    channels[channelcount - 1].topic = malloc(100 * sizeof(char));
    strcpy(channels[channelcount - 1].topic, "");
    channels[channelcount - 1].modes = malloc(100 * sizeof(char));
    strcpy(channels[channelcount - 1].modes, "");
    channels[channelcount - 1].usercount = 0;
    channels[channelcount - 1].users = NULL;
    return (channelcount - 1);
}

int create_user(int sockfd, char *hostname) {
    user_t *new_users = realloc(users, ++usercount * sizeof(user_t));
    if(!new_users) {
        --usercount;
        return ERROR_REALLOC_FAILED;
    }
    users = new_users;
    users[usercount - 1].nick = malloc(50 * sizeof(char));
    strcpy(users[usercount - 1].nick, "");
    users[usercount - 1].realname = malloc(100 * sizeof(char));
    strcpy(users[usercount - 1].realname, "");
    users[usercount - 1].ident = malloc(10 * sizeof(char));
    strcpy(users[usercount - 1].ident, "");
    users[usercount - 1].hostname = malloc(NI_MAXHOST * sizeof(char));
    strcpy(users[usercount - 1].hostname, hostname);
    users[usercount - 1].channelcount = 0;
    users[usercount - 1].sockfd = sockfd;
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    users[usercount - 1].last_ping = current_time.tv_sec;
    users[usercount - 1].last_pong = 0;
    users[usercount - 1].channels = NULL;
    return (usercount - 1);
}

int add_user_to_channel(int userindex, int channelindex) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    channel_t *new_channels = realloc(users[userindex].channels, ++(users[userindex].channelcount) * sizeof(channel_t));
    if(!new_channels) {
        --(users[userindex].channelcount);
        free(new_channels);
        return ERROR_REALLOC_FAILED;
    }
    users[userindex].channels = new_channels;

    user_t *new_users = realloc(channels[channelindex].users, ++(channels[channelindex].usercount) * sizeof(user_t));
    if(!new_users) {
        --usercount;
        free(new_users);
        return ERROR_REALLOC_FAILED;
    }
    channels[channelindex].users = new_users;

    users[userindex].channels[users[userindex].channelcount - 1] = channels[channelindex];
    channels[channelindex].users[channels[channelindex].usercount - 1] = users[userindex];

    FD_SET(users[userindex].sockfd, &(channels[channelindex].fds));
    return ERROR_NONE;
}

int remove_empty_channel(int channelindex) {
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    int i;
    for(i = channelindex; i < channelcount - 1; i++) {
        channels[i] = channels[i + 1];
    }
    channels = realloc(channels, --channelcount * sizeof(channel_t));
    return ERROR_NONE;
}

int remove_channel(int channelindex) {
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    // remove channel from every user's user_t.channels, call remove_empty_channel afterwards
    int i;
    for(i = 0; i < usercount; i++) {
        remove_user_from_channel(i, channelindex);
    }

    remove_empty_channel(channelindex);
    return ERROR_NONE;
}

int remove_user_from_channel(int userindex, int channelindex) {
    // move left, realloc, decrement user_t.channelcount and channel_t.usercount
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    if(!ISVALIDCHANNEL(channelindex)) {
        return ERROR_INVALID_CHANNEL;
    }
    int userinchannel = -1, channelinuser = -1;
    // find user in channel_t.users and find channel in user_t.channels
    int i;
    for(i = 0; users[userindex].channels != NULL && i < users[userindex].channelcount; i++) {
        if(strcmp(users[userindex].channels[i].name, channels[channelindex].name) == 0) {
            channelinuser = i;
            break;
        }
    }
    // desired channel not found in user_t.channels
    if(channelinuser == -1) {
        return ERROR_CHANNEL_NOT_IN_USER;
    }

    for(i = 0; channels[channelindex].users != NULL && i < channels[channelindex].usercount; i++) {
        if(strcmp(channels[channelindex].users[i].nick, users[userindex].nick) == 0) {
            userinchannel = i;
            break;
        }
    }
    // desired user not found in channel_t.users
    if(userinchannel == -1) {
        return ERROR_USER_NOT_IN_CHANNEL;
    }

    // move everything to the left and decremt the counters in user_t and channel_t
    // if channel is empty now remove the channel
    users[userindex].channelcount--;
    for(i = channelinuser; i < (users[userindex].channelcount); i++) {
        users[userindex].channels[i] = users[userindex].channels[i + 1];
    }

    channels[channelindex].usercount--;
    for(i = userinchannel; i < (channels[channelindex].usercount); i++) {
        channels[channelindex].users[i] = channels[channelindex].users[i + 1];
    }

    users[userindex].channels = realloc(users[userindex].channels, users[userindex].channelcount * sizeof(channel_t));
    channels[channelindex].users = realloc(channels[channelindex].users, channels[channelindex].usercount * sizeof(user_t));

    FD_CLR(users[userindex].sockfd, &(channels[channelindex].fds));
    return ERROR_NONE;
}

int remove_user_from_every_channel(int userindex) {
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    int i;
    for(i = 0; i < channelcount; i++) {
        remove_user_from_channel(userindex, i);
    }
    return ERROR_NONE;
}

int remove_user_from_network(int userindex) {
    // remove the user from user_t *users, realloc *users, decrement usercount by one – easy
    if(!ISVALIDUSER(userindex)) {
        return ERROR_INVALID_USER;
    }
    FD_CLR(users[userindex].sockfd, &g_master);
    close(users[userindex].sockfd);
    int i;
    for(i = userindex; i < usercount - 1; i++) {
        users[i] = users[i + 1];
    }
    users = realloc(users, --usercount * sizeof(user_t));
    return ERROR_NONE;
}

int is_user_in_channel_with_user(int userindex, int userindex2) {
    if(!ISVALIDUSER(userindex) || !ISVALIDUSER(userindex2) || userindex == userindex2) {
        return 0;
    }
    int i, j;
    for(i = 0; i < users[userindex].channelcount; i++) {
        for(j = 0; j < users[userindex2].channelcount; j++) {
            if(strcmp(users[userindex].channels[i].name, users[userindex2].channels[j].name) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

int is_user_in_channel(int userindex, int channelindex) {
    if(!ISVALIDUSER(userindex) || !ISVALIDCHANNEL(channelindex)) {
        return 0;
    }
    int i;
    for(i = 0; i < channels[channelindex].usercount; i++) {
        if(channels[channelindex].users[i].sockfd == users[userindex].sockfd) {
            return 1;
        }
    }
    return 0;
}

// TODO: function to define which characters are valid for nicknames
int is_valid_nick(char *nick) {

    return 1;
}

// TODO: function to define valid channel name characters
int is_valid_channelname(char *channelname) {

    return 1;
}

int *get_matching_nicks(char *mask) {
    int match_count = 0;
    int *matches = NULL;
    int i;
    for(i = 0; i < usercount; i++) {
        if(wildcard_match(users[i].nick, mask)) {
            int *new_matches = realloc(matches, ++match_count * sizeof(int));
            if(!new_matches) {
                --match_count;
                break;
            }
            matches = new_matches;
            matches[match_count - 1] = i;
        }
    }

    int *new_matches = realloc(matches, (match_count + 1) * sizeof(int));
    if(new_matches) {
        matches = new_matches;
    }
    matches[match_count] = -1;
    return matches;
}

int *get_matching_hostnames(char *mask) {
    int match_count = 0;
    int *matches = NULL;
    int i;
    for(i = 0; i < usercount; i++) {
        if(wildcard_match(users[i].hostname, mask)) {
            int *new_matches = realloc(matches, ++match_count * sizeof(int));
            if(!new_matches) {
                --match_count;
                break;
            }
            matches = new_matches;
            matches[match_count - 1] = i;
        }
    }

    int *new_matches = realloc(matches, (match_count + 1) * sizeof(int));
    if(new_matches) {
        matches = new_matches;
    }
    matches[match_count] = -1;
    return matches;
}

int *get_matching_realnames(char *mask) {
    int match_count = 0;
    int *matches = NULL;
    int i;
    for(i = 0; i < usercount; i++) {
        if(wildcard_match(users[i].realname, mask)) {
            int *new_matches = realloc(matches, ++match_count * sizeof(int));
            if(!new_matches) {
                --match_count;
                break;
            }
            matches = new_matches;
            matches[match_count - 1] = i;
        }
    }

    int *new_matches = realloc(matches, (match_count + 1) * sizeof(int));
    if(new_matches) {
        matches = new_matches;
    }
    matches[match_count] = -1;
    return matches;
}

int *get_matching_idents(char *mask) {
    int match_count = 0;
    int *matches = NULL;
    int i;
    for(i = 0; i < usercount; i++) {
        if(wildcard_match(users[i].ident, mask)) {
            int *new_matches = realloc(matches, ++match_count * sizeof(int));
            if(!new_matches) {
                --match_count;
                break;
            }
            matches = new_matches;
            matches[match_count - 1] = i;
        }
    }

    int *new_matches = realloc(matches, (match_count + 1) * sizeof(int));
    if(new_matches) {
        matches = new_matches;
    }
    matches[match_count] = -1;
    return matches;
}

int *get_matching_channels(char *mask) {
    int match_count = 0;
    int *matches = NULL;
    int i;
    for(i = 0; i < channelcount; i++) {
        if(wildcard_match(channels[i].name, mask)) {
            int *new_matches = realloc(matches, ++match_count * sizeof(int));
            if(!new_matches) {
                --match_count;
                break;
            }
            matches = new_matches;
            matches[match_count - 1] = i;
        }
    }

    int *new_matches = realloc(matches, (match_count + 1) * sizeof(int));
    if(new_matches) {
        matches = new_matches;
    }
    matches[match_count] = -1;
    return matches;
}

int wildcard_match(char *string, char *mask) {
    if(!string || !mask) {
        return 0;
    }
    if(*string == '\0' && *mask == '\0') {
        return 1;
    }
    if(*mask == '*' && *(mask + 1) != '\0' && *string == '\0') {
        return 0;
    }
    if(*mask == '?' || *string == *mask) {
        return wildcard_match(string + 1, mask + 1);
    }
    if(*mask == '*') {
        return (wildcard_match(string + 1, mask) || wildcard_match(string, mask + 1));
    }
    return 0;
}
