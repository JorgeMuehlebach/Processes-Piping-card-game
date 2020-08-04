
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


#ifndef UNTITLED_TESTER_H
#define UNTITLED_TESTER_H

/**
 * this enum represents all the conditions that would cause the game to exit
 * this enum is used in p_exit_game(Error);
 */
typedef enum {
    NORMAL,
    NUM_ARG,
    PLAYER_NUM,
    POSITION,
    THRESHOLD,
    HAND_SIZE,
    INV_HUB,
    WRONG_EOF
} Pfault;

/**
 * this is a card struct that represents a card in the game and has a
 * number, suit, and an owner
 */
typedef struct {
    char number;
    char suit;
    int ownerOfCard;
} Card;

/**
 * this is a player struct that holds many attributes of a player in the game
 */
typedef struct {
    Card* hand;
    int playerNumber;
    int threshold;
    int handSize;
    char type;
    int score;
    int dCardsWon;
    FILE* receive;
    FILE* send;
} Player;

void remove_card(Player* gamer, int choiceIndex);
char* read_line(FILE* deckFile);
char* read_lines(FILE* deckFile);
void p_exit_game(Pfault status);
int card_checker(char* testCard);
void player_input_check(int argc, char** argv);
int update_d_card_tally(int* dCardTally, Card* thisRound, int numPLayers);
int get_choice(char chooseOrder[4], int highest, Player name, int turnCounter);
void play_card(Card* thisRound, int turnCounter, Player gamer, char leadSuit,
        int maxDCardCount);
int turns_to_wait(int startingPlayer, Player alice, int numPlayers);
Card* get_hand(Player alice, int handSize);
void check_other_plays(char* otherPLays, int numPlayers);
int d_cards_this_round(Card* thisRound, int turnCounter);
void check_newround(char* newRound, int numPlayers);
void player_play(char playerType, int argc, char** argv);
void print_this_rounds(Card* thisRound, int numPlayers, int startingPlayer);
void initialise_d_card_tally(int* dCardTally, int numPlayers);
int card_checker2(Card testCard);
void assign_this_round(Card* thisRound, Player gamer, int turnCounter,
        int choiceIndex);
void play_card_bob(Card* thisRound, int turnCounter, Player gamer,
        char leadSuit, int maxDCardCount);
void play_card_alice(Card* thisRound, int turnCounter, Player gamer,
        char leadSuit, int maxDCardCount);
void print_at_symbol(void);

#endif
