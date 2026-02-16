# added by [X] LLM model
# ChatClipper

A lightweight C++ CLI tool for real-time chat signal processing and automatic stream highlight detection. Optimized for gaming streams with 2,000+ viewers.

## Overview

ChatClipper processes live chat messages in real-time and emits clip-worthy timestamps based on:
- **Chat spike detection** (abnormal message rate increases)
- **Keyword matching** with fuzzy search (W/L detection, emotes)
- **Weighted scoring** combining multiple signals

Perfect for streamers who want to auto-generate TikTok-ready 30-60 second clips from their gaming streams.

## Features

- **Real-time processing**: <50ms latency per message
- **Memory efficient**: Only holds 3-5 minutes of chat context (~300 KB)
- **Configurable**: JSON-based keyword and scoring configuration
- **Fuzzy matching**: Levenshtein distance for handling typos (WWW, WW, W)
- **CLI-first**: Simple stdin/file input, JSON output
- **No ML required**: Lightweight rule-based detection

## Quick Start

```bash
# Build
make

# Run tests
./bin/chatclipper --test

# Process sample chat log
./bin/chatclipper --file examples/sample_chat.txt

# Pipe from IRC tool or chat bot
tail -f chat_log.txt | ./bin/chatclipper --stdin
```

## Architecture

ChatClipper consists of 5 core components:

1. **ChatBuffer**: Circular buffer with time-based expiration (3-5 min retention)
2. **SpikeDetector**: Moving average + z-score spike detection
3. **KeywordMatcher**: Fuzzy keyword matching using Levenshtein distance
4. **ScoringEngine**: Weighted score combining signals
5. **ClipDetector**: Emits clip events with 60s window (30s lookback + 30s forward)

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

## References

This project implements algorithms from:
- Wagner & Fischer (1974) - String-to-String Correction Problem
- Ringer et al. (2020) - TwitchChat dataset research
- Ringer (2022) - Multi-modal livestream highlight detection
- Barbieri et al. (2017) - Twitch emote modeling

See `docs/literature/` for full citations and `docs/REFERENCES.md` for comprehensive algorithm documentation and academic attribution.

### Academic Attribution

All algorithms implemented in this project are properly cited with academic references:
- **Levenshtein Distance**: Wagner & Fischer (1974)
- **Fuzzy Matching**: Navarro (2001) survey on approximate string matching
- **Z-Score Anomaly Detection**: Lucas & Saccucci (1990)
- **Online Variance**: Welford (1962)
- **Circular Buffer**: Standard CS literature

See `docs/REFERENCES.md` for complete citations.

## Building

Requirements:
- C++17 compatible compiler
- Make

```bash
make              # Build release version
make clean        # Clean build artifacts
```

## Usage

```bash
# Help
./bin/chatclipper --help

# Read from stdin (default)
./bin/chatclipper

# Read from file
./bin/chatclipper --file chat.log

# With custom config
./bin/chatclipper --config my_config.json --file chat.log

# Run tests
./bin/chatclipper --test
```

## Use Cases

- **Auto-clipping**: Pipe output to video editing tools
- **Stream highlights**: Mark timestamps for later review
- **Analytics**: Track viewer engagement spikes
- **Integration**: Use as backend for OBS dock or web service

## License

MIT License - See LICENSE file

## Contributing

This is a focused open-source project. Contributions welcome for:
- Additional keyword categories
- Platform adapters (Twitch IRC, YouTube chat)
- Performance optimizations
- Integration examples

---

*Built with algorithms. Designed for streamers.*
