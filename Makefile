CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

all: knapsack tests

knapsack: knapsack_main.cpp knapsack.hpp
	$(CXX) $(CXXFLAGS) knapsack_main.cpp -o $@

tests: tests.cpp knapsack.hpp
	$(CXX) $(CXXFLAGS) tests.cpp -o $@

check: tests
	./tests

clean:
	rm -f knapsack tests

.PHONY: all check clean
