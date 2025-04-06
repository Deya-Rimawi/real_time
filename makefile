CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lrt -lm

SRC_SIM = main.c config.c
SRC_VIEWER = viewer.c
HEADERS = config.h shared_data.h

BIN_SIM = rope_game
BIN_VIEWER = viewer

all: $(BIN_SIM) $(BIN_VIEWER)

$(BIN_SIM): $(SRC_SIM) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRC_SIM) $(LDFLAGS)

$(BIN_VIEWER): $(SRC_VIEWER) shared_data.h
	$(CC) $(CFLAGS) -o $@ $(SRC_VIEWER) -lGL -lGLU -lglut -lm

clean:
	rm -f $(BIN_SIM) $(BIN_VIEWER)

