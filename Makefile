CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -O2
BINDIR = bin

.PHONY: all clean directories twitch_irc twitch_vod

all: directories twitch_irc twitch_vod

directories:
	@mkdir -p $(BINDIR)

twitch_irc: directories
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_irc src/twitch_irc.cpp

twitch_vod: directories
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_vod_chat src/twitch_vod_chat.cpp $(shell pkg-config --libs libcurl)

clean:
	rm -rf $(BINDIR)
