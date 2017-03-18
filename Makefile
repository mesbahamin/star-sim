build:
	g++ -std=c++11 -Wall -Wextra -g stars.cpp -o stars `sdl2-config --cflags --libs`

run:
	./stars

test: build run
