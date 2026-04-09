SHELL := /usr/bin/env bash

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

HW1_SRC := homework-1/main.cpp
HW1_BIN := homework-1/hw1

.PHONY: build hw1 run clean

# Usage:
#   make build hw1
build: hw1

hw1: $(HW1_BIN)

run: hw1
	cd homework-1 && ./hw1

$(HW1_BIN): $(HW1_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(HW1_BIN)
