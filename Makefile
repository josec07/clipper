CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -I./include
LDFLAGS = 

TARGET = ctic

SRC = src/main.cpp \
      src/cli/commands.cpp \
      src/core/config.cpp \
      src/core/detection.cpp \
      src/core/text.cpp \
      src/core/chat_buffer.cpp \
      src/core/spike_detector.cpp \
      src/core/monitor.cpp \
      src/core/monitor_pool.cpp \
      src/providers/twitch_irc.cpp

OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ) src/*/*.o

test: $(TARGET)
	./$(TARGET) --help
	./$(TARGET) list

.PHONY: all clean test
