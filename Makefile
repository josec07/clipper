CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include/core -I./include/tools -O2
OBJDIR = build
BINDIR = bin

CORE_SOURCES = $(wildcard src/core/*.cpp)
CORE_OBJECTS = $(CORE_SOURCES:src/core/%.cpp=$(OBJDIR)/core/%.o)

TARGET = $(BINDIR)/chatclipper

.PHONY: all clean directories twitch_irc twitch_vod test

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJDIR)/core $(BINDIR)

$(TARGET): $(CORE_OBJECTS) $(OBJDIR)/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/core/%.o: src/core/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

CURL_LIBS = $(shell pkg-config --libs libcurl)

twitch_irc: directories
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_irc src/tools/twitch_irc.cpp

twitch_vod: directories
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_vod_chat src/tools/twitch_vod_chat.cpp $(CURL_LIBS)

test: all
	@echo "Running tests..."
	./$(TARGET) --test
