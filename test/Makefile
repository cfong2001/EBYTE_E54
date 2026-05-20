CXX = g++
CXXFLAGS = -std=c++11 -Iinclude -I../src -Wall -Wextra -Wno-unused-parameter -g

TEST_SRCS = $(wildcard *.cpp) ../src/BroadcastServer.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_EXEC = run_tests

.PHONY: all clean test

all: $(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_EXEC)
	./$(TEST_EXEC)

clean:
	rm -f $(TEST_OBJS) $(TEST_EXEC)
