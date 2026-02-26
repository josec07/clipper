# added by [X] LLM model
# Twitch Chat CLI

A lightweight C++ CLI tool for connecting to Twitch chat - both live IRC and VOD chat downloads. Zero setup required for read-only access.

## Quick Start

```bash
# Build the tools
make all

# Connect to any Twitch channel live (no auth required!)
./bin/twitch_irc --channel shroud --continuous

# Download chat from a VOD
./bin/twitch_vod_chat --video 1234567890 --output chat.json
```

**No OAuth tokens. No app registration. Just works.**

## Features

- **Zero-setup IRC**: Connect anonymously to any Twitch channel
- **Live chat**: Real-time message streaming with <50ms latency
- **VOD chat**: Download complete chat history from any video
- **Simple output**: Pipe-delimited format perfect for scripting
- **Lightweight**: Single binary, no dependencies for IRC tool

## Tools

### twitch_irc

Connect to live Twitch chat in real-time.

```bash
# Anonymous mode (read-only, recommended)
./bin/twitch_irc --channel xqc --continuous

# Authenticated mode (for sending messages)
./bin/twitch_irc --channel xqc --oauth "oauth:your_token" --username "your_name" --continuous
```

**Output format:** `timestamp|username|message`
```
1700000000000|user1|Hello chat
1700000000100|user2|PogChamp
1700000000200|user3|GG
```

**Options:**
- `--channel <name>` - Channel to join (required)
- `--anonymous` - Connect anonymously (default if no oauth)
- `--oauth <token>` - OAuth token (or set TWITCH_OAUTH env)
- `--username <name>` - Username (or set TWITCH_USERNAME env)
- `--continuous` - Keep reading messages
- `--timeout <ms>` - Timeout in milliseconds (default: 30000)

### twitch_vod_chat

Download chat history from any Twitch VOD.

```bash
# Download chat to JSON file
./bin/twitch_vod_chat --video 1234567890 --output chat.json

# Stream chat to stdout
./bin/twitch_vod_chat --video 1234567890
```

**Options:**
- `--video <id>` - Twitch video ID (required)
- `--output <file>` - Output file (optional, defaults to stdout)

## Building

Requirements:
- C++17 compatible compiler (g++ or clang++)
- Make
- libcurl (for VOD chat downloader)
- pkg-config

```bash
# Build both tools
make all

# Build IRC tool only
make twitch_irc

# Build VOD tool only
make twitch_vod

# Clean build artifacts
make clean
```

## Examples

### Live Stream Monitoring
```bash
# Watch chat in real-time
./bin/twitch_irc --channel shroud --continuous

# Filter for specific keywords
./bin/twitch_irc --channel shroud --continuous | grep -i "win\|pog"

# Save to file with timestamps
./bin/twitch_irc --channel shroud --continuous > chat_$(date +%Y%m%d).log
```

### VOD Analysis
```bash
# Download and analyze chat
./bin/twitch_vod_chat --video 1234567890 | jq '.comments | length'

# Get messages per minute
./bin/twitch_vod_chat --video 1234567890 | jq '.comments | group_by(.content_offset_seconds / 60 | floor) | map(length)'
```

### Pipeline Examples
```bash
# Live chat → file with timestamps
./bin/twitch_irc --channel shroud --continuous | tee chat.log

# VOD chat → pretty print
./bin/twitch_vod_chat --video 1234567890 | jq '.comments[] | "\(.commenter.display_name): \(.message.body)"'
```

## Anonymous Mode

The IRC tool can connect anonymously using Twitch's `justinfanXXXXX` accounts. This requires:
- **No OAuth token**
- **No app registration**
- **No setup**

Simply run without credentials:
```bash
./bin/twitch_irc --channel <any_channel> --continuous
```

**Limitations:** Read-only (cannot send messages)

## Authentication (Optional)

For sending messages, get an OAuth token from https://twitchtokengenerator.com or https://twitchapps.com/tmi/

```bash
export TWITCH_OAUTH="oauth:your_token_here"
export TWITCH_USERNAME="your_username"
./bin/twitch_irc --channel shroud --continuous
```

## Architecture

Both tools use native system sockets:
- **IRC**: Direct TCP socket connection to `irc.chat.twitch.tv:6667`
- **VOD**: HTTP requests via libcurl to Twitch's GraphQL API

## License

GNU General Public License v3.0 - See LICENSE file

## Contributing

Contributions welcome for:
- Additional output formats
- Platform support (macOS, Windows)
- Performance optimizations
- Integration examples

---

*Built for streamers, chat bots, and data enthusiasts.*
