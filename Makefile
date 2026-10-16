CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

all: knapsack tests experiment

knapsack: knapsack_main.cpp knapsack.hpp
	$(CXX) $(CXXFLAGS) knapsack_main.cpp -o $@

tests: tests.cpp knapsack.hpp
	$(CXX) $(CXXFLAGS) tests.cpp -o $@

experiment: experiment.cpp knapsack.hpp knapsack_generator.h
	$(CXX) $(CXXFLAGS) experiment.cpp -o $@

check: tests
	./tests

gen: gen.cpp knapsack_generator.h
   	$(CXX) $(CXXFLAGS) gen.cpp -o $@

clean:
	rm -f knapsack tests experiment

.PHONY: all check clean
