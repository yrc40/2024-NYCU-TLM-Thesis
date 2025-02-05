.PHONY: all

all: build run

build:
	g++ -std=c++23 -Iinclude -o bus -Wall main.cpp System.cpp Bus.cpp Event.cpp Plan.cpp

prod:
	g++ -O3 -std=c++23 -Iinclude -o bus -Wall main.cpp System.cpp Bus.cpp Event.cpp Plan.cpp

run: 
	./bus > result.txt
