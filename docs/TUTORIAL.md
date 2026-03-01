# CTIC Tutorial: Opinionated Detection Pipeline

**Goal**: An adaptable CLI tool for detecting clip-worthy moments from Twitch chat, with opinionated defaults and user-tunable weights.

---

## Quick Start

```bash
# Add a creator
./ctic add tfue https://twitch.tv/tfue

# Start monitoring (Ctrl+C to stop)
./ctic run

# Check results
cat .ctic/outputs/tfue/high/*.csv
```

That's it. You're now detecting moments using **opinionated defaults** that work for most streamers.

---

## What CTIC Does

CTIC watches Twitch chat in real-time and detects when something interesting happens. The core insight:

> **When chat explodes with similar reactions simultaneously, something clip-worthy occurred on stream.**

The tool is **opinionated**: it comes with pre-configured word lists, thresholds, and detection strategies based on academic research.

---

## The Detection Pipeline (End-to-End)

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐     ┌────────────┐
│  Twitch IRC │────►│   Parse &    │────►│   Tiered    │────►│  CSV Output│
│   Stream    │     │   Normalize  │     │  Detectors  │     │            │
└─────────────┘     └──────────────┘     └─────────────┘     └────────────┘
                           │                     │
                           │                     │
                           ▼                     ▼
                    ┌─────────────┐       ┌─────────────┐
                    │  Word Lists │       │   Profile   │
                    │  (hardcoded)│       │   Weights   │
                    └─────────────┘       └─────────────┘
```

### Stage 1: IRC Connection & Parsing

CTIC connects to Twitch IRC anonymously (no OAuth needed for reading):

```
Raw message:
@badge-info=;color=#FF0000;display-name=xQcOW;emotes=;id=abc123;mod=0;... :xQcOW!xQcOW@xQcOW.tmi.twitch.tv PRIVMSG #xqcow :POGGERS that was INSANE

                │
                ▼

Parsed:
  username: "xQcOW"
  content:  "POGGERS that was INSANE"   ← IRC metadata stripped
```

**Key parsing step**: IRC tags are stripped using `rfind(" :")` to find the actual message content. This prevents garbage like `@badge-info=;mod=0;...` from polluting your data.

### Stage 2: Message Distribution to Tiers

Each message is checked against **all enabled tiers** in parallel:

```
Message: "POGGERS that was INSANE"
                │
        ┌───────┼───────┬───────┐
        ▼       ▼       ▼       ▼
      high   medium   easy   high-negative
       │       │       │         │
       ▼       │       │         │
   "POGGERS"  │       │         │
   "INSANE"   │       │         │
   matched!   │       │         │
       │       │       │         │
       ▼       ▼       ▼         ▼
    process  process process   process
```

### Stage 3: Per-Tier Detection (The Core Logic)

Each tier has its own **Detector** with configurable behavior:

```
┌────────────────────────────────────────────────────────────────┐
│                     TIER: "high"                               │
│                                                                │
│  CONFIG (from profile):                                        │
│    burst_threshold:      3     ← need 3+ matches              │
│    min_word_length:      3     ← filter "K", "W" etc.         │
│    cooldown_seconds:    60     ← wait 60s after detection    │
│    require_unique_users: 2     ← need 2+ different chatters   │
│                                                                │
│  PROCESS:                                                      │
│                                                                │
│  1. CHECK MATCH                                                │
│     ┌────────────────────────────────────────────────────┐    │
│     │  word_matches("POG", "POGGERS that was INSANE")    │    │
│     │                                                    │    │
│     │  Algorithm:                                        │    │
│     │    if word.length > 4:                             │    │
│     │        substring_match("INSANE" in message) ✓      │    │
│     │    else:                                           │    │
│     │        tokenize → [POGGERS, THAT, WAS, INSANE]    │    │
│     │        for each token:                             │    │
│     │            collapse_repeated_chars(POGGERS→POGERS) │    │
│     │            levenshtein_similarity(POG, POGGERS)   │    │
│     │            if sim >= 0.8: MATCH ✓                   │    │
│     └────────────────────────────────────────────────────┘    │
│                     │                                         │
│                     ▼ matched_word = "POG"                    │
│                                                                │
│  2. FILTER BY min_word_length                                  │
│     ┌────────────────────────────────────────────────────┐    │
│     │  matched_word = "POG"                             │    │
│     │  matched_word.length = 3                          │    │
│     │  min_word_length = 3                              │    │
│     │  3 >= 3 → PASS ✓                                  │    │
│     │                                                    │    │
│     │  (if matched "K", length=1, would FAIL ✗)         │    │
│     └────────────────────────────────────────────────────┘    │
│                     │                                         │
│                     ▼                                         │
│  3. COUNT BURST                                                │
│     ┌────────────────────────────────────────────────────┐    │
│     │  BurstDetector (time window: 30s)                  │    │
│     │                                                    │    │
│     │  Recent matches in window:                         │    │
│     │    [t=0s] user1: "POG"                            │    │
│     │    [t=1s] user2: "POGGERS"  ← similar to "POG"    │    │
│     │    [t=2s] user3: "POG!!"    ← similar to "POG"    │    │
│     │    [t=3s] user4: "POG"      ← current message     │    │
│     │                                                    │    │
│     │  count = 4 (all have similarity >= 0.8 to "POG")   │    │
│     │  unique_users = 4                                  │    │
│     └────────────────────────────────────────────────────┘    │
│                     │                                         │
│                     ▼                                         │
│  4. THRESHOLD CHECK                                           │
│     ┌────────────────────────────────────────────────────┐    │
│     │  count (4) >= burst_threshold (3)?      YES ✓      │    │
│     │  unique_users (4) >= require (2)?        YES ✓      │    │
│     │  in_cooldown?                            NO ✓       │    │
│     │                                                    │    │
│     │  → DETECTED!                                       │    │
│     │  → Set cooldown for 60 seconds                     │    │
│     │  → Reset burst counter for this tier               │    │
│     └────────────────────────────────────────────────────┘    │
│                     │                                         │
│                     ▼                                         │
│  5. OUTPUT CSV ROW                                            │
│     timestamp,matched_word,burst_count,users_matched,...     │
│     2026-03-01T10:21:06Z,POG,4,4,...                         │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

## The Opinionated Defaults

CTIC ships with **working defaults** based on real Twitch chat analysis:

### Word Lists (Hardcoded, Curated)

```cpp
// src/core/config.cpp

high: [
  "POG", "POGGERS", "POGCHAMP", "INSANE", "CLUTCH", "ACE", "PENTA",
  "CRACKED", "GOATED", "DIFF", "WTF", "OMFG", "SHEESH", "DAMN",
  "LETS GO", "GOAT", "ONE TAP", "WORLD RECORD", ...
  // ~42 words total
]

high-negative: [
  "L", "LMAO", "LOST", "BOT", "TRASH", "CRINGE", "OMEGALUL",
  "CHOKE", "SKILL ISSUE", "REPORT", "UNINSTALL", ...
  // ~40 words total
]

medium: [
  "W", "GG", "GGS", "NICE", "SHEESH", "KEKW", "BASED", ...
  // ~40 words total
]

easy: [
  "lol", "wow", "true", "bruh", "fr", "no cap", ...
  // ~40 words total
]
```

### Thresholds (Via Profiles)

| Tier | Burst Thresh | Min Word Len | Cooldown | Unique Users |
|------|--------------|--------------|----------|--------------|
| **high** | 3 | 3 | 60s | 2 |
| **high-negative** | 5 | 1 | 45s | 3 |
| **medium** | 8 | 1 | 90s | 4 |
| **easy** | 15 | 2 | 120s | 5 |

**Why these values?**

- `min_word_length=3` for high tier: Filters "K" matching "Kobe", "OK", etc.
- `cooldown_seconds=60`: Prevents spam from one moment
- `require_unique_users=2`: Requires crowd consensus, not one spammer
- `burst_threshold` varies by tier: High-sentiment words are rarer (lower threshold)

---

## Tuning Without Code: Profiles

The entire detection behavior is configurable via JSON.

### Profile System

```
.ctic/profiles/
├── balanced.json      ← Default, recommended for variety streamers
├── aggressive.json    ← Use for smaller chats or more detections
└── conservative.json  ← Use for huge chats or cleaner output
```

### Example: balanced.json

```json
{
  "profile_name": "balanced",
  "description": "Good for variety streamers - balanced between sensitivity and noise",
  "tiers": {
    "high": {
      "burst_threshold": 3,
      "min_word_length": 3,
      "cooldown_seconds": 60,
      "require_unique_users": 2
    },
    "high-negative": {
      "burst_threshold": 5,
      "min_word_length": 1,
      "cooldown_seconds": 45,
      "require_unique_users": 3
    },
    "medium": {
      "burst_threshold": 8,
      "min_word_length": 1,
      "cooldown_seconds": 90,
      "require_unique_users": 4
    },
    "easy": {
      "burst_threshold": 15,
      "min_word_length": 2,
      "cooldown_seconds": 120,
      "require_unique_users": 5
    }
  }
}
```

### Changing Profiles

Edit creator config:

```bash
# .ctic/creators/tfue.json
{
  "name": "tfue",
  "channel": "tfue",
  "profile": "aggressive",    ← Change this
  "enabled_tiers": ["high", "medium", "easy"]
}
```

### Creating Custom Profiles

```bash
# Copy and edit
cp .ctic/profiles/balanced.json .ctic/profiles/my-custom.json

# Edit thresholds
nano .ctic/profiles/my-custom.json

# Use it
# Edit .ctic/creators/tfue.json: "profile": "my-custom"
```

---

## Understanding Each Knob

### `burst_threshold`

**What it does**: Number of matching messages needed in the time window.

| Value | Effect | Use Case |
|-------|--------|----------|
| 2 | Very sensitive, more false positives | Small chats (<500 viewers) |
| 3 (default for high) | Balanced | Most streamers |
| 5+ | Conservative, only clear moments | Huge chats (10k+ viewers) |

**Tuning tip**: Start with defaults. If too many detections in low-hype moments, increase. If missing obvious moments, decrease.

### `min_word_length`

**What it does**: Minimum matched word length to count.

| Value | Effect | Example |
|-------|--------|---------|
| 1 | All words count | "K", "W", "L" all match |
| 2 | Filters single letters | "GG" counted, "K" ignored |
| 3 | Only 3+ char words | "POG" counted, "W" ignored |
| 4+ | Only substantial words | "INSANE" counted, "GG" ignored |

**Why it matters**: Single letters ("K", "W", "L") match too many unrelated contexts:
- "K" matches "kobe", "OK", "know", etc.
- Setting `min_word_length=3` for high tier filters these false positives

### `cooldown_seconds`

**What it does**: Time to wait after a detection before that tier can trigger again.

| Value | Effect |
|-------|--------|
| 30s | Quick reset, may get cascades |
| 60s (default high) | Balanced |
| 90s+ | One detection per extended moment |

**Problem solved**: Without cooldown, one hype moment produces 10+ CSV rows:
```
t=10:21:06 - DETECTED (burst_count=3)
t=10:21:07 - DETECTED (burst_count=4)  ← same moment
t=10:21:08 - DETECTED (burst_count=5)  ← same moment
...
```

With cooldown=60s, you get ONE detection per tier per minute maximum.

### `require_unique_users`

**What it does**: Minimum different usernames in the burst.

| Value | Effect |
|-------|--------|
| 1 | Any single user can trigger (spam-vulnerable) |
| 2 (default high) | Requires at least 2 people |
| 4+ | Requires crowd consensus |

**Why it matters**: Prevents one spammer from triggering false detections.

---

## Common Tuning Scenarios

### Scenario 1: "Too Many False Positives"

**Symptom**: Detections when nothing interesting happened.

**Fixes**:
1. Increase `burst_threshold` (e.g., 3 → 5)
2. Increase `require_unique_users` (e.g., 2 → 3)
3. Increase `min_word_length` to filter short words
4. Switch to "conservative" profile

```json
"high": {
  "burst_threshold": 5,        // was 3
  "min_word_length": 4,        // was 3
  "require_unique_users": 3    // was 2
}
```

### Scenario 2: "Missing Obvious Moments"

**Symptom**: Chat goes wild but no detection.

**Fixes**:
1. Decrease `burst_threshold` (e.g., 3 → 2)
2. Check if words are in the list (add if needed in `src/core/config.cpp`)
3. Use "aggressive" profile

### Scenario 3: "Small Chat (<500 viewers)"

**Symptom**: Not enough messages to reach thresholds.

**Fixes**:
```json
// aggressive.json overrides
"high": {
  "burst_threshold": 2,
  "require_unique_users": 1
}
```

### Scenario 4: "Huge Chat (10k+ viewers)"

**Symptom**: Too many detections, noise.

**Fixes**:
```json
// conservative.json
"high": {
  "burst_threshold": 7,
  "require_unique_users": 5,
  "cooldown_seconds": 120
}
```

### Scenario 5: "Want Negative Moments (Fails)"

**Symptom**: Want to clip fails/disasters, not just hype.

**Fix**:
Ensure creator has `high-negative` tier enabled:
```json
// .ctic/creators/tfue.json
"enabled_tiers": ["high", "high-negative", "medium", "easy"]
```

---

## Advanced: Modifying Word Lists

Word lists are currently hardcoded in `src/core/config.cpp`. To add/remove words:

```cpp
// src/core/config.cpp:164-171 (high tier)
if (tier_name == "high") {
    config.words = {
        "POG", "POGGERS", "INSANE",
        // ADD YOUR CUSTOM WORDS HERE
        "MY_CUSTOM_HYPE_WORD", "STREAMER_SPECIFIC_EMOTE",
        // ...
    };
}
```

Then rebuild:
```bash
make clean && make
```

**Future**: Word list customization via JSON (planned).

---

## The Academic Foundation

The algorithms used are from peer-reviewed research:

| Technique | Paper | Use |
|-----------|-------|-----|
| Levenshtein Distance | Wagner & Fischer (1974) | Fuzzy word matching |
| Z-Score Anomaly Detection | Lucas & Saccucci (1990) | Chat rate spike detection |
| Welford's Algorithm | Welford (1962) | Online variance calculation |
| Approximate String Matching | Navarro (2001) | Fuzzy matching survey |

See `docs/REFERENCES.md` for full citations.

---

## Output Format

```csv
timestamp,matched_word,sentiment,burst_count,spike_z_score,users_matched,spike_intensity,config_id,sample_messages
2026-03-01T10:21:06Z,POG,positive,4,2,4,0.24,balanced,"user1: POG | user2: POGGERS | user3: POG!! | user4: POG"
```

| Column | Meaning |
|--------|---------|
| timestamp | When detection triggered |
| matched_word | The word that matched (first match) |
| sentiment | "positive" or "negative" |
| burst_count | How many similar messages in window |
| spike_z_score | Chat rate spike (0-5 scale) |
| users_matched | Unique usernames in burst |
| spike_intensity | Raw spike value (0.0-1.0) |
| config_id | Profile used |
| sample_messages | Last 10 messages in buffer |

---

## Philosophy

**Opinionated by default, adaptable by design.**

1. **Working defaults**: Install and run, get usable results
2. **Tunable without code**: JSON configs for all detection parameters
3. **Transparent pipeline**: Every step is documented and visible
4. **Research-backed**: Algorithms from academic literature
5. **Open source**: Modify anything, extend as needed

---

## Next Steps

1. **Run CTIC** on your favorite streamer
2. **Observe output** for 10-15 minutes
3. **Identify issues**: Too many? Too few? Wrong moments?
4. **Tune profile**: Edit `.ctic/profiles/balanced.json`
5. **Iterate**: Small changes, test, repeat

For questions or contributions: [GitHub Issues](https://github.com/your-repo/ctic/issues)
