
#include "game.h"
#include "read_write.h"
#include "multicast.h"
#include "utils.h"


void send_announcement(int multicastfd, const char *name) {
    struct sockaddr_in multicast_addr;
    socklen_t addr_len = sizeof(multicast_addr);
    char message[256];

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr("224.2.2.2");
    multicast_addr.sin_port = htons(5554);

    // Creating message in TLV format
    int name_len = strlen(name);
    int message_len = 1 + 2 + name_len;

    uint8_t type = 1;

    uint8_t leng[2];
    leng[0] = (name_len >> 8) & 0xFF;
    leng[1] = name_len & 0xFF;

    memcpy(message, &type, sizeof(type));
    memcpy(message + sizeof(type), &leng, sizeof(leng));
    memcpy(message + sizeof(type) + sizeof(leng), name, name_len);

    // Seding message on multicast address
    ssize_t bytes_sent = sendto(multicastfd, message, message_len, 0,
                                (struct sockaddr *)&multicast_addr, addr_len);
    if (bytes_sent < 0) {
        perror("sendto failed");
        syslog (LOG_ERR, "sendto failed in send_announcement function");
        
    } else {
        syslog(LOG_INFO, "Announcement sent successfully.\n");
    }
}


void view_ranking(int sockfd) {

    FILE *file;
    char buffer[MAXLINE*2] = {0};
    size_t n;

    file = fopen("ranking", "r");
    if (file == NULL) {
        syslog (LOG_ERR, "error while openning file 'ranking'");
        return;
    }

    // Check whether there is any record in the ranking
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) {
        char *no_entries = "No entries";
        send_tlv(sockfd, 1, strlen(no_entries), no_entries);
        fclose(file);
        return;
    }
    fseek(file, 0, SEEK_SET);

    // Reading from file and sending data through socket
    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        
        if (send_tlv(sockfd, 1, strlen(buffer), buffer) != 0) {
            syslog (LOG_ERR, "send_tlv error in view_ranking: %s\n", strerror(errno));
            fclose(file);
            return;
        }
    }

    if (ferror(file)) {
        syslog (LOG_ERR, "fread error in view_ranking\n");
    }

    fclose(file);
}


void play_game(int sockfd, int multicastfd, char* nickname) {

    int i;
    int attempts = 10;

    struct timeval start, end;
    long seconds, useconds;
    double mtime;
    const char *directory = "./users";
    char filepath[256];

    snprintf(filepath, sizeof(filepath), "%s/%s", directory, nickname);
    FILE *file;
    file = fopen(filepath, "w");
    if (file == NULL) {
        syslog (LOG_ERR, "Error while opening player's file");
        return;
    }
    
    TLV user_message;

    // Pick up random word from "words" file
    char *word = draw_word("./words");
    if (word != NULL) {
        syslog (LOG_INFO, "Random word: '%s', for player '%s'\n", word, nickname);
    }

    char *underscore_word = create_underscore_word(word);

    // Send random word (but wist underscores intead of letters) to client. Setup number of attempts by client to be equal to number of letters in (word + 5) 11s
    send_tlv(sockfd, 1, strlen(underscore_word), underscore_word);


    int length = strlen(word);
    attempts = 11;

    // Start measuring time
    gettimeofday(&start, NULL);


    for (i = 0; i < attempts ; i++) {

        // User choice (what does he want to do)
        char choice;
        TLV user_message;
        recv_tlv(sockfd, &user_message);
        choice = *((char*)user_message.value);

        syslog(LOG_INFO, "Player '%s' choice: %c\n", nickname, choice);

        // Guess the letter
        if (choice != '1') {

            // Check if the letter exists in the word. If yes, change relating underscore to this letter
            if(check_letter(choice, underscore_word, word)) {

                if (!strcmp(underscore_word, word)) {
                    
                    // Stop measuring time
                    gettimeofday(&end, NULL);

                    seconds  = end.tv_sec  - start.tv_sec;
                    useconds = end.tv_usec - start.tv_usec;

                    mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
                    syslog (LOG_INFO, "Player '%s' guessed the word '%s' in %f miliseconds\n", nickname, word, mtime);
                    
                    // Check ranking and actualize it if necessary
                    if (check_ranking(nickname, mtime) == 2) {

                        // Send announcement if this player was the fastest one
                        char message[MAX_MULTICAST_MESSAGE_LENGTH - 10];
                        snprintf(message, MAX_MULTICAST_MESSAGE_LENGTH - 10, "New player at the top! Nickname: %s", nickname);
                        syslog(LOG_INFO, "Player '%s' is now at the top.\n", nickname);
                        send_announcement(multicastfd, message);
                    }

                    fprintf(file, "%s %lf\n", word, mtime);
                    send_tlv(sockfd, 7, strlen(underscore_word), underscore_word);
                    free(underscore_word);
                    free(word);
                    fclose(file);
                    return;
                
                } else {
                    send_tlv(sockfd, 6, strlen(underscore_word), underscore_word);
                    i -= 1;
                }

            }

            // If no, send current state of guesing word to client
            else {
                send_tlv(sockfd, 5, strlen(underscore_word), underscore_word);
            }
            continue;
        }

        // Exit
        else {

            // Stop measuring time
            syslog (LOG_INFO, "Player '%s' exited the game\n", nickname);
            fprintf(file, "%s failure\n", word);
            free(underscore_word);
            free(word);
            fclose(file);
            return;
        }

    }
    // Stop measuring time
    fprintf(file, "%s failure\n", word);
    free(underscore_word);
    free(word);
    fclose(file);
    return;
}



void game(int connfd, int multicastfd) {
	ssize_t		n;
	char		response[MAXLINE*10];
    int respn;
    char nickname[MAX_NICKNAME_LENGTH];
    

    // Read nickname
    TLV user_message;
    recv_tlv(connfd, &user_message);
    strncpy(nickname, user_message.value, MAX_NICKNAME_LENGTH - 1);
    nickname[MAX_NICKNAME_LENGTH - 1] = '\0';
    syslog(LOG_INFO,"Player '%s' has logged in\n", nickname);

    const char *directory = "./users";
    const char *filename = (char*)user_message.value;
    char filepath[256];

    snprintf(filepath, sizeof(filepath), "%s/%s", directory, filename);

    FILE *file = fopen(filepath, "w+");
    if (file == NULL) {
        syslog(LOG_ERR, "error while opening user's file\n");
        return;
    }
    free(user_message.value);

    // Main game loop (main menu)
    for ( ; ; ) {
    
        // User choice (what does he want to do)
        int choice;
        recv_tlv(connfd, &user_message);
        choice = *((char*)user_message.value) - 48;
        // Play game
        if (choice == 1) {
            play_game(connfd, multicastfd, nickname);
        
        } 
        // View ranking
        else if (choice == 2) {
            syslog(LOG_INFO,"Player %s checked the ranking\n", nickname);
            view_ranking(connfd);
        }

        // Exit
        else {
            syslog(LOG_INFO,"Player %s has left the game\n", nickname);
            free(user_message.value);
            exit(0);
        }
    }
}



