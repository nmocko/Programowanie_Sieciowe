
#include "multicast.h"


SA *saptr;
socklen_t lenp;

int snd_udp_socket(const char *serv, int port, SA **saptrarg, socklen_t *lenparg) {
	int sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	struct sockaddr_in *pservaddrv4;

	*saptrarg = malloc( sizeof(struct sockaddr_in));
	pservaddrv4 = (struct sockaddr_in*)*saptrarg;
	bzero(pservaddrv4, sizeof(struct sockaddr_in));

	if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
		syslog (LOG_ERR, "AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
		return -1;
	} else {
		pservaddrv4->sin_family = AF_INET;
		pservaddrv4->sin_port   = htons(port);
		*lenparg =  sizeof(struct sockaddr_in);
		if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			syslog (LOG_ERR, "AF_INET socket error : %s\n", strerror(errno));
			return -1;
		}
	}

	return sockfd;
}


int setup_multicast(const char *serv, int port, SA **saptrarg, socklen_t *lenparg, const char *interface) { 

    int mul_sockfd;

    mul_sockfd = snd_udp_socket(serv, port, saptrarg, lenparg);

    setsockopt(mul_sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface));

	struct ip_mreq        localInterface;
    localInterface.imr_multiaddr.s_addr = inet_addr("224.2.2.2");
	localInterface.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(mul_sockfd, IPPROTO_IP, IP_MULTICAST_IF, (void *)&localInterface, sizeof(localInterface)) < 0) {
        syslog (LOG_ERR, "error in setting local interface at setup_multicast");
        exit(1);
    }

	return mul_sockfd;
}

