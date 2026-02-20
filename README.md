# ChatClipper

A lightweight C++ CLI tool for real-time chat signal processing and automatic stream highlight detection. Optimized for gaming streams with 2,000+ viewers.

## Overview

ChatClipper processes live chat messages in real-time and emits clip-worthy timestamps based on:
- **Chat spike detection** (abnormal message rate increases)
- **Keyword matching** with fuzzy search (W/L detection, emotes)
- **Weighted scoring** combining multiple signals

Perfect for streamers who want to auto-generate TikTok-ready 30-60 second clips from their gaming streams.

## Quick Start

```bash
# Build all tools
make all

# Connect to any Twitch channel instantly (no auth required!)
./bin/twitch_irc --channel nickmercs --continuous

# Pipe live chat to clip detection
./bin/twitch_irc --channel nickmercs --continuous | ./bin/chatclipper
```

**That's it.** No OAuth tokens. No app registration. No setup. Just works.

## Features

- **Zero-setup IRC**: Connect to Twitch chat anonymously - no auth required
- **Real-time processing**: <50ms latency per message
- **Memory efficient**: Only holds 3-5 minutes of chat context (~300 KB)
- **Configurable**: JSON-based keyword and scoring configuration
- **Fuzzy matching**: Levenshtein distance for handling typos (WWW, WW, W)
- **CLI-first**: Simple stdin/file input, JSON output
- **No ML required**: Lightweight rule-based detection

## Tools

### twitch_irc (Live Chat)

Connect to any Twitch channel in real-time. **No authentication required.**

```bash
# Connect to a channel (anonymous, read-only)
./bin/twitch_irc --channel nickmercs --continuous

# Output format: timestamp_ms|username|message
# 1771615869981|user1|hello
# 1771615870141|user2|PogChamp
```

**Anonymous mode** uses `justinfanXXXXX` accounts - no OAuth, no app registration, works instantly. Read-only (can't send messages).

**Authenticated mode** (optional, for sending messages):
```bash
./bin/twitch_irc --channel tfue --oauth "oauth:your_token" --username "your_username" --continuous
```

### twitch_vod_chat (VOD Chat)

Download chat history from any Twitch VOD:

```bash
# Download chat from VOD
./bin/twitch_vod_chat --video 2699618601 --output chat.json

# Pipe directly to analyzer
./bin/twitch_vod_chat --video 2699618601 | ./bin/chatclipper
```

### chatclipper (Analysis Engine)

Core analysis engine for detecting clip-worthy moments:

```bash
# Analyze a chat file
./bin/chatclipper --file chat.log

# Live analysis from IRC
./bin/twitch_irc --channel nickmercs --continuous | ./bin/chatclipper
```

## Full Pipeline Examples

### Live Stream Monitoring
```bash
# Watch live chat and detect clips in real-time
./bin/twitch_irc --channel nickmercs --continuous | ./bin/chatclipper
```

### VOD Analysis
```bash
# Download VOD chat and find highlights
./bin/twitch_vod_chat --video 2699618601 | ./bin/chatclipper
```

### Save Chat for Later
```bash
# Download chat to file
./bin/twitch_vod_chat --video 2699618601 --output chat.json

# Analyze later
./bin/chatclipper --file chat.json
```

## Architecture

ChatClipper consists of 5 core components:

1. **ChatBuffer**: Circular buffer with time-based expiration (3-5 min retention)
2. **SpikeDetector**: Moving average + z-score spike detection
3. **KeywordMatcher**: Fuzzy keyword matching using Levenshtein distance
4. **ScoringEngine**: Weighted score combining signals
5. **ClipDetector**: Emits clip events with 60s window (30s lookback + 30s forward)

## Project Structure

```
├── src/
│   ├── core/           # Library code (algorithms, processing)
│   └── tools/          # Standalone binaries (twitch_irc, twitch_vod_chat)
├── include/
│   ├── core/           # Library headers
│   └── tools/          # Tool headers
├── main.cpp            # chatclipper entry point
└── Makefile
```

## Input Format

Pipe-delimited chat messages:
```
timestamp_ms|username|message
```

Example:
```
1700000000000|user1|W
1700000000100|user2|PogChamp
1700000000200|user3|insane
```

## Output Format

JSON clip events:
```json
{
  "start_ms": 1700000000000,
  "end_ms": 1700000060000,
  "score": 0.85,
  "category": "win",
  "keywords": ["W", "PogChamp"]
}
```

## Configuration

Edit `examples/config.json` to customize:

```json
{
  "keywords": {
    "wins": ["W", "pog", "clutch", "insane"],
    "losses": ["L", "ff", "sad", "rip"],
    "hype": ["OMEGALUL", "KEKW", "PogChamp"]
  },
  "fuzzy_tolerance": 0.8,
  "weights": {
    "spike": 0.4,
    "keyword": 0.5,
    "uniqueness": 0.1
  },
  "min_clip_score": 0.7,
  "clip_cooldown_seconds": 30
}
```

## Building

Requirements:
- C++17 compatible compiler
- Make
- libcurl (for VOD chat downloader)
- nlohmann_json (for JSON parsing)

```bash
make all            # Build all tools
make twitch_irc     # Build IRC tool only
make twitch_vod     # Build VOD downloader only
make clean          # Clean build artifacts
```

## Usage

```bash
# twitch_irc - Live chat connection
./bin/twitch_irc --channel nickmercs --continuous           # Anonymous (recommended)
./bin/twitch_irc --channel tfue --oauth "oauth:xxx" --username "user"  # Authenticated

# twitch_vod_chat - VOD chat download
./bin/twitch_vod_chat --video 2699618601 --output chat.json

# chatclipper - Analysis
./bin/chatclipper --file chat.log
./bin/chatclipper --test
```

## References

This project implements algorithms from:
- Wagner & Fischer (1974) - String-to-String Correction Problem
- Ringer et al. (2020) - TwitchChat dataset research
- Ringer (2022) - Multi-modal livestream highlight detection
- Barbieri et al. (2017) - Twitch emote modeling

### Academic Attribution

All algorithms implemented in this project are properly cited with academic references:
- **Levenshtein Distance**: Wagner & Fischer (1974)
- **Fuzzy Matching**: Navarro (2001) survey on approximate string matching
- **Z-Score Anomaly Detection**: Lucas & Saccucci (1990)
- **Online Variance**: Welford (1962)
- **Circular Buffer**: Standard CS literature

See `docs/literature/` for citation files (.bib).

## Use Cases

- **Auto-clipping**: Pipe output to video editing tools
- **Stream highlights**: Mark timestamps for later review
- **Analytics**: Track viewer engagement spikes
- **Integration**: Use as backend for OBS dock or web service
- **Chat logging**: Record chat for archival or analysis

## License

GNU General Public License v3.0 - See LICENSE file

## Contributing

This is a focused open-source project. Contributions welcome for:
- Additional keyword categories
- Platform adapters (YouTube chat, Discord)
- Performance optimizations
- Integration examples

---

*Built with algorithms. Designed for streamers.*
