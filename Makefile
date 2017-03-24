build:
	g++ -std=c++11 -Wall -Wextra -g main.cpp -o star-garden `sdl2-config --cflags --libs`

run:
	./star-garden

test: build run
