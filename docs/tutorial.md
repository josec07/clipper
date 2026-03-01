# CTIC Tutorial

Detect clip-worthy moments from Twitch chat in real-time.

## Quick Start

```bash
./ctic add tfue https://twitch.tv/tfue
./ctic run
cat .ctic/outputs/tfue/high/*.csv
```

## How It Works

CTIC watches Twitch chat and detects when multiple viewers react simultaneously with similar words.

**Pipeline**: `Twitch IRC → Parse → Tier Match → Burst Detect → CSV`

## Default Thresholds

| Tier | Burst Thresh | Min Word Len | Cooldown | Users |
|------|--------------|--------------|----------|-------|
| high | 3 | 3 | 60s | 2 |
| high-negative | 5 | 1 | 45s | 3 |
| medium | 8 | 1 | 90s | 4 |
| easy | 15 | 2 | 120s | 5 |

## Tuning Knobs

### burst_threshold
Number of matching messages needed. Lower = more sensitive.

| Value | Use Case |
|-------|----------|
| 2 | Small chats (<500 viewers) |
| 3 | Default for most streams |
| 5+ | Large chats (10k+ viewers) |

### min_word_length
Minimum matched word length to count.

| Value | Effect |
|-------|--------|
| 1 | "K", "W", "L" all count |
| 3 | Filters single letters |
| 4+ | Only substantial words |

### cooldown_seconds
Wait time after detection before tier can trigger again.

Prevents one moment from producing multiple CSV rows.

### require_unique_users
Minimum different usernames in burst.

Prevents one spammer from triggering false detections.

## Profiles

Edit `.ctic/profiles/balanced.json` to tune thresholds.

```json
{
  "tiers": {
    "high": {
      "burst_threshold": 3,
      "min_word_length": 3,
      "cooldown_seconds": 60,
      "require_unique_users": 2
    }
  }
}
```

Available profiles: `balanced` (default), `aggressive`, `conservative`.

## Common Scenarios

**Too many false positives?**
- Increase `burst_threshold` (3 → 5)
- Increase `min_word_length` (3 → 4)
- Switch to `conservative` profile

**Missing obvious moments?**
- Decrease `burst_threshold` (3 → 2)
- Switch to `aggressive` profile

**Small chat not triggering?**
```json
"high": { "burst_threshold": 2, "require_unique_users": 1 }
```

**Large chat too noisy?**
```json
"high": { "burst_threshold": 7, "cooldown_seconds": 120 }
```

## Output Format

```csv
timestamp,matched_word,burst_count,users_matched,...
2026-03-01T10:21:06Z,POG,4,4,...
```

| Column | Meaning |
|--------|---------|
| timestamp | When detection triggered |
| matched_word | First matched word |
| burst_count | Messages in window |
| users_matched | Unique chatters |

## Custom Word Lists

Edit `src/core/config.cpp` and rebuild:

```cpp
config.words = {
    "POG", "POGGERS", "INSANE",
    "MY_CUSTOM_WORD",
};
```

```bash
make clean && make
```

## Next Steps

1. Run on a streamer for 10-15 minutes
2. Check `.ctic/outputs/` for CSV results
3. Tune thresholds if needed
4. Iterate

For algorithm details, see [references.md](references.md).
