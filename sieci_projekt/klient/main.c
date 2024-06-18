#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define ASCII_ESC 27

typedef struct {
    uint8_t type;
    uint16_t length;
    char *value;
} TLV;

void snd_tlv(uint8_t type, uint16_t length, void *value, uint16_t *encoded_length, int socket) {
    // Alokowanie pamięci dla zakodowanych danych
    *encoded_length = 1 + 2 + length; // Type (1 byte) + Length (2 bytes) + Value
    uint8_t *encoded = (uint8_t *)malloc(*encoded_length);

    // Kodowanie typu
    encoded[0] = type;

    // Kodowanie długości (wielkość w bajtach)
    encoded[1] = (length >> 8) & 0xFF; // Pierwszy bajt długości
    encoded[2] = length & 0xFF;        // Drugi bajt długości

    // Kodowanie wartości
    memcpy(encoded + 3, value, length);
    // printf("\n\n\nsizeof encoded %ld\n", sizeof(encoded));
    if(write(socket, encoded, length+3) < 0) {
            fprintf(stderr, "write error (choose the letter): %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
}

void rcv_tlv(TLV *tlv, int socket) {
    uint8_t data[3];
    // char * buffer[3];

    if (read(socket, data, sizeof(data)) < 0) {
        fprintf(stderr, "read error tlv header: %s\n", strerror(errno));
    }

    // Dekodowanie typu
    tlv->type = data[0];

    // Dekodowanie długości
    tlv->length = (data[1] << 8) | data[2];

    // Alokowanie pamięci dla wartości i kopiowanie wartości
    tlv->value = malloc(tlv->length + 1);

    if (read(socket, tlv->value, tlv->length) < 0) {
        fprintf(stderr, "read error tvl->value: %s\n", strerror(errno));
    }

    tlv->value[tlv->length] = '\0';
}

void print_address(struct addrinfo *rp) {
    char addr_str[INET6_ADDRSTRLEN]; // Wystarczająco duży dla IPv4 i IPv6
    void *addr;
    
    // Pobierz adres z odpowiedniej struktury sockaddr
    if (rp->ai_family == AF_INET) { // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
        addr = &(ipv4->sin_addr);
    } else { // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
        addr = &(ipv6->sin6_addr);
    }
    
    // Konwertuj adres na postać tekstową
    inet_ntop(rp->ai_family, addr, addr_str, sizeof(addr_str));
    // printf("address: %s\n", addr_str);
}

int connect_to_hangman() {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */
    int s, sfd;

    s = getaddrinfo("hangman-server", "hangman", &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        // print_address(rp);
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        // printf("address: %s\n", rp->ai_canonname);
        if (sfd == -1) {
            continue;
        }
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;                  /* Success */
        }
        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); 
    // printf("Connected to the server");
    return sfd;
}

int connect_to_announcements() {

    struct sockaddr_in localAddr;
    struct ip_mreq multicastRequest;
    int multifd;
    if ( (multifd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(5554);

    int opt = 1;
    if (setsockopt(multifd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr,"setsockopt SO_REUSEADDR multicast bind error : %s\n", strerror(errno));
        close(multifd);
        exit(EXIT_FAILURE);
    }


    if (bind(multifd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        fprintf(stderr,"multicast bind error : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    multicastRequest.imr_multiaddr.s_addr = inet_addr("224.2.2.2");
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(multifd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&multicastRequest, sizeof(multicastRequest)) < 0) {
        fprintf(stderr,"adding to multicast group error : %s\n", strerror(errno));
        close(multifd);
        exit(EXIT_FAILURE);
    }

    printf("\033[s\033[fconnected to the announcments\033[u");
    fflush(stdout);
    alarm(5);

    return multifd;

}

void get_nickname(int socket, char * nickname) {
    // char nickanme[30];
    printf("\033[f\033[1B\nWelcome to the hangman game!\n");
    printf("Enter your nickname: ");
    scanf("%s", nickname);
    getchar();

    uint8_t type = 1; //jaki type? jaki type?
    uint16_t length = strlen(nickname);
    uint16_t encoded_length;
    snd_tlv(type, length, nickname, &encoded_length, socket);

}

int hangman(int socket) {
    uint8_t type = 1; //jaki type? jaki type?
    uint16_t length = 1;
    uint16_t encoded_length;
    char letters[26] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};
    snd_tlv(type, length, "1", &encoded_length, socket);


    FILE *fptr;
    fptr = fopen("./animation.txt", "r");
    if(fptr == NULL) {
        fprintf(stderr, "could not open animation.txt file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    char hang1[36];
    char hang2[50];
    size_t n;
    n = fread(hang1, sizeof(char), sizeof(hang1)-1, fptr);
    fread(hang2, 1, sizeof(hang2)-1, fptr);
    hang1[35] = '\0';
    hang2[49] = '\0';

    TLV recive;
    rcv_tlv(&recive, socket);
    printf("\033[f\033[1B\033[J\n");

    printf("Avilable letters: ");
        for (int j=0; j<26; j++) {
            printf("%c ", letters[j]);
        }
        printf("or 1 to exit\n\n");
        printf("%s       ", hang1);
        for (int j=0; j < recive.length; j++) {
            printf("%c ", recive.value[j]);
        }
        printf("%s", hang2);
        printf("\n");

    for (int i=0; i < 11; i++) {

        int choice;
        if (i != 11) {
        while(1) {
            printf("Choose the letter: ");
            choice = getchar();
            getchar();

            if (choice == 49) {
                return 1;
            }
            else if (choice > 64 && choice < 91) {
                choice += 32;
            }
            else if (choice > 96 && choice < 123) {
                if (letters[choice - 97] == '-') {
                    printf("You have already used this letter\n");
                }
                else {
                    letters[choice -97] = '-';
                    break;
                }
            }
            else {
                printf("Incorrect letter!\n");
            }

        }}
        
        snd_tlv(type, length, &choice, &encoded_length, socket);

        // TLV recive;
        rcv_tlv(&recive, socket);
        
        switch(recive.type) {
            case 5: 
                if (i != 11) {
                    n = fread(hang1, sizeof(char), sizeof(hang1)-1, fptr);
                    fread(hang2, 1, sizeof(hang2)-1, fptr);
                    hang1[35] = '\0';
                    hang2[49] = '\0';
                }
                printf("\033[f\033[1B\033[J\n");

                printf("The letter %c is not appear in this word\n", choice);
                break;
            case 6:
                printf("\033[f\033[1B\033[J\n");
                printf("Good choice!\n");
                i -= 1;
                break;
            case 7:
            printf("\033[f\033[1B\033[J\n");
            printf("Avilable letters: ");
                for (int j=0; j<26; j++) {
                    printf("%c ", letters[j]);
                }
                printf("or 1 to exit\n\n");
                printf("%s       ", hang1);
                for (int j=0; j < recive.length; j++) {
                    printf("%c ", recive.value[j]);
                }
                printf("%s", hang2);
                printf("\n");

                // TLV recive;
                // rcv_tlv(&recive, socket);


                printf("\nCongratulation you won!\n");
                printf("Type enter to continue\n");
                getchar();
                return 0;
                break;
            default:
                printf("\033[f\033[1B\033[J");
                fprintf(stderr, "Serwer send incorrect value in hangman function\n");
                exit(EXIT_FAILURE);             

        }

        printf("Avilable letters: ");
        for (int j=0; j<26; j++) {
            printf("%c ", letters[j]);
        }
        printf("or 1 to exit\n\n");
        printf("%s       ", hang1);
        for (int j=0; j < recive.length; j++) {
            printf("%c ", recive.value[j]);
        }
        printf("%s", hang2);
        printf("\n");
    }

    printf("\nYou loosed\n");
    printf("Better luck next time\n");
    printf("Type enter to continue\n");
    getchar();
    return 0;

}

void see_ranking(int socket) {

    uint8_t type = 1; //jaki type? jaki type?
    uint16_t length = 1;
    uint16_t encoded_length;
    snd_tlv(type, length, "2", &encoded_length, socket);

    TLV recive;
    rcv_tlv(&recive, socket);

    printf("\033[f\033[1B\033[J\n"
        "====RANKING====\n");
    printf("%s", recive.value);
    printf("\n\nType enter to continue\n");
    getchar();
}

void clear_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void user_exit(int socket) {

    uint8_t type = 1; //jaki type? jaki type?
    uint16_t length = 1;
    uint16_t encoded_length;
    snd_tlv(type, length, "3", &encoded_length, socket);

}

void menu(int socket, char * nickname) {

    int f = 0;
    int p = 0;
    while(1) {
        if (p == 0) {
            printf("\033[f\033[1B\033[J"
           "\nHi %s!\n"
           "What would you like to do in hangman game?\n"
           "1. Play the game\n"
           "2. See ranking\n"
           "3. Exit\n"
           "\n", nickname);
        }
        printf("Choose the number: ");
        int choice = getchar();
        getchar();
        switch(choice) {
            case '1': 
                hangman(socket);
                p = 0;
                // f = 1;
                break;
            case '2': 
                see_ranking(socket);
                p = 0;
                // f = 1;
                break;
            case '3': 
                user_exit(socket);
                f = 1;
                break;
            default:
                printf("%c is incorrect input. Type 1, 2 or 3\n", choice);
                p = 1;

        }
        if (f == 1) {
            break;
        }
    }

}

void read_announcment(int mtd) {

    while (1) {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        TLV multicast_msg;
        uint8_t data[1024];

        if (recvfrom(mtd, data, sizeof(data), 0, (struct sockaddr *) &addr, &addrlen) < 0) {
            fprintf(stderr, "recivfrom error tlv header multicast: %s\n", strerror(errno));
            fflush(stdout);
        }


        // Dekodowanie typu 
        multicast_msg.type = data[0];

        // Dekodowanie długości
        multicast_msg.length = (data[1] << 8) | data[2];
        
        multicast_msg.value = malloc(multicast_msg.length + 1);
        if (multicast_msg.value == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        memcpy(multicast_msg.value, data + 3, multicast_msg.length);
        multicast_msg.value[multicast_msg.length] = '\0';

        printf("\033[s\033[f%s\033[u", multicast_msg.value);
        fflush(stdout);

        // Zwolnienie zaalokowanej pamięci
        free(multicast_msg.value);
        alarm(5);
    }
}

void comment_problem(int sig) {
    printf("\033[s\033[fA problem has occurred with the comment section\033[u");
    alarm(5);
}

void clear_comment(int sig) {
    printf("\033[s\033[f\033[K\033[u");
}


void sigchld_handler(int sig) {
    int saved_errno = errno;
    // Zapobieganie zablokowaniu w przypadku, gdy wiele procesów zakończyło się jednocześnie
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


int main(int argc, char **argv)
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        exit(1);
    }

    signal(SIGUSR1, comment_problem);
    signal(SIGALRM, clear_comment);

    system("clear");
    pid_t p;
    p = fork();
    if (p<0) {
        fprintf(stderr, "fork error: %s\n", strerror(errno));
    }
    else if (p==0) {
        int mfd;
        mfd = connect_to_announcements();
        read_announcment(mfd);
        kill(getppid(), SIGUSR1);
    }
    else {
        int sfd;
        sfd = connect_to_hangman();
        char nickname[20];
        get_nickname(sfd, nickname);
        menu(sfd, nickname);
    }

    return 0;

}