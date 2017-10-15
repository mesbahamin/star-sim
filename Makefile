CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter
LDFLAGS = $(SDL_LDFLAGS) -lm

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

override CFLAGS += $(SDL_CFLAGS)

SRC_FILES = platform_sdl.c sim.c star.c barnes_hut.c
SRC = $(addprefix src/, $(SRC_FILES))
EXE = star-garden

DBGDIR = debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGCFLAGS = -g -Og -Werror

RELDIR = release
RELEXE = $(RELDIR)/$(EXE)
RELCFLAGS = -O2 -Os

.PHONY: all clean compile_debug compile_release debug memcheck prep run todo

all: compile_debug compile_release

clean:
	rm -f $(RELDIR)/* $(DBGDIR)/*

compile_debug: prep
	$(CC) $(CFLAGS) $(DBGCFLAGS) $(SRC) -o $(DBGEXE) $(LDFLAGS)

compile_release: prep
	$(CC) $(CFLAGS) $(RELCFLAGS) $(SRC) -o $(RELEXE) $(LDFLAGS)

debug: compile_debug
	gdb $(DBGEXE)

memcheck: compile_debug
	valgrind --track-origins=yes ./$(DBGEXE)

prep:
	@mkdir -p $(DBGDIR) $(RELDIR)

run: compile_release
	./$(DBGEXE)

todo:
	@grep -FIR --colour=never --ignore-case --line-number todo src/ \
	| sed -re  's/^([^:]+):[[:space:]]*(.*)/\1\x01\2/' \
	| sed -re  's/^([^:]+):[[:space:]]*(.*)/\1\x01\2/' \
	| column -s $$'\x01' -t
