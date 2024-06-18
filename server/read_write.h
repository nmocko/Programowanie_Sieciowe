

#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <syslog.h>
#include        <unistd.h>
#include        <dirent.h>


#define MAXLINE 1024


typedef struct {
    uint8_t type;
    uint16_t length;
    void *value;
} TLV;

ssize_t Writen(int fd, const void *vptr, size_t n);

void* encode_tlv(uint8_t type, uint16_t length, void *value, uint16_t *encoded_length);

int send_tlv(int sockfd, uint8_t type, uint16_t length, void *value);

ssize_t Readn(int sockfd, void *buf, size_t n);

void decode_tlv(void *encoded, TLV *tlv);

void recv_tlv(int sockfd, TLV *tlv);

