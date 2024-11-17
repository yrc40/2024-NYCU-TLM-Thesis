.PHONY: all

all:
	g++ -std=c++23 -o bus -Wall main.cpp System.cpp Bus.cpp Event.cpp

prod:
	g++ -O3 -std=c++23 -o bus -Wall main.cpp System.cpp Bus.cpp Event.cpp