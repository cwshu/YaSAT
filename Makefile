# A template C++ Makefile for your SAT solver.

CXX=g++

# Debugging flags
FLAGS=-Wall -Wold-style-cast -Wformat=2 -pedantic -ggdb3 \
-DDEBUG \
-std=c++11

# Optimizing flags
#FLAGS=-Wall -Wold-style-cast -Wformat=2 -pedantic -O3 \
-std=c++11

# List all the .o files you need to build here
OBJS=parser.o sat.o sat_solver.o

# This is the name of the executable file that gets built.  Please
# don't change it.
EXENAME=yasat

# Compile targets
all: $(OBJS)
	$(CXX) $(FLAGS) $(OBJS) -lz -o $(EXENAME)
parser.o: parser.cpp parser.h
	$(CXX) $(FLAGS) -c parser.cpp
sat.o: sat.cpp parser.h
	$(CXX) $(FLAGS) -c sat.cpp

# Add more compilation targets here

%.o: %.cpp
	$(CXX) $(FLAGS) -c $^


# The "phony" `clean' compilation target.  Type `make clean' to remove
# your object files and your executable.
.PHONY: clean
clean:
	rm -rf $(OBJS) $(EXENAME)
