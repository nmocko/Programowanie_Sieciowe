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
#include        <string.h>
#include        <stdio.h>

#define MAX_PLAYERS 10
#define MAX_NICK_LENGTH 20
#define RANKING_FILE "ranking"

int check_ranking(const char* nickname, double seconds);

int count_lines(const char *filename);

char* draw_word(const char *filename);

char* create_underscore_word(char *word);

int check_letter(char letter, char* underscore_word, char* word);

