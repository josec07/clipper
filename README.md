# CTIC - Chat Twitch Intelligence CLI

A terminal-based service for detecting clip-worthy moments from Twitch chat using burst detection and Z-score anomaly analysis. Zero setup required for read-only access.

## Quick Start

```bash
# Build
make

# Add a creator to monitor
./ctic add https://twitch.tv/xqc

# Start monitoring all creators
./ctic run
```

**No OAuth tokens. No app registration. Multi-creator support. Just works.**

## Features

- **Zero-setup IRC**: Connect anonymously to any Twitch channel
- **Multi-creator monitoring**: Simultaneous monitoring with thread pool
- **Burst detection**: Detect hype moments via Levenshtein similarity
- **Z-score spike detection**: Statistical anomaly detection with Welford's algorithm
- **Tiered detection**: High (rare), Medium (moderate), Easy (common) word lists
- **CSV output**: Full metadata logging with traceability
- **State tracking**: JSON state file for session management

## Installation

### Requirements

- C++17 compatible compiler (g++ or clang++)
- Make
- POSIX sockets (Linux/macOS)

### Build

```bash
# Build the CLI
make

# Build and run tests
cd tests && make && ./test_runner
```

## Commands

### `ctic add <url>`

Add a creator to monitor. Tests connection for 30 seconds before saving.

```bash
./ctic add https://twitch.tv/xqc
./ctic add xqc                    # Also accepts channel name
```

Output:
- Saves config to `.ctic/creators/xqc.json`
- Creates output folders `.ctic/outputs/xqc/{high,medium,easy}/`

### `ctic list`

List all configured creators.

```bash
./ctic list
```

### `ctic status [name]`

Test connection to all creators or a specific one.

```bash
./ctic status                 # Test all
./ctic status xqc             # Test specific
```

### `ctic run`

Start monitoring all configured creators simultaneously. Each creator runs in a separate thread.

```bash
./ctic run
```

Output:
- CSV logs to `.ctic/outputs/{creator}/{tier}/matches-*.csv`
- State saved to `.ctic/state.json`
- Press Ctrl+C to stop all monitors

### `ctic remove <name>`

Remove a creator from monitoring.

```bash
./ctic remove xqc
```

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  CLI Layer (src/cli/commands.cpp)                   │
└─────────────────────┬───────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────┐
│  Core Layer (src/core/)                             │
│  ├── MonitorPool      → Thread pool management      │
│  ├── Monitor          → Per-creator monitoring      │
│  ├── ChatBuffer       → Time-based sliding windows │
│  ├── SpikeDetector    → Z-score anomaly detection   │
│  ├── Detector         → Burst detection logic       │
│  ├── text.cpp         → Levenshtein similarity      │
│  └── config.cpp       → Config management            │
└─────────────────────┬───────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────┐
│  Provider Layer (src/providers/)                    │
│  ├── twitch_irc.cpp   → IRC socket connection       │
│  └── twitch_url.h     → URL parsing (header-only)   │
└─────────────────────────────────────────────────────┘
```

## Detection

### Burst Detection

Detects when multiple similar words appear in a time window using Levenshtein distance:

```
Message: "POG"
Message: "POGGG"     ← 85% similar to "POG"
Message: "POG"      ← 100% match
→ BURST DETECTED (threshold: 3 in 30s)
```

### Z-Score Spike Detection

Uses Welford's online variance algorithm to detect abnormal message rates:

```
Baseline: 5 msg/sec
Current:  25 msg/sec
Z-score:  4.0σ
→ SPIKE DETECTED
```

### Tiers

| Tier | Words | Threshold | Use Case |
|------|-------|-----------|----------|
| **High** | INSANE, CLUTCH, POGCHAMP, ACE, PENTA | 3 messages | Rare viral moments |
| **Medium** | W, POG, GGS, NICE, LETS GO | 5 messages | Moderate hype |
| **Easy** | lol, wow, nice, gg, crazy | 10 messages | Common engagement |

## Output

### CSV Format

`.ctic/outputs/{creator}/{tier}/matches-*.csv`:

```csv
timestamp,matched_word,sentiment,burst_count,spike_z_score,users_matched,spike_intensity,config_id,sample_messages
2026-02-28T12:34:02Z,INSANE,positive,3,4,4,0.80,default,"user1: INSANE | user2: insane | user3: INSANITY"
```

### State File

`.ctic/state.json`:

```json
{
  "xqc": {
    "creator_name": "xqc",
    "running": false,
    "connected": false,
    "messages_processed": 1523,
    "bursts_detected": 7
  }
}
```

### Creator Config

`.ctic/creators/xqc.json`:

```json
{
  "name": "xqc",
  "channel": "xqc",
  "twitch_url": "https://twitch.tv/xqc",
  "enabled_tiers": ["high", "medium", "easy"],
  "detector_config": "default",
  "total_sessions": 5,
  "total_clips_detected": 127
}
```

## Testing

```bash
cd tests
make test              # Run all tests
make test_text         # Run text processing tests
make test_detector     # Run burst detection tests
make test_spike        # Run spike detector tests
```

Current coverage: **133 assertions in 33 test cases**

## Project Structure

```
.
├── ctic                      ← Compiled binary
├── Makefile
│
├── src/
│   ├── main.cpp
│   ├── cli/commands.cpp
│   ├── core/
│   │   ├── monitor.cpp
│   │   ├── monitor_pool.cpp
│   │   ├── chat_buffer.cpp
│   │   ├── spike_detector.cpp
│   │   ├── detection.cpp
│   │   ├── text.cpp
│   │   └── config.cpp
│   └── providers/
│       └── twitch_irc.cpp
│
├── include/
│   ├── CLI11.hpp
│   ├── cli/
│   ├── core/
│   └── providers/
│
├── tests/
│   ├── catch_amalgamated.hpp
│   ├── test_text.cpp
│   ├── test_url.cpp
│   ├── test_spike_detector.cpp
│   ├── test_detector.cpp
│   └── test_config.cpp
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── INDEX.md
│   └── REFERENCES.md
│
└── .ctic/                    ← Created at runtime
    ├── creators/*.json
    ├── outputs/{creator}/{tier}/*.csv
    └── state.json
```

## Documentation

- `docs/ARCHITECTURE.md` - Technical architecture guide
- `docs/INDEX.md` - Algorithm reference index
- `docs/REFERENCES.md` - Academic citations

## References

This project implements algorithms from the following research:

- **Levenshtein Distance**: Levenshtein, V. I. (1966). "Binary codes capable of correcting deletions, insertions, and reversals"
- **Welford's Algorithm**: Welford, B. P. (1962). "Note on a method for calculating corrected sums of squares and products"
- **Z-Score Anomaly Detection**: Standard statistical outlier detection

## License

GNU General Public License v3.0 - See LICENSE file

---

*Built for clippers, streamers, and chat analysts.*
