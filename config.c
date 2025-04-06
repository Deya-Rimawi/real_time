// config.c
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

int load_config(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    fscanf(file, "initial_energy_min=%d\n", &config->initial_energy_min);
    fscanf(file, "initial_energy_max=%d\n", &config->initial_energy_max);
    fscanf(file, "energy_decrease_min=%d\n", &config->energy_decrease_min);
    fscanf(file, "energy_decrease_max=%d\n", &config->energy_decrease_max);
    fscanf(file, "fall_time_min=%d\n", &config->fall_time_min);
    fscanf(file, "fall_time_max=%d\n", &config->fall_time_max);
    fscanf(file, "rounds_to_win=%d\n", &config->rounds_to_win);
    fscanf(file, "game_duration=%d\n", &config->game_duration);
    fscanf(file, "max_score=%d\n", &config->max_score);

    fclose(file);
    return 0;
}
