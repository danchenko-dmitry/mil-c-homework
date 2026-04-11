SHELL := /usr/bin/env bash

CXX ?= g++
JSON_INCLUDE := third_party/json/include
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -I$(JSON_INCLUDE)

HW1_SRC := homework-1/main.cpp
HW1_BIN := homework-1/hw1

HW1_TEST_SRC := homework-1/test/test.cpp
HW1_TEST_BIN := homework-1/hw1-test

.PHONY: build hw1 hw1-test run run-hw1-test clean

# Usage:
#   make build hw1
#   make hw1-test
#   make run
#   make run hw1-test   (runs ./hw1-test; same as make run-hw1-test)
build: hw1

hw1: $(HW1_BIN)

hw1-test: $(HW1_TEST_BIN)

# When "hw1-test" is among the goals (e.g. make run hw1-test), run the test binary.
run:
ifeq ($(filter hw1-test,$(MAKECMDGOALS)),hw1-test)
	$(MAKE) hw1-test
	cd homework-1 && ./hw1-test
else
	$(MAKE) hw1
	cd homework-1 && ./hw1
endif

$(HW1_BIN): $(HW1_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(HW1_TEST_BIN): $(HW1_TEST_SRC) $(HW1_SRC)
	$(CXX) $(CXXFLAGS) -D_LIBRARY_ -o $@ $(HW1_TEST_SRC) $(HW1_SRC)

clean:
	rm -f $(HW1_BIN) $(HW1_TEST_BIN)
