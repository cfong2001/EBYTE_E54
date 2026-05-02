CXX = g++
CXXFLAGS = -I../src -Iinclude -Wall -Wextra
TARGET = test_motion_compensation
SRC = test_motion_compensation.cpp

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
