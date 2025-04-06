#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <math.h>
#include "shared_data.h"

SharedScores *shared_scores = NULL;
int round_winner_display_timer = 0;

void connect_shared_memory() {
    int shmid;
    for (int attempts = 0; attempts < 10; attempts++) {
        shmid = shmget(SHM_KEY, sizeof(SharedScores), 0666);
        if (shmid >= 0) {
            shared_scores = (SharedScores *)shmat(shmid, NULL, 0);
            if (shared_scores != (void *)-1) return;
        }
        printf("[Viewer] Waiting for shared memory...\n");
        sleep(1);
    }
    fprintf(stderr, "Failed to connect to shared memory.\n");
    exit(1);
}

void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text++);
}

void drawCircle(float cx, float cy, float r) {
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 30; i++) {
        float angle = 2.0f * M_PI * i / 30;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

void drawSquare(float x, float y, float size) {
    glBegin(GL_QUADS);
    glVertex2f(x - size, y - size);
    glVertex2f(x + size, y - size);
    glVertex2f(x + size, y + size);
    glVertex2f(x - size, y + size);
    glEnd();
}

void drawScoreRope() {
    float t1 = shared_scores->team1_score;
    float t2 = shared_scores->team2_score;
    float total = t1 + t2;
    if (total == 0) return;

    float left = -0.8f;
    float right = 0.8f;
    float mid = 0.0f;

    float percent1 = t1 / total;
    float percent2 = t2 / total;

    float rope_half = 0.8f;
    float rope1_end = mid - (1.0f - percent1) * rope_half;
    float rope2_start = mid + (1.0f - percent2) * rope_half;

    // Team 1 segment
    glColor3f(0.4f, 0.6f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(left, 0.0f);
    glVertex2f(rope1_end, 0.0f);
    glVertex2f(rope1_end, 0.05f);
    glVertex2f(left, 0.05f);
    glEnd();

    // Team 2 segment
    glColor3f(1.0f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(rope2_start, 0.0f);
    glVertex2f(right, 0.0f);
    glVertex2f(right, 0.05f);
    glVertex2f(rope2_start, 0.05f);
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Round: %d | Team 1: %d (W: %d) | Team 2: %d (W: %d)",
             shared_scores->round,
             shared_scores->team1_score,
             shared_scores->team1_rounds,
             shared_scores->team2_score,
             shared_scores->team2_rounds);
    glColor3f(1, 1, 1);
    drawText(-0.5f, 0.9f, buffer);

    if (shared_scores->round_winner != 0 && round_winner_display_timer > 0) {
        const char *winner_msg =
            (shared_scores->round_winner == 1) ? "Team 1 won the round!" :
            (shared_scores->round_winner == 2) ? "Team 2 won the round!" : "It's a draw!";
        glColor3f(1.0f, 1.0f, 0.0f);
        drawText(-0.15f, 0.8f, winner_msg);
        round_winner_display_timer--;
    }

    drawScoreRope();

    int team1_count = 0;
    int team2_count = 0;

    for (int i = 0; i < NUM_PLAYERS; i++) {
        float y;
        if (shared_scores->players[i].team == 1 && team1_count < 4) {
            y = 0.7f - team1_count * 0.3f;
            glColor3f(0.4f, 0.6f, 1.0f);
            drawCircle(-0.7f, y, 0.04f);
            snprintf(buffer, sizeof(buffer), "P%d | Eff: %3d | %s",
                     shared_scores->players[i].id,
                     shared_scores->players[i].effort,
                     shared_scores->players[i].is_falling ? "FALLING" : "ACTIVE");
            drawText(-0.6f, y - 0.015f, buffer);
            team1_count++;
        }

        if (shared_scores->players[i].team == 2 && team2_count < 4) {
            y = 0.7f - team2_count * 0.3f;
            glColor3f(1.0f, 0.5f, 0.5f);
            drawSquare(0.7f, y, 0.04f);
            snprintf(buffer, sizeof(buffer), "P%d | Eff: %3d | %s",
                     shared_scores->players[i].id,
                     shared_scores->players[i].effort,
                     shared_scores->players[i].is_falling ? "FALLING" : "ACTIVE");
            drawText(0.1f, y - 0.015f, buffer);
            team2_count++;
        }
    }

    glutSwapBuffers();
}

void timer(int value) {
    if (shared_scores->round_winner != 0)
        round_winner_display_timer = 3;
    glutPostRedisplay();
    glutTimerFunc(1000, timer, 0);
}

int main(int argc, char** argv) {
    connect_shared_memory();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(900, 700);
    glutCreateWindow("Tug-of-War Viewer");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();

    shmdt(shared_scores);
    return 0;
}
