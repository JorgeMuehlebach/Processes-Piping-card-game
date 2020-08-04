//
// Created by Jorge Muehlebach on 2019-09-11.
//
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "tester.h"


/**
 * checks if the card is in the correct format and is in the possible value
 * range this card checker method is used mainly for read_deck
 * @param testCard the string that contains the card
 * @return 1 if the card is correct else 0
 */
int card_checker(char *testCard) {
    if (strlen(testCard) != 2) {
        return 0;
    } else if (!((testCard[1] >= 'a' && testCard[1] <= 'f')
            || (testCard[1] >= '1' && testCard[1] <= '9'))) {
        return 0;
    } else if (testCard[0] != 'S' && testCard[0] != 'C' && testCard[0]
            != 'D' && testCard[0] != 'H') {
        return 0;
    } else {
        return 1;
    }
}

/**
 * reads from an input stream or file until there is a newline
 * character or EOF
 * @param input
 * @return a string a characters in the input stream
 */
char *read_line(FILE *input) {
    // remember to free this
    char *result = malloc(sizeof(char) * 80);
    int position = 0;
    int next = 0;
    while (1) {
        next = fgetc(input);
        if (next == EOF || next == '\n') {
            result[position] = '\0';
            return result;
        } else {
            result[position++] = (char) next;
        }
    }
}

/**
 * checks the command line arguments given to the player
 * exits the game if any of the arguments given incorrectly
 * @param argc the number of arguments given to the player
 * @param argv a list of strings each string is an argument
 */
void player_input_check(int argc, char **argv) {
    if (argc != 5) {
        p_exit_game(NUM_ARG);
    }

    int numPlayers = atoi(argv[1]);
    int playersNumber = atoi(argv[2]);
    int threshold = atoi(argv[3]);
    int handSize = atoi(argv[4]);

    if (isdigit(*argv[1]) == 0 || numPlayers < 2
            || strlen(argv[1]) > 1) {
        p_exit_game(PLAYER_NUM);
    } else if (isdigit(*argv[2]) == 0 || playersNumber >= numPlayers
            || playersNumber < 0 || strlen(argv[2]) > 1) {
        p_exit_game(POSITION);
    } else if (isdigit(*argv[3]) == 0 || threshold < 2
            || strlen(argv[3]) > 1) {
        p_exit_game(THRESHOLD);
    } else if (isdigit(*argv[4]) == 0 || handSize < 1
            || strlen(argv[4]) > 1) {
        p_exit_game(HAND_SIZE);
    }
}

/**
 * exits the game with an exit status and message to stderr based on the
 * enum Pfault input
 * @param status
 */
void p_exit_game(Pfault status) {
    switch (status) {
        case NORMAL:
            break;
        case NUM_ARG:
            fprintf(stderr, "Usage: player players myid threshold"
                    " handsize\n");
            break;
        case PLAYER_NUM:
            fprintf(stderr, "Invalid players\n");
            break;
        case POSITION:
            fprintf(stderr, "Invalid position\n");
            break;
        case THRESHOLD:
            fprintf(stderr, "Invalid threshold\n");
            break;
        case HAND_SIZE:
            fprintf(stderr, "Invalid hand size\n");
            break;
        case INV_HUB:
            fprintf(stderr, "Invalid message\n");
            break;
        case WRONG_EOF:
            fprintf(stderr, "EOF\n");
            break;
    }
    exit(status);
}

/**
 * updates the d card tally which is a list integers associated with the amount
 * of d cards that each player has won.
 * @param dCardTally
 * @param thisRound: a list of cards played this round
 * @param numPLayers: the number of players in the game
 * @return the highest number of d cards currently won
 */
int update_d_card_tally(int *dCardTally, Card *thisRound, int numPLayers) {
    char leadSuit = thisRound[0].suit;
    int dCardCount = 0;
    Card winningCard;
    winningCard.number = 0;
    for (int i = 0; i < numPLayers; i++) {
        if (thisRound[i].suit == leadSuit && thisRound[i].number
                > winningCard.number) {
            winningCard = thisRound[i];
        }
        if (thisRound[i].suit == 'D') {
            dCardCount++;
        }
    }
    dCardTally[winningCard.ownerOfCard] = dCardTally[winningCard.ownerOfCard]
            + dCardCount;
    return dCardTally[winningCard.ownerOfCard];
}

/**
 * receives the hand from the hub through stdin
 * @param gamer: the player who is reading the hand
 * @param handSize: the expected size of a hand
 * @return the hand
 */
Card *get_hand(Player gamer, int handSize) {
    //HANDn,SR,SR,SR
    Card *hand = malloc(sizeof(Card) * gamer.handSize + 10);
    char *handData = read_line(stdin);
    int handDataSize = strlen(handData);
    if (handDataSize != (5 + 3 * handSize)) {
        p_exit_game(INV_HUB);
    } else if (atoi(&handData[4]) != handSize) {
        p_exit_game(INV_HUB);
    }
    int x = 0;
    for (int i = 6; i < handDataSize; i = i + 3) {
        hand[x].suit = handData[i];
        hand[x].number = handData[i + 1];
        if (card_checker2(hand[x]) == 0) {
            p_exit_game(INV_HUB);
        }
        x++;
    }
    return hand;
}

/**
 * calculates the number of turns the player needs to wait before it is their
 * turn.
 * @param startingPlayer: the number of the player who is starting the round
 * @param gamer: the player
 * @param numPlayers the number of players in the game
 * @return
 */
int turns_to_wait(int startingPlayer, Player gamer, int numPlayers) {
    if (startingPlayer == gamer.playerNumber) {
        return 0;
    } else if (startingPlayer > gamer.playerNumber) {
        return numPlayers - startingPlayer + gamer.playerNumber;
    } else {
        return gamer.playerNumber - startingPlayer;
    }
}

/**
 * chooses and plays a card depending on the player type.
 * calls play_card_alice if the card is to be played by alice and
 * play_card_bob if the card is to be played by bob.
 * @param thisRound: the list of cards played so far in this round
 * @param turnCounter: keeps track of how many turns have been had this round
 * @param gamer: the player
 * @param leadSuit: the suit that the first player of the round chose to
 * lead with
 * @param maxDCardCount: the highest amount of d cards won by a player so
 * far this game.
 */
void play_card(Card *thisRound, int turnCounter, Player gamer, char leadSuit,
        int maxDCardCount) {
    if (gamer.type == 'a') {
        play_card_alice(thisRound, turnCounter, gamer, leadSuit,
                maxDCardCount);
    } else if(gamer.type == 'b') {
        play_card_bob(thisRound, turnCounter, gamer, leadSuit, maxDCardCount);
    }
}

/**
 * alice chooses (with the help of get_choice) a card to play and plays
 * that card.
 * the chooseOrder1-2 are card choosing preference orders that the player
 * must follow
 * @param thisRound: the list of cards played so far in this round
 * @param turnCounter: keeps track of how many turns have been had this round
 * @param gamer: the player
 * @param leadSuit: the suit that the first player of the round chose to
 * lead with
 * @param maxDCardCount: the highest amount of d cards won by a player so
 * far this game.
 */
void play_card_alice(Card *thisRound, int turnCounter, Player gamer,
        char leadSuit, int maxDCardCount) {
    char chooseOrder1[5] = {'S', 'S', 'C', 'D', 'H'};
    char chooseOrder2[5] = {'D', 'D', 'H', 'S', 'C'};
    chooseOrder1[0] = leadSuit;
    chooseOrder2[0] = leadSuit;
    int choiceIndex = 0;
    if (turnCounter == 0) {
        choiceIndex = get_choice(chooseOrder1, 1, gamer,
                turnCounter);
        assign_this_round(thisRound, gamer, turnCounter, choiceIndex);
        fprintf(stdout, "PLAY%c%c\n", gamer.hand[choiceIndex].suit,
                gamer.hand[choiceIndex].number);
        fflush(stdout);
    } else { //when alice is not the lead
        choiceIndex = get_choice(chooseOrder2, 0, gamer,
                turnCounter);
        if (gamer.hand[choiceIndex].suit != leadSuit) {
            choiceIndex = get_choice(chooseOrder2, 1, gamer,
                    turnCounter);
        }
        assign_this_round(thisRound, gamer, turnCounter, choiceIndex);
        fprintf(stdout, "PLAY%c%c\n", gamer.hand[choiceIndex].suit,
                gamer.hand[choiceIndex].number);
        fflush(stdout);
    }
    remove_card(&gamer, choiceIndex);
}

/**
 * bob chooses (with the help of get_choice) a card to play and plays
 * that card. the chooseOrder1-3 are card choosing preference orders
 * that the player must follow.
 * @param thisRound: the list of cards played so far in this round
 * @param turnCounter: keeps track of how many turns have been had this round
 * @param gamer: the player
 * @param leadSuit: the suit that the first player of the round chose to
 * lead with
 * @param maxDCardCount: the highest amount of d cards won by a player so
 * far this game.
 */
void play_card_bob(Card *thisRound, int turnCounter, Player gamer,
        char leadSuit, int maxDCardCount) {
    char chooseOrder1[5] = {'S', 'S', 'C', 'D', 'H'};
    char chooseOrder2[5] = {'D', 'D', 'H', 'S', 'C'};
    char chooseOrder3[5] = {'S', 'S', 'C', 'H', 'D'};
    chooseOrder1[0] = leadSuit;
    chooseOrder2[0] = leadSuit;
    chooseOrder3[0] = leadSuit;
    int choiceIndex = 0;
    if (turnCounter == 0) {
        choiceIndex = get_choice(chooseOrder2, 0, gamer,
                turnCounter);
        assign_this_round(thisRound, gamer, turnCounter, choiceIndex);
        fprintf(stdout, "PLAY%c%c\n", gamer.hand[choiceIndex].suit,
                gamer.hand[choiceIndex].number);
        fflush(stdout);
    } else if (gamer.threshold - 2 <= maxDCardCount &&
            d_cards_this_round(thisRound, turnCounter) > 0) {
        choiceIndex = get_choice(chooseOrder2, 1, gamer,
                turnCounter);
        if (gamer.hand[choiceIndex].suit != leadSuit) {
            choiceIndex = get_choice(chooseOrder3, 0, gamer,
                    turnCounter);
        }
        assign_this_round(thisRound, gamer, turnCounter, choiceIndex);
        fprintf(stdout, "PLAY%c%c\n", gamer.hand[choiceIndex].suit,
                gamer.hand[choiceIndex].number);
        fflush(stdout);
    } else {
        choiceIndex = get_choice(chooseOrder2, 0, gamer,
                turnCounter);
        if (gamer.hand[choiceIndex].suit != leadSuit) {
            choiceIndex = get_choice(chooseOrder1, 1, gamer,
                    turnCounter);
        }
        assign_this_round(thisRound, gamer, turnCounter, choiceIndex);
        fprintf(stdout, "PLAY%c%c\n", gamer.hand[choiceIndex].suit,
                gamer.hand[choiceIndex].number);
        fflush(stdout);
    }
    remove_card(&gamer, choiceIndex);
}

/**
 * counts the number of d cards played this round
 * @param thisRound: the list of all the cards played this round
 * @param turnCounter: keeps track of how many turns have been had this round
 * @return the amount of d cards that have been played this round
 */
int d_cards_this_round(Card *thisRound, int turnCounter) {
    int dCardCount = 0;
    for (int i = 0; i < turnCounter; i++) {
        if (thisRound[i].suit == 'D') {
            dCardCount++;
        }
    }
    return dCardCount;
}

/**
 * chooses a card from the players hand based on the choose order or if
 * they are looking
 * for the highest or the lowest card.
 * @param chooseOrder: the priority order that the cards will be chosen in.
 * @param highest: if highest is 1 then we are looking for the highest number
 * card in the choose order
 * @param name: the player
 * @param turnCounter: keeps track of how many turns have been had this round.
 * @return the index of the card in the players hand that should be played
 */
int get_choice(char chooseOrder[5], int highest, Player name,
        int turnCounter) {
    int leading = 0;
    if (turnCounter == 0) {
        leading = 1;
    }
    int i = 0;
    int leastIndex = 0;
    int greatestIndex = 0;
    for (int x = leading; x < 5; x++) {
        char cardsAdded = 0;
        int greatest = 0;
        char least = 0;
        for (i = 0; i < name.handSize; i++) {
            if (chooseOrder[x] == name.hand[i].suit) {
                if (cardsAdded == 0) {
                    greatest = name.hand[i].number;
                    greatestIndex = i;
                    least = name.hand[i].number;
                    leastIndex = i;
                    cardsAdded++;
                } else if (name.hand[i].number > greatest) {
                    greatest = name.hand[i].number;
                    greatestIndex = i;
                } else if (name.hand[i].number < least) {
                    least = name.hand[i].number;
                    leastIndex = i;
                }
            }
        }
        if (cardsAdded > 0) {
            break;
        }
    }
    if (highest == 1) {
        return greatestIndex;
    } else {
        return leastIndex;
    }
}

/**
 * the main game loop for the player, sets up and plays the whole game as
 * the player
 * @param playerType: the type of the player 'b' for bob and 'a' for alice
 * @param argc: the number of command line arguments given
 * @param argv: the command line arguments given
 */
void player_play(char playerType, int argc, char **argv) {
    Player gamer;
    gamer.type = playerType;
    player_input_check(argc, argv);
    print_at_symbol();
    int numPlayers = atoi(argv[1]);
    gamer.playerNumber = atoi(argv[2]);
    gamer.threshold = atoi(argv[3]);
    gamer.handSize = atoi(argv[4]);
    int *dCardTally = malloc(sizeof(Card) * (numPlayers + 10));
    initialise_d_card_tally(dCardTally, numPlayers);
    int maxDCardCount = 0;
    gamer.hand = get_hand(gamer, gamer.handSize);
    for (int i = 0; i < gamer.handSize; i++) {
        Card *thisRound = malloc(sizeof(Card) * numPlayers + 10);
        int playedCard = 0;
        char leadSuit = 'z';
        int turnCounter = 0;
        char *newRound = read_lines(stdin);
        check_newround(newRound, numPlayers);
        int startingPlayer = atoi(&newRound[8]);
        int turnsToWait = turns_to_wait(startingPlayer, gamer, numPlayers);
        if (startingPlayer == gamer.playerNumber) {
            play_card(thisRound, turnCounter, gamer, leadSuit, maxDCardCount);
            turnCounter++;
            playedCard = 1;
        }
        while (turnCounter != numPlayers) {
            if (turnsToWait == 0 && playedCard != 1) {
                play_card(thisRound, turnCounter, gamer, leadSuit,
                        maxDCardCount);
                playedCard = 1;
                turnCounter++;
                continue;
            }
            char *otherPLays = read_lines(stdin);
            check_other_plays(otherPLays, numPlayers);
            if (turnCounter == 0) {
                leadSuit = otherPLays[8];
            }
            thisRound[turnCounter].suit = otherPLays[8];
            thisRound[turnCounter].number = otherPLays[9];
            thisRound[turnCounter].ownerOfCard = otherPLays[6];
            turnsToWait--;
            turnCounter++;
        }
        print_this_rounds(thisRound, numPlayers, startingPlayer);
        maxDCardCount = update_d_card_tally(dCardTally, thisRound, numPlayers);
        free(thisRound);
    }
}

/**
 * removes a card from the players hand
 * @param gamer: the player
 * @param choiceIndex: the index of the card that is to be removed
 */
void remove_card(Player *gamer, int choiceIndex) {
    for (int i = choiceIndex; i < gamer->handSize; i++) {
        gamer->hand[i] = gamer->hand[i + 1];
    }
    gamer->handSize = gamer->handSize - 1;
}

/**
 * prints the cards played this round as well as the lead player to stderr
 * @param thisRound: the cards played this round
 * @param numPlayers: the number of players in the game
 * @param startingPlayer: the number of the starting player
 */
void print_this_rounds(Card *thisRound, int numPlayers, int startingPlayer) {
    //Lead player=1: C.5 C.1 C.e H.3
    char pHand[50];
    sprintf(pHand, "Lead player=%d:", startingPlayer);
    int traverse = 14;
    for (int i = 0; i < numPlayers; i++) {
        sprintf(pHand + traverse, " %c.%c", thisRound[i].suit,
                thisRound[i].number);
        traverse = traverse + 4;
    }
    fprintf(stderr, "%s\n", pHand);
}

/**
 * sets every value in the d card tally to zero
 * @param dCardTally keeps track of how many d cards have been won by each
 * player
 * @param numPlayers: the number of players in the game
 */
void initialise_d_card_tally(int *dCardTally, int numPlayers) {
    for (int i = 0; i < numPlayers; i++) {
        dCardTally[i] = 0;
    }
}

/**
 * reads from an input stream or file until there is a newline
 * character or EOF.
 * if there is a an EOF then the process will exit with an error.
 * If "GAMEOVER" is received then the the process exits normally
 * @param input
 * @return a string a characters in the input stream
 */
char *read_lines(FILE *input) {

    char *result = malloc(sizeof(char) * 80);
    int position = 0;
    int next = 0;
    while (1) {
        next = fgetc(input);
        if (next == EOF) {
            p_exit_game(WRONG_EOF);
        }
        if (next == '\n') {
            result[position] = '\0';
            if (strcmp(result, "GAMEOVER") == 0) {
                p_exit_game(NORMAL);
            }
            return result;
        } else {
            result[position++] = (char) next;
        }
    }
}

/**
 * checks to see if the newround message sent to the player process is in the
 * correct format.
 * @param newRound: the string sent to the player for newround
 * @param numPlayers: the number of players
 */
void check_newround(char *newRound, int numPlayers) {
    int startingPlayer = atoi(&newRound[8]);
    if (startingPlayer >= numPlayers || isdigit(newRound[8]) == 0) {
        p_exit_game(INV_HUB);
    }
}

/**
 * checks to see if the play card moves of other players received
 * by the player are in the right
 * format.
 * @param otherPLays: a play card move made by another player
 * @param numPlayers: the number of players in the game
 */
void check_other_plays(char *otherPLays, int numPlayers) {
    //PLAYED0,B5
    char suit = otherPLays[8];
    char number = otherPLays[9];
    int playerNum = atoi(&otherPLays[6]);
    if (playerNum >= numPlayers || isdigit(otherPLays[6]) == 0) {
        p_exit_game(INV_HUB);
    }
    char chooseOrder1[4] = {'S', 'C', 'H', 'D'};
    if (suit != chooseOrder1[0] && suit != chooseOrder1[1] && suit
            != chooseOrder1[2] && suit != chooseOrder1[3]) {
        p_exit_game(INV_HUB);
    }
    if (!((number >= 'a' && number <= 'f') ||
            (number >= '1' && number <= '9'))) {
        p_exit_game(INV_HUB);
    }
}

/**
 * checks if the card is a correct cards in the correct range
 * @param testCard: the card to be checked
 * @return 1 if the card is correct else 0
 */
int card_checker2(Card testCard) {
    if (!((testCard.number >= 'a' && testCard.number <= 'f')
            || (testCard.number >= '1' && testCard.number <= '9'))) {
        return 0;
    } else if (testCard.suit != 'S' && testCard.suit != 'C' && testCard.suit
            != 'D' && testCard.suit != 'H') {
        return 0;
    } else {
        return 1;
    }
}

/**
 * places a card into the thisRound list
 * @param thisRound: a list of cards placed this round
 * @param gamer: the player
 * @param turnCounter: keeps track of the number of turns had this round
 * @param choiceIndex: the index of the card in the players
 * hand chosen to be played.
 */
void assign_this_round(Card *thisRound, Player gamer, int turnCounter,
        int choiceIndex) {
    thisRound[turnCounter].ownerOfCard = gamer.playerNumber;
    thisRound[turnCounter].suit = gamer.hand[choiceIndex].suit;
    thisRound[turnCounter].number = gamer.hand[choiceIndex].number;
}

/**
 * prints an @ symbol to the hub to indicate it has started porperly
 */
void print_at_symbol(void) {
    printf("@");
    fflush(stdout);
}


