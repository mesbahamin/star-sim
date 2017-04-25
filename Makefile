CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Wshadow
LDFLAGS = $(SDL_LDFLAGS) -lm

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

override CFLAGS += $(SDL_CFLAGS)

SRC = main.c
EXE = star-garden

DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGCFLAGS = -g -Og -Werror

RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELCFLAGS = -O2 -Os

.PHONY: all clean debug prep release run test

all: debug

clean:
	rm -f $(RELEXE) $(DBGEXE)

debug: prep
	$(CC) $(CFLAGS) $(DBGCFLAGS) $(SRC) -o $(DBGEXE) $(LDFLAGS)

prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

release: prep
	$(CC) $(CFLAGS) $(RELCFLAGS) $(SRC) -o $(RELEXE) $(LDFLAGS)

run:
	./$(DBGEXE)

test: debug run
