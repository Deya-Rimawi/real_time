#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "shared_data.h"

#define NUM_PLAYERS 8

typedef struct {
    pid_t pid;
    int team;
    int id;
    int pipe_fd[2];
    int energy;
    int effort;
    int is_falling;
    int fall_time_remaining;
} Player;

volatile sig_atomic_t start_pulling = 0;

void handle_get_ready(int sig) {
    printf("[PID %d] Received GET READY (SIGUSR1)\n", getpid());
}

void handle_start_pulling(int sig) {
    printf("[PID %d] Received START PULLING (SIGUSR2)\n", getpid());
    start_pulling = 1;
}

void handle_exit(int sig) {
    printf("[PID %d] Exiting gracefully on SIGTERM\n", getpid());
    exit(0);
}

int compare_players(const void *a, const void *b) {
    Player *player_a = (Player *)a;
    Player *player_b = (Player *)b;
    return player_b->effort - player_a->effort;
}

int main() {
    Config config;
    if (load_config("config.txt", &config) != 0) {
        fprintf(stderr, "Failed to load config.\n");
        return 1;
    }

    int shmid = shmget(SHM_KEY, sizeof(SharedScores), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }
    SharedScores *shared_scores = (SharedScores *)shmat(shmid, NULL, 0);

    Player players[NUM_PLAYERS];
    srand(time(NULL));

    for (int i = 0; i < NUM_PLAYERS; i++) {
        if (pipe(players[i].pipe_fd) < 0) {
            perror("Pipe creation failed");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) {
            int team = (i < 4) ? 1 : 2;
            int id = i % 4;
            srand(time(NULL) ^ getpid() ^ (id << 8));

            int energy = config.initial_energy_min + rand() % (config.initial_energy_max - config.initial_energy_min + 1);
            int effort = 0;
            int is_falling = 0;
            int fall_time_remaining = 0;

            signal(SIGUSR1, handle_get_ready);
            signal(SIGUSR2, handle_start_pulling);
            signal(SIGTERM, handle_exit);
            printf("[PID %d] Player %d from Team %d started with energy %d\n", getpid(), id, team, energy);
            close(players[i].pipe_fd[0]);

            while (!start_pulling) {
                pause();
            }

            while (1) {
                if (is_falling) {
                    effort = 0;
                    fall_time_remaining--;
                    if (fall_time_remaining <= 0) {
                        is_falling = 0;
                        printf("[PID %d] Player %d recovered from fall.\n", getpid(), id);
                    }
                } else {
                    int energy_decrease = config.energy_decrease_min + rand() % (config.energy_decrease_max - config.energy_decrease_min + 1);
                    energy -= energy_decrease;
                    if (energy < 0) energy = 0;

                    effort = energy * (id + 1);

                    if (energy <= 50)
                    {
                        int fall_chance = 5 + (100 - energy) / 4;
                        if ((rand() % 100) < fall_chance)
                        {
                            is_falling = 1;
                            fall_time_remaining = config.fall_time_min + rand() % (config.fall_time_max - config.fall_time_min + 1);
                            printf("[PID %d] Player %d from Team %d fell! Recovery: %d sec (Energy: %d, Chance: %d%%)\n",
                                   getpid(), id, team, fall_time_remaining, energy, fall_chance);
                        }
                    }}


                PlayerUpdate update = {effort, energy, is_falling};
                write(players[i].pipe_fd[1], &update, sizeof(PlayerUpdate));
                sleep(1);
            }
        } else {
            players[i].pid = pid;
            players[i].team = (i < 4) ? 1 : 2;
            players[i].id = i % 4;
            close(players[i].pipe_fd[1]);
        }
    }

    printf("\n[Referee] All players initialized.\n");
    sleep(1);
    qsort(players, NUM_PLAYERS, sizeof(Player), compare_players);

    int team1_rounds = 0, team2_rounds = 0;
    int round = 1;

    shared_scores->team1_rounds = 0;
    shared_scores->team2_rounds = 0;
    shared_scores->round_winner = 0;

    while (team1_rounds < config.rounds_to_win && team2_rounds < config.rounds_to_win) {
        printf("\n[Referee] Round %d Starting...\n", round);
        shared_scores->round = round;
        shared_scores->round_winner = 0;

        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i].pid, SIGUSR1);
        sleep(2);
        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i].pid, SIGUSR2);

        int team1_score = 0, team2_score = 0;

        for (int sec = 0; sec < config.game_duration; sec++) {
            PlayerUpdate update;
            for (int i = 0; i < NUM_PLAYERS; i++) {
                read(players[i].pipe_fd[0], &update, sizeof(PlayerUpdate));

                players[i].effort = update.effort;
                players[i].energy = update.energy;
                players[i].is_falling = update.is_falling;

                if (players[i].team == 1) team1_score += update.effort;
                else team2_score += update.effort;

                shared_scores->players[i].effort = update.effort;
                shared_scores->players[i].energy = update.energy;
                shared_scores->players[i].is_falling = update.is_falling;
                shared_scores->players[i].team = players[i].team; // ✅ added
                shared_scores->players[i].id = players[i].id;     // ✅ added
            }

            shared_scores->team1_score = team1_score;
            shared_scores->team2_score = team2_score;

            printf("[Referee][Sec %d] Team 1: %d | Team 2: %d\n", sec + 1, team1_score, team2_score);

            if (team1_score >= config.max_score || team2_score >= config.max_score) break;
        }

        printf("\n[Referee] Final Round Scores - Team 1: %d, Team 2: %d\n", team1_score, team2_score);
        if (team1_score > team2_score) {
            printf("[Referee] Team 1 wins Round %d!\n", round);
            team1_rounds++;
            shared_scores->team1_rounds = team1_rounds;
            shared_scores->round_winner = 1;
        } else if (team2_score > team1_score) {
            printf("[Referee] Team 2 wins Round %d!\n", round);
            team2_rounds++;
            shared_scores->team2_rounds = team2_rounds;
            shared_scores->round_winner = 2;
        } else {
            printf("[Referee] Round %d is a draw!\n", round);
            shared_scores->round_winner = 0;
        }

        round++;
        sleep(1);
    }

    printf("\n[Referee] Game Over. Sending SIGTERM to all players...\n");
    for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i].pid, SIGTERM);
    for (int i = 0; i < NUM_PLAYERS; i++) wait(NULL);
    printf("[Referee] All players have exited.\n");

    if (team1_rounds > team2_rounds)
        printf("[Referee] Team 1 is the Champion!\n");
    else
        printf("[Referee] Team 2 is the Champion!\n");

    shmdt(shared_scores);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
