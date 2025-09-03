CC=gcc
CXX=g++

CFLAGS=-O3 -std=c17 
CXXFLAGS=-O3 -std=c++17 
# Preprocessing flags, not CPP flags
CPPFLAGS=

LIBS=-pthread

# First target is the default target
all: problem3

BIN=./bin
$(shell mkdir -p $(BIN))

.DEFAULT_TARGET: problem3

# $@ is a variable that contains the target name.
problem3: problem_3/220531.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $(BIN)/$@.out $(LIBS)

# The target is not a filename, always run the rule even if there is a file
.PHONY: clean
clean:
	rm -f -- bin *.out compile_commands.json
