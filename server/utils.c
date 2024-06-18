
#include "utils.h"

// Struct representing the player in the ranking
typedef struct {
    char nickname[MAX_NICK_LENGTH];
    double seconds;
} Player;

int count_lines(const char *filename) {
    FILE *file;
    int lines = 0;
    char buffer[MAX_WORD_LENGTH];

    file = fopen(filename, "r");
    if (file == NULL) {
        syslog (LOG_ERR, "Error opening file at function count_lines\n");
        return -1;
    }

    while (fgets(buffer, MAX_WORD_LENGTH, file)) {
        lines++;
    }

    fclose(file);
    return lines;
}

char* draw_word(const char *filename) {
    FILE *file;
    int total_lines, random_line, current_line = 0;
    char buffer[MAX_WORD_LENGTH];

    total_lines = count_lines(filename);
    if (total_lines == -1 || total_lines == 0) {
        syslog (LOG_ERR, "No words found in file or error counting lines.\n");
        return NULL;
    }

    srand(time(NULL));
    random_line = rand() % total_lines;

    file = fopen(filename, "r");
    if (file == NULL) {
        syslog (LOG_ERR, "Error opening file 'words' at function draw_word\n");
        return NULL;
    }

    while (fgets(buffer, MAX_WORD_LENGTH, file)) {
        if (current_line == random_line) {
            buffer[strcspn(buffer, "\n")] = 0;
            fclose(file);
            return strdup(buffer);
        }
        current_line++;
    }

    fclose(file);
    return NULL;
}


char* create_underscore_word(char *word) {
    int length = strlen(word);
    char *underscore_word = (char *)malloc((length + 1) * sizeof(char));


    if (underscore_word == NULL) {
        syslog (LOG_ERR, "Error allocating memory at function create_underscore_word\n");
        exit(1);
    }

    for (int i = 0; i < length; i++) {
        underscore_word[i] = '_';
    }

    underscore_word[length] = '\0';
    return underscore_word;
}


// Function checking whether the given letter exists in the word and if it does, actualizing underscore_word
int check_letter(char letter, char* underscore_word, char* word) {
    int found = 0;

    for (int i = 0; i < strlen(word); i++) {
        if (word[i] == letter) {
            underscore_word[i] = letter;
            found = 1;
        }
    }

    return found;
}


int check_ranking(const char* nickname, double seconds) {
    FILE *file;
    Player players[MAX_PLAYERS+1];
    int count = 0;
    int i, j;

    file = fopen("ranking", "r");
    if (file != NULL) {
        while (fscanf(file, "%s %lf", players[count].nickname, &players[count].seconds) == 2 && count < MAX_PLAYERS) {
            count++;
        }
        fclose(file);
    }

    // Check if new result has beaten someone in the ranking
    int rank_updated = 0;
    for (i = 0; i < count; i++) {
        if (seconds < players[i].seconds) {
            rank_updated = 1;
            break;
        }
    }

    // Dodaj nowy wynik do rankingu, jeśli jest lepszy od istniejących
    // Add the new result to the ranking if it is better than someone in the ranking or the ranking is smaller than MAX_PLAYERS value
    if (rank_updated || count < MAX_PLAYERS) {
        // Drop worse ranking results one place down in order to make a place for the new player
        for (j = count; j > i; j--) {
            players[j] = players[j-1];
        }
        // Add new record
        strncpy(players[i].nickname, nickname, MAX_NICK_LENGTH - 1);
        players[i].nickname[MAX_NICK_LENGTH - 1] = '\0';
        players[i].seconds = seconds;

        if (count < MAX_PLAYERS) {
            count++;
        }
    }

    // Save actualized ranking in the 'ranking' file
    file = fopen("ranking", "w");
    if (file == NULL) {
        syslog (LOG_ERR, "Error while opening file 'ranking' at function check_ranking\n");
        return -1;
    }
    for (int k = 0; k < count; k++) {
        fprintf(file, "%s %lf\n", players[k].nickname, players[k].seconds);
    }
    fclose(file);

    if (rank_updated) {
        // 2 if there is new first place in ranking, 1 if the player has beaten someone
        if (i == 0) {
            return 2;
        } else {
            return 1;
        }
    } else {
        // 0 if the player didn't beat anyone
        return 0;
    }
}