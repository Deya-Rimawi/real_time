#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#define NUM_PLAYERS 8
#define SHM_KEY 1234

typedef struct {
    int effort;
    int energy;
    int is_falling;
} PlayerUpdate;

typedef struct {
    int team1_score;
    int team2_score;
    int round;
    int team1_rounds;
    int team2_rounds;
    int round_winner; // 0 = none/draw, 1 = Team 1, 2 = Team 2
    struct {
        int effort;
        int energy;
        int is_falling;
        int id;      // player ID within the team (0-3)
        int team;    // 1 or 2
    } players[NUM_PLAYERS];
} SharedScores;

#endif
