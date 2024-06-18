
#include    <sys/types.h>   /* basic system data types */
#include    <sys/socket.h>  /* basic socket definitions */
#include    <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include    <arpa/inet.h>   /* inet(3) functions */
#include    <errno.h>
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include 	<sys/ioctl.h>
#include    <syslog.h>
#include 	<unistd.h>
#include 	<net/if.h>
#include	<netdb.h>
#include	<sys/utsname.h>
#include	<linux/un.h>
#include    <dirent.h>


#define SA struct sockaddr
extern SA *saptr;
extern socklen_t lenp;


int setup_multicast(const char *serv, int port, SA **saptrarg, socklen_t *lenparg, const char *interface);

int snd_udp_socket(const char *serv, int port, SA **saptrarg, socklen_t *lenparg);