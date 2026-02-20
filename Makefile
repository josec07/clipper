# added by [X] LLM model
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -O2
SRCDIR = src
OBJDIR = build
BINDIR = bin

LIB_SOURCES = $(filter-out $(SRCDIR)/twitch_irc.cpp $(SRCDIR)/twitch_vod_chat.cpp, $(wildcard $(SRCDIR)/*.cpp))
LIB_OBJECTS = $(LIB_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/chatclipper

.PHONY: all clean directories

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(LIB_OBJECTS) $(OBJDIR)/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

CURL_LIBS = $(shell pkg-config --libs libcurl)
JSON_LIBS = $(shell pkg-config --libs nlohmann_json 2>/dev/null || echo "")

TWITCH_IRC = $(BINDIR)/twitch_irc
TWITCH_VOD = $(BINDIR)/twitch_vod_chat

twitch_irc: directories
	$(CXX) $(CXXFLAGS) -o $(TWITCH_IRC) src/twitch_irc.cpp

twitch_vod: directories
	$(CXX) $(CXXFLAGS) -o $(TWITCH_VOD) src/twitch_vod_chat.cpp $(CURL_LIBS)

test: all
	@echo "Running tests..."
	./$(TARGET) --test
