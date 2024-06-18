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
#include        <netinet/in.h>
#include        <sys/wait.h>

#include "read_write.h"
#include "multicast.h"
#include "game.h"

#define MAXLINE 1024

#define LISTENQ 2


#define MAXFD   64


void sig_chld(int signo) {
	pid_t	pid;
	int		stat;

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        syslog(LOG_INFO, "child %d terminated\n", pid);

	return;
}

void sig_pipe(int signo) {
    syslog(LOG_INFO, "Server received SIGPIPE - Default action is exit \n");
    
	exit(1);
}


int daemon_init(const char *pname, int facility, uid_t uid, int socket){

    int     i, p;
    pid_t   pid;


    if ( (pid = fork()) < 0)
        return (-1);
    else if (pid)
        exit(0);  /* parent terminates */


    /* child 1 continues... */


    if (setsid() < 0)  /* become session leader */
        return (-1);


    signal(SIGHUP, SIG_IGN);
    if ((pid = fork()) < 0)
        return (-1);
    else if (pid)
        exit(0);  /* child 1 terminates */


    /* child 2 continues... */

    chroot("/tmp");  /* change working directory  or chroot()*/

    /* close off file descriptors */
    for (i = 0; i < MAXFD; i++){
        if(socket != i)
            close(i);
    }


    /* redirect stdin, stdout, and stderr to /dev/null */
    p= open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

  
    syslog(LOG_ERR," STDIN = %i\n", p);
    setuid(uid);  /* change user */
    
    return (0);  /* success */
}
//----------------------


int main(int argc, char **argv){

    // Setting up logging level
    setlogmask(LOG_UPTO (LOG_INFO));
    openlog ("Hangman_server", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);


    int                 listenfd, connfd, multicastfd;
    socklen_t           len;
    char                buff[MAXLINE], str[INET_ADDRSTRLEN+1];
    struct sockaddr_in servaddr, cliaddr;
    struct sigaction new_action, old_action;

    // Handling SIGCHLD signal
    new_action.sa_handler = sig_chld;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    if(sigaction (SIGCHLD, &new_action, &old_action) < 0 ){
        syslog (LOG_ERR, "sigaction error : %s\n", strerror(errno));
        return 1;
    }

    // Setting up multicast
    const char *serv = "224.2.2.2";
    int port = 5554;
    const char *interface = "wlo1";

    multicastfd = setup_multicast(serv, port, &saptr, &lenp, interface);
    syslog (LOG_INFO, "Multicast socket setup complete on interface %s\n", interface);


    // socket()
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        syslog (LOG_ERR, "socket error : %s\n", strerror(errno));
        return 1;
    }

    int optval = 1;               
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
        syslog (LOG_ERR, "SO_REUSEADDR setsockopt error : %s\n", strerror(errno));
    }

    // sockaddr_in "servaddr"
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    if(argc == 1)
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    else{
        if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1 ){
            syslog(LOG_ERR, "ERROR: Address format error\n");
            return -1;
        }
    }
    servaddr.sin_port = htons(5450);

    // bind(), listen()
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        syslog(LOG_ERR, "bind error : %s\n", strerror(errno));
        return 1;
    }


    if (listen(listenfd, LISTENQ) < 0){
        syslog(LOG_ERR, "listen error : %s\n", strerror(errno));
        return 1;
    }


    // Creating daemon
    daemon_init(argv[0], LOG_USER, 1000, listenfd);
    syslog(LOG_NOTICE, "Program started by User %d", getuid());
    syslog(LOG_INFO,"Waiting for clients ... ");
    
    for ( ; ; ) {

        // Accepting connection from client
        len = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0){
            syslog(LOG_ERR,"accept error : %s\n", strerror(errno));
            continue;
        }


        bzero(str, sizeof(str));
        inet_ntop(AF_INET, (struct sockaddr  *) &cliaddr.sin_addr,  str, sizeof(str));
        syslog(LOG_INFO,"Connection from %s\n", str);


        int childpid;
		if ( (childpid = fork()) == 0) {  /* child process */
			close(listenfd);  /* close listening socket */
			game(connfd, multicastfd);  /* playing the game */
			exit(0);
		}
		close(connfd);  /* parent closes connected socket */

    }

    closelog();
    close(multicastfd);
}
