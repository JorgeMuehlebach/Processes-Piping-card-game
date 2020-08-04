#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tester.h"
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * this enum represents all the conditions that would casue the game to exit
 * this enum is used in exit_game(Error);
 */
typedef enum {
    NORMAL_EXIT,
    NUM_ARGS,
    THRESHOLD_ERR,
    BAD_DECK,
    LESS_CARDS,
    PLAYER_ERR,
    EOF_PLAYER,
    INV_MESSAGE,
    CHOSE_WRONG_CARD,
    SIGHUP_RECEIVED
} Error;

void send_game_over(Player* players, int numPLayers);
void print_this_rounds(Card* thisRound, int numPlayers, int startingPlayer);
void out_put_scores(Player* players, int numPLayers);
void print_this_round(Card* thisRound, int numPlayers);
void print_lead_player(int leadPlayer);
int set_scores(Player* players, Card* thisRound, int numPlayers);
void send_played(Player* players, int currentPlayer, int numPlayers,
        Card rCard);
Card receive(Player gamer);
void send_new_round(Player* players, int numPLayers, int leader);
void remove_card_hub(Player* players, int playerNUmber, char suit,
        char number);
Player* initialise_players(int threshHold, long deckSize, int numPlayers,
        Card* deck, char** argv);
void check_players(FILE*** pipeEnds, int numPLayers);
void exit_game(Error status);
FILE*** setup_pipes(int numPLayers, Player* players, char** argv);
void check_hub_input(int argc, char** argv);
Card* read_deck(char* deckName, long int* deckLength, int numPLayers);
void send_hands(Player* players, int numPLayers);
void check_card(Card rCard, Player gamer);
char* read_linex(FILE* input);
void free_players(Player* players, int numPlayers);
void kill_child_process(int signal);

/**
 * these two global variables are used to handle the SIGHUP signal
 */
int numberOfPLayers = 0;
int pids[60]; // max 4 suits * 15 ranks

int main(int argc, char** argv) {
    //handles sighup
    struct sigaction sa;
    sa.sa_handler = kill_child_process;
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, 0);
    check_hub_input(argc, argv);
    char* deckName = argv[1];
    int threshold = atoi(argv[2]);
    long deckSize = 0;
    int numPLayers = argc - 3;
    numberOfPLayers = numPLayers;
    Card* deck = read_deck(deckName, &deckSize, numPLayers);
    Player* players = initialise_players(threshold, deckSize, numPLayers,
            deck, argv);
    FILE*** pipeEnds = setup_pipes(numPLayers, players, argv);
    check_players(pipeEnds, numPLayers);
    send_hands(players, numPLayers);
    int initialHandSize = players[0].handSize;
    int leadPlayer = 0;
    int currentPlayer = 0;
    for(int i = 0; i < initialHandSize; i++) {
        send_new_round(players, numPLayers, leadPlayer);
        Card* thisRound = malloc(sizeof(Card) * numPLayers + 10);
        print_lead_player(leadPlayer);
        for(int x = 0; x < numPLayers; x++) {
            Card rCard = receive(players[currentPlayer]);
            check_card(rCard, players[currentPlayer]);
            thisRound[x].suit = rCard.suit;
            thisRound[x].number = rCard.number;
            thisRound[x].ownerOfCard = currentPlayer;
            remove_card_hub(players, currentPlayer, rCard.suit, rCard.number);
            send_played(players, currentPlayer, numPLayers, rCard);
            if(currentPlayer < numPLayers - 1) {
                currentPlayer++;
            } else {
                currentPlayer = currentPlayer - numPLayers + 1;
            }
        }
        print_this_round(thisRound, numPLayers);
        leadPlayer = set_scores(players, thisRound, numPLayers);
        currentPlayer = leadPlayer;
        free(thisRound);
    }
    out_put_scores(players, numPLayers);
    send_game_over(players, numPLayers);
    free(deck);
    free_players(players, numPLayers);
    exit_game(NORMAL_EXIT);
    return 0;
}

/**
 * checks a few of the  the command line arguments
 * @param argc: the number of arguments
 * @param argv: the arguments
 */
void check_hub_input(int argc, char** argv) {
    int threshold = atoi(argv[2]);
    if(argc < 4) {
        exit_game(NUM_ARGS);
    } else if(isdigit(*argv[2]) == 0 || threshold < 2) {
        exit_game(THRESHOLD_ERR);
    }
}

/**
 * exits the game with an exit status and message to stderr based on the
 * enum Error input
 * @param status
 */
void exit_game(Error status) {
    switch(status) {
        case NORMAL_EXIT:
            break;
        case NUM_ARGS:
            fprintf(stderr, "Usage: 2310hub deck threshold player0"
                    " {player1}\n");
            break;
        case THRESHOLD_ERR:
            fprintf(stderr, "Invalid threshold\n");
            break;
        case BAD_DECK:
            fprintf(stderr, "Deck error\n");
            break;
        case LESS_CARDS:
            fprintf(stderr, "Not enough cards\n");
            break;
        case PLAYER_ERR:
            fprintf(stderr, "player error\n");
            break;
        case EOF_PLAYER:
            fprintf(stderr, "player EOF\n");
            break;
        case INV_MESSAGE:
            fprintf(stderr, "Invalid message\n");
            break;
        case CHOSE_WRONG_CARD:
            fprintf(stderr, "Invalid card choice\n");
            break;
        case SIGHUP_RECEIVED:
            fprintf(stderr, "Ended due to signal\n");
            break;
        default:
            break;
    }
    exit(status);
}

/**
 * reads from the deck text file and checks the format of the deck
 * @param deckName: the name of the deck file
 * @param deckLength the length of the deck
 * @param numPLayers: the number of players in the game
 * @return a list of cards
 */
Card* read_deck(char* deckName, long int* deckLength, int numPLayers) {
    int numberOfCardsCheck = 1;
    FILE* deckFile;
    deckFile = fopen(deckName, "r");
    struct stat test;
    stat(deckName, &test);
    if (deckFile == NULL || S_ISDIR(test.st_mode)) {
        exit_game(BAD_DECK);
    }
    char* firstLine;
    firstLine = read_line(deckFile);
    for (int i = 0; i < strlen(firstLine); i++) {
        if (isdigit(firstLine[i]) == 0) {
            exit_game(BAD_DECK);
        }
    }
    *deckLength = strtol(firstLine, NULL, 10);
    long int deckSize = *deckLength;
    if (deckSize < numPLayers) {
        numberOfCardsCheck = 0;
    }
    Card* deck = malloc(sizeof(Card) * deckSize + 20);
    for (int i = 0; i < deckSize; i = i + 1) {
        char* fakeCard = read_line(deckFile);
        if (card_checker(fakeCard) == 0) {
            //printf("what the heck\n");
            exit_game(BAD_DECK);
        }
        deck[i].suit = fakeCard[0];
        deck[i].number = fakeCard[1];
    }
    char* emptyLine = read_line(deckFile);
    if (emptyLine[0] != '\0') {
        exit_game(BAD_DECK);
    }
    if(numberOfCardsCheck == 0) {
        exit_game(LESS_CARDS);
    }
    return deck;
}

/**
 * initialises and sets up all the child processes and saves file pointer
 * connections to them
 * @param numPLayers: the number of players in the game
 * @param players: a list of the players in the game
 * @param argv: the command line arguments
 * @return a multi dimentional array of file pointers that connect to
 * the child processes/other players.
 */
FILE*** setup_pipes(int numPLayers, Player* players, char** argv) {
    FILE*** connections;
    connections = (FILE***)malloc(sizeof(FILE**) * numPLayers * 2);
    for (int i = 0; i < numPLayers * 2; ++i) {
        connections[i] = (FILE**)malloc(sizeof(FILE*) * 2);
    }
    for(int i = 0; i < numPLayers; i++) {
        char numPLayerz[10];
        char playersNumber[10];
        char threshold[10];
        char handSize[10];
        sprintf(numPLayerz, "%d", numPLayers);
        sprintf(playersNumber, "%d", players[i].playerNumber);
        sprintf(threshold, "%d", players[i].threshold);
        sprintf(handSize, "%d", players[i].handSize);
        char* playerArgs[] = {"2310alice", numPLayerz, playersNumber,
                threshold, handSize, NULL};
        char* playerArgs1[] = {"2310bob", numPLayerz, playersNumber, threshold,
                handSize, NULL};
        int x = 2 * i;
        int pipes1[2], pipes2[2], redirect[2];
        pipe(pipes1);
        pipe(pipes2);
        pipe(redirect);
        int fVal = fork();
        pids[i] = fVal;
        if (fVal == 0) {
            close(pipes2[1]);
            close(pipes1[0]);
            close(redirect[0]);
            dup2(redirect[1], STDERR_FILENO);
            dup2(pipes1[1], STDOUT_FILENO);
            dup2(pipes2[0], STDIN_FILENO);
            if(players[i].type == 'a') {
                execvp(argv[i + 3], playerArgs);
            } else {
                execvp(argv[i + 3], playerArgs1);
            }
            exit(1);
        } else {
            close(pipes2[0]);
            close(pipes1[1]);
            connections[x][0] = fdopen(pipes1[0], "r");
            players[i].receive = connections[x][0];
            connections[x + 1][1] = fdopen(pipes2[1], "w");
            players[i].send = connections[x + 1][1];
        }
    }
    return connections;
}

/**
 * checks to see if an @ symbol was sent from each player as they
 * are initiated. exits the game if the @ symbols are not sent
 * @param pipeEnds: the multi dimentional array of pipes that are connected
 * to the children
 * @param numPLayers: the number of players in the game
 */
void check_players(FILE*** pipeEnds, int numPLayers) {
    for(int x = 0; x < numPLayers; ++x) {
        int i = x * 2;
        if(fgetc(pipeEnds[i][0]) != '@') {
            exit_game(PLAYER_ERR);
        }
    }
}

/**
 * initialises the players with a hand, a threshold, a player
 * number a handsize, and a type.
 * @param threshHold: the amount of d cards the player must win
 * to achieve a good score
 * @param deckSize: the size of the deck
 * @param numPlayers: the number of players in the game
 * @param deck: the list of cards that is the deck
 * @param argv: the command line arguments given to hub
 * @return a list of players in the game
 */
Player* initialise_players(int threshHold, long deckSize, int numPlayers,
        Card* deck, char** argv) {
    int handSize = (int)deckSize / numPlayers;
    Player* players = malloc(sizeof(Player) * numPlayers);
    int topCardIndex = 0;
    for(int i = 0; i < numPlayers; i++) {
        //initialise hand
        Card* hand = malloc(sizeof(Card) * handSize);
        for(int x = 0; x < handSize; x++) {
            hand[x].suit = deck[topCardIndex].suit;
            hand[x].number = deck[topCardIndex].number;
            topCardIndex++;
        }
        players[i].hand = hand;
        players[i].threshold = threshHold;
        players[i].playerNumber = i;
        players[i].type = argv[i + 3][6];
        players[i].handSize = handSize;
    }
    return players;
}

/**
 * sends the hands to the players
 * @param players: the list of players in the game
 * @param numPLayers: the number of players in the game
 */
void send_hands(Player* players, int numPLayers) {

    int tempSize = players[0].handSize;
    int numDigits = 0;
    while(tempSize != 0) {
        tempSize = tempSize / 10;
        numDigits++;
    }
    for (int i = 0; i < numPLayers; i++) {
        char pHand[50];
        sprintf(pHand, "HAND%d", players[i].handSize);
        int traverse = 4 + numDigits;
        for(int x = 0; x < players[i].handSize; x++) {
            sprintf(pHand + traverse, ",%c%c",
                    players[i].hand[x].suit, players[i].hand[x].number);
            traverse = traverse + 3;
        }
        fprintf(players[i].send, "%s\n", pHand);
        fflush(players[i].send);
    }
}

/**
 * removes the card from the players hand that the player played
 * @param players: the players in the game
 * @param playerNumber: the number of the player that played the card
 * @param suit: the suit of the card
 * @param number: the number of the card
 */
void remove_card_hub(Player* players, int playerNUmber, char suit,
        char number) {
    int started = 0;
    Player gamer = players[playerNUmber];
    for(int i = 0; i < gamer.handSize; i++) {
        if((gamer.hand[i].suit == suit && gamer.hand[i].number == number)
                || started == 1) {
            gamer.hand[i] = gamer.hand[i + 1];
            started = 1;
        }
    }
    gamer.handSize = gamer.handSize - 1;
}

/**
 * sends the newround message to all the players
 * @param players: the players in the game
 * @param numPLayers: the number of players in the game
 * @param leader: player number of the leader
 */
void send_new_round(Player* players, int numPLayers, int leader) {
    char newRound[50];
    snprintf(newRound, 20, "NEWROUND%d", leader);
    for(int i = 0; i < numPLayers; i++) {
        fprintf(players[i].send, "%s\n", newRound);
        fflush(players[i].send);
    }
}

/**
 * receives a card from the player and returns it
 * @param gamer the player that the card is being received from
 * @return the card that the player received
 */
Card receive(Player gamer) {
    Card receivedCard;
    char* input;
    input = read_linex(gamer.receive);
    receivedCard.suit = input[4];
    receivedCard.number = input[5];
    free(input);
    return receivedCard;
}

/**
 * sends a card to each of the players excluding the player who the card was
 * received from.
 * @param players: the players in the game
 * @param currentPlayer: the player who played the card
 * @param numPlayers: the number of players in the game
 * @param rCard: the card that was played by the current player
 */
void send_played(Player* players, int currentPlayer, int numPlayers,
        Card rCard) {

    char played[50];
    snprintf(played, 20, "PLAYED%d,%c%c", currentPlayer,
            rCard.suit, rCard.number);
    for(int i = 0; i < numPlayers; i++) {
        if(i != currentPlayer) {
            fprintf(players[i].send, "%s\n", played);
            fflush(players[i].send);
        }
    }
}

/**
 * set the scores of each player
 * @param players: the players in the game
 * @param thisRound: the cards played this round
 * @param numPlayers: the number of players in the game
 * @return: the card that won this round
 */
int set_scores(Player* players, Card* thisRound, int numPlayers) {
    char leadSuite = thisRound[0].suit;
    int dCardCount = 0;
    Card winningCard = thisRound[0];
    for(int i = 0; i < numPlayers; i++) {
        if(thisRound[i].suit == 'D') {
            dCardCount++;
        }
        if(thisRound[i].suit == leadSuite && thisRound[i].number
                > winningCard.number) {
            winningCard = thisRound[i];
        }
    }
    players[winningCard.ownerOfCard].score =
            players[winningCard.ownerOfCard].score + 1;
    players[winningCard.ownerOfCard].dCardsWon =
            players[winningCard.ownerOfCard].dCardsWon + dCardCount;

    return winningCard.ownerOfCard;
}

/**
 * prints the leading player for the current round
 * @param leadPlayer: the leading player for this current round
 */
void print_lead_player(int leadPlayer) {
    printf("Lead player=%d\n", leadPlayer);
}

/**
 * prints the cards played this round
 * @param thisRound: the cards played this round
 * @param numPlayers: the number of players in the game
 */
void print_this_round(Card* thisRound, int numPlayers) {
    char pHand[50];
    sprintf(pHand, "Cards=");
    int traverse = 6;
    for(int i = 0; i < numPlayers; i++) {
        if(i == numPlayers - 1) {
            sprintf(pHand + traverse, "%c.%c", thisRound[i].suit,
                    thisRound[i].number);
        } else {
            sprintf(pHand + traverse, "%c.%c ", thisRound[i].suit,
                    thisRound[i].number);
        }
        traverse = traverse + 4;
    }
    printf("%s\n", pHand);
}

/**
 * prints out the scores each player received at the end of the game
 * @param players: the players in the game
 * @param numPLayers the number of players in the game
 */
void out_put_scores(Player* players, int numPLayers) {
    for(int i = 0; i < numPLayers; i++) {
        if(players[i].threshold <= players[i].dCardsWon) {
            printf("%d:%d", i, players[i].dCardsWon + players[i].score);
        } else {
            printf("%d:%d", i, players[i].score - players[i].dCardsWon);
        }
        if(i < numPLayers - 1) {
            printf(" ");
        }

    }
    printf("\n");
}

/**
 * sends game over to the players
 * @param players: the players in the game
 * @param numPLayers: the number of players in the game
 */
void send_game_over(Player* players, int numPLayers) {
    for (int i = 0; i < numPLayers; i++) {
        fprintf(players[i].send, "GAMEOVER\n");
    }
}

/**
 * checks to see if the card is a valid card and then check if the player
 * owns that card
 * @param rCard: the card that is being checked
 * @param gamer: the player who owns the card
 */
void check_card(Card rCard, Player gamer) {
    if(card_checker2(rCard) == 0) {
        exit_game(INV_MESSAGE);
    }
    int ownsCard = 0;
    for(int i = 0; i < gamer.handSize; i++) {
        if(gamer.hand[i].suit == rCard.suit && gamer.hand[i].number
                == rCard.number) {
            ownsCard = 1;
        }
    }
    if(ownsCard == 0) {
        exit_game(INV_MESSAGE);
    }
}

/**
 * reads from an input stream or file until there is a newline
 * character or EOF. if there is a an EOF then the process will
 * exit with an error.
 * @param input
 * @return a string a characters in the input stream
 */
char* read_linex(FILE* input) {
    char* result = malloc(sizeof(char) * 80);
    int position = 0;
    int next = 0;
    while (1) {
        next = fgetc(input);
        if(next == EOF) {
            exit_game(EOF_PLAYER);
        }
        if (next == EOF || next == '\n') {
            result[position] = '\0';
            return result;
        } else {
            result[position++] = (char)next;
        }
    }
}

/**
 * free's the memory of the players hands
 * @param players: the players in the game
 * @param numPlayers: the number of players in the game
 */
void free_players(Player* players, int numPlayers) {
    for(int i = 0; i < numPlayers; i++) {
        free(players[i].hand);
    }
    free(players);
}

/**
 * kills all the child processes when called
 * is only called when hub receives SIGHUP
 * @param signal
 */
void kill_child_process(int signal) {
    // pkill -u$USER -HUP 2310hub will kill the players with SIGHUP
    for(int i = 0; i < numberOfPLayers; i++) {
        kill(pids[i], SIGKILL);
    }
    while(wait(NULL) >= 0);
    exit_game(SIGHUP_RECEIVED);
}

