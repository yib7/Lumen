# Makefile for Lumen. Mirrors build.sh for people who prefer make.
#   make          release build with OpenMP -> lumen(.exe)
#   make debug    debug build, -g -O0, no OpenMP
#   make clean    remove the binary

# GNU Make pre-defines CC as "cc". Override only that built-in default so a
# real user-supplied CC (environment or command line) still wins. WinLibs
# MinGW ships gcc, not cc, so the bare default would fail there.
ifeq ($(origin CC),default)
CC := gcc
endif
CFLAGS  := -std=c11 -Wall -Wextra -Isrc
RELEASE := -O2 -fopenmp
DEBUG   := -g -O0
LDLIBS  := -lm
SRC     := $(wildcard src/*.c)
OUT     := lumen

.PHONY: all debug clean

all:
	$(CC) $(CFLAGS) $(RELEASE) $(SRC) $(LDLIBS) -o $(OUT)

debug:
	$(CC) $(CFLAGS) $(DEBUG) $(SRC) $(LDLIBS) -o $(OUT)

clean:
	rm -f $(OUT) $(OUT).exe
