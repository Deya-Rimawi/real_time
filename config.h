#ifndef CONFIG_H
#define CONFIG_H

// Configuration structure
typedef struct {
    int initial_energy_min;
    int initial_energy_max;
    int energy_decrease_min;
    int energy_decrease_max;
    int fall_time_min;
    int fall_time_max;
    int effort_threshold;
    int game_duration;
    int max_score;
    int rounds_to_win;
} Config;

int load_config(const char *filename, Config *config);

#endif
