
#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>        /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>       /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <syslog.h>
#include        <unistd.h>
#include        <dirent.h>
#include        <stdint.h>


#define MAXLINE 1024
#define MAX_NICKNAME_LENGTH 20
#define MAX_MULTICAST_MESSAGE_LENGTH 1024


void game(int sockfd, int multicastfd);

void play_game(int sockfd, int multicastfd, char* nickname);

void send_announcement(int multicastfd, const char *name);

void view_ranking(int sockfd);


