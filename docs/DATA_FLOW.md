# CTIC Data Flow Architecture

Visual reference for the complete data processing pipeline.

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           CTIC Data Processing Pipeline                          │
└─────────────────────────────────────────────────────────────────────────────────┘

                                    ┌──────────────┐
                                    │  Twitch IRC  │
                                    │  irc.chat.   │
                                    │ .twitch.tv   │
                                    └──────┬───────┘
                                           │ Raw IRC stream
                                           │ @tags; :user! PRIVMSG #channel :msg
                                           ▼
                           ┌───────────────────────────────┐
                           │      TwitchIRC::parse_message │
                           │  ┌─────────────────────────┐  │
                           │  │ 1. Find "PRIVMSG #chan" │  │
                           │  │ 2. Extract username    │  │
                           │  │ 3. rfind(" :") for msg │  │
                           │  │ 4. Strip IRC tags      │  │
                           │  └─────────────────────────┘  │
                           └───────────────┬───────────────┘
                                           │
                              ┌────────────┴────────────┐
                              │ username: "xQcOW"       │
                              │ content: "POGGERS"      │
                              └────────────┬────────────┘
                                           │
                                           ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                 MONITOR                                         │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                           processMessage()                                  │  │
│  │                                                                            │  │
│  │   ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────┐   │  │
│  │   │   ChatBuffer    │    │  SpikeDetector  │    │  SpikeDetector      │   │  │
│  │   │  (time window)  │    │   (rate calc)   │    │   (Z-score check)   │   │  │
│  │   │                 │    │                 │    │                     │   │  │
│  │   │  300s sliding   │    │  msg/sec rate   │    │  mean + 3σ = spike  │   │  │
│  │   │  window         │    │                 │    │                     │   │  │
│  │   └─────────────────┘    └────────┬────────┘    └─────────────────────┘   │  │
│  │                                   │                                       │  │
│  │                                   │ spike_intensity                        │  │
│  │                                   ▼                                       │  │
│  │   ┌─────────────────────────────────────────────────────────────────────┐ │  │
│  │   │                     DETECTOR (per tier)                             │ │  │
│  │   │                                                                     │ │  │
│  │   │  ┌──────────────────────────────────────────────────────────────┐  │ │  │
│  │   │  │  TIER: "high"        TIER: "medium"      TIER: "easy"        │  │ │  │
│  │   │  │  ─────────────       ──────────────      ─────────────       │  │ │  │
│  │   │  │  min_word_len: 3     min_word_len: 1    min_word_len: 2      │  │ │  │
│  │   │  │  burst_thresh: 3     burst_thresh: 8    burst_thresh: 15     │  │ │  │
│  │   │  │  cooldown: 60s       cooldown: 90s      cooldown: 120s      │  │ │  │
│  │   │  │  unique_users: 2      unique_users: 4    unique_users: 5     │  │ │  │
│  │   │  └──────────────────────────────────────────────────────────────┘  │ │  │
│  │   │                                                                     │ │  │
│  │   │  ┌─────────────────────────────────────────────────────────────┐   │ │  │
│  │   │  │                    check_match()                            │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  │   content = "POGGERS that was INSANE"                       │   │ │  │
│  │   │  │                         │                                    │   │ │  │
│  │   │  │                         ▼                                    │   │ │  │
│  │   │  │   ┌─────────────────────────────────────────────────────┐   │   │ │  │
│  │   │  │   │  word_matches(word, message, threshold=0.8)        │   │   │ │  │
│  │   │  │   │                                                     │   │   │ │  │
│  │   │  │   │   for word in [POG, POGGERS, INSANE, CLUTCH...]:  │   │   │ │  │
│  │   │  │   │                                                     │   │   │ │  │
│  │   │  │   │   if word.length > 4:                               │   │   │ │  │
│  │   │  │   │       substring_match(word, message)                │   │   │ │  │
│  │   │  │   │                                                     │   │   │ │  │
│  │   │  │   │   if word.length <= 4:                              │   │   │ │  │
│  │   │  │   │       tokenize(message) → [POGGERS, THAT, WAS...]  │   │   │ │  │
│  │   │  │   │       for token in tokens:                          │   │   │ │  │
│  │   │  │   │           collapse_repeated_chars(WWWW → W)         │   │   │ │  │
│  │   │  │   │           levenshtein_similarity(word, token)        │   │   │ │  │
│  │   │  │   │           if sim >= 0.8: MATCH!                      │   │   │ │  │
│  │   │  │   └─────────────────────────────────────────────────────┘   │   │ │  │
│  │   │  │                         │                                    │   │ │  │
│  │   │  │                         ▼ matched_word = "POG"              │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  └─────────────────────────────────────────────────────────────┘   │ │  │
│  │   │                                                                     │ │  │
│  │   │  ┌─────────────────────────────────────────────────────────────┐   │ │  │
│  │   │  │                 FILTER LAYER                                 │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  │   matched_word.length >= min_word_length?                   │   │ │  │
│  │   │  │       │                                                    │   │ │  │
│  │   │  │       ├─ NO (e.g. "K" in high tier where min=3)            │   │ │  │
│  │   │  │       │    → RETURN (no detection)                         │   │ │  │
│  │   │  │       │                                                    │   │ │  │
│  │   │  │       └─ YES → continue                                    │   │ │  │
│  │   │  └─────────────────────────────────────────────────────────────┘   │ │  │
│  │   │                                                                     │ │  │
│  │   │  ┌─────────────────────────────────────────────────────────────┐   │ │  │
│  │   │  │                 BurstDetector                                │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  │   count_burst(matched_word, username, timestamp)           │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  │   ┌─────────────────────────────────────────────────────┐   │   │ │  │
│  │   │  │   │  recent_matches_ (deque, time-windowed)             │   │   │ │  │
│  │   │  │   │                                                     │   │   │ │  │
│  │   │  │   │   [usr1:POG@t=0s]                                    │   │   │ │  │
│  │   │  │   │   [usr2:POG@t=1s]  ←── same matched_word            │   │   │ │  │
│  │   │  │   │   [usr3:POG@t=2s]                                   │   │   │ │  │
│  │   │  │   │   [usr1:POG@t=3s]  ←── count = 4                     │   │   │ │  │
│  │   │  │   │                                                     │   │   │ │  │
│  │   │  │   │   unique_users() = 3                                 │   │   │ │  │
│  │   │  │   └─────────────────────────────────────────────────────┘   │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  └─────────────────────────────────────────────────────────────┘   │ │  │
│  │   │                                                                     │ │  │
│  │   │  ┌─────────────────────────────────────────────────────────────┐   │ │  │
│  │   │  │              THRESHOLD CHECK                                  │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  │   count >= burst_threshold?       (e.g. 4 >= 3: YES)        │   │ │  │
│  │   │  │   unique_users >= required?     (e.g. 3 >= 2: YES)         │   │ │  │
│  │   │  │   in_cooldown?                   (NO)                       │   │ │  │
│  │   │  │                                                             │   │ │  │
│  │   │  │   → ALL PASS → DETECTED!                                    │   │ │  │
│  │   │  │   → Set cooldown, clear state for this tier                 │   │ │  │
│  │   │  └─────────────────────────────────────────────────────────────┘   │ │  │
│  │   │                                                                     │ │  │
│  │   └─────────────────────────────────────────────────────────────────────┘   │  │
│  │                                                                             │  │
│  └───────────────────────────────────┬─────────────────────────────────────────┘  │
│                                      │                                          │
│                                      ▼ result.detected = true                    │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                             ClipEvent                                      │  │
│  │                                                                             │  │
│  │   timestamp:          2026-03-01T10:21:06Z                                 │  │
│  │   creator_name:       "tfue"                                               │  │
│  │   tier:               "high"                                               │  │
│  │   matched_word:       "POG"                                                │  │
│  │   burst_count:        4                                                    │  │
│  │   spike_z_score:       2                                                    │  │
│  │   users_matched:       3                                                    │  │
│  │   spike_intensity:   0.24                                                  │  │
│  │   sample_messages:   "user1: POG | user2: POGGERS | user3: POG!!"         │  │
│  │                                                                             │  │
│  └─────────────────────────────────────┬───────────────────────────────────────┘  │
│                                        │                                        │
└────────────────────────────────────────┼────────────────────────────────────────────┘
                                         │
                                         ▼
                    ┌──────────────────────────────────────┐
                    │            CSV OUTPUT                 │
                    │                                      │
                    │  .ctic/outputs/tfue/high/            │
                    │  matches-high-20260301-101816.csv   │
                    │                                      │
                    │  timestamp,matched_word,sentiment,   │
                    │  burst_count,spike_z_score,          │
                    │  users_matched,spike_intensity,      │
                    │  config_id,sample_messages           │
                    │                                      │
                    └──────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────┐
│                            CONFIG LAYER (JSON)                                   │
│                                                                                 │
│  .ctic/creators/tfue.json          .ctic/profiles/balanced.json                │
│  ┌─────────────────────────┐       ┌─────────────────────────────────┐          │
│  │ name: "tfue"            │       │ profile_name: "balanced"        │          │
│  │ channel: "tfue"         │       │                                 │          │
│  │ profile: "balanced"  ───┼──────►│ tiers:                          │          │
│  │ enabled_tiers:          │       │   high:                         │          │
│  │   - high                │       │     burst_threshold: 3          │          │
│  │   - medium              │       │     min_word_length: 3          │          │
│  │   - easy                │       │     cooldown_seconds: 60        │          │
│  └─────────────────────────┘       │     require_unique_users: 2    │          │
│                                    │   medium: {...}                │          │
│                                    │   easy: {...}                   │          │
│                                    └─────────────────────────────────┘          │
│                                                                                 │
│  src/core/config.cpp (hardcoded word lists)                                   │
│  ┌─────────────────────────────────────────────────────────────────┐          │
│  │ high: [POG, POGGERS, INSANE, CLUTCH, GOATED, DIFF, ...]        │          │
│  │ high-negative: [L, LOST, BOT, TRASH, CRINGE, OMEGALUL, ...]    │          │
│  │ medium: [W, GG, GGS, NICE, SHEESH, DAMN, KEKW, ...]             │          │
│  │ easy: [lol, wow, bruh, true, real, fr, no cap, ...]            │          │
│  └─────────────────────────────────────────────────────────────────┘          │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────┐
│                              KEY ALGORITHMS                                      │
│                                                                                 │
│  1. LEVENSHTEIN DISTANCE (Wagner & Fischer, 1974)                              │
│     ┌─────────────────────────────────────────────────────────────────────┐    │
│     │  dp[i][j] = min(dp[i-1][j]+1, dp[i][j-1]+1, dp[i-1][j-1]+cost)     │    │
│     │                                                                     │    │
│     │  calculate_similarity = 1 - (distance / max_length)                 │    │
│     │                                                                     │    │
│     │  Example: "POG" vs "POGGERS" → sim ≈ 0.43                          │    │
│     │           "POG" vs "POG"     → sim = 1.0                            │    │
│     └─────────────────────────────────────────────────────────────────────┘    │
│                                                                                 │
│  2. Z-SCORE SPIKE DETECTION (Lucas & Saccucci, 1990)                           │
│     ┌─────────────────────────────────────────────────────────────────────┐    │
│     │  z = (x - mean) / stddev                                             │    │
│     │                                                                     │    │
│     │  Welford's online variance:                                         │    │
│     │    mean = mean + (x - mean) / n                                     │    │
│     │    M2 = M2 + (x - mean_old) * (x - mean)                            │    │
│     │    variance = M2 / (n - 1)                                          │    │
│     │                                                                     │    │
│     │  Spike if: z > 3.0 (3 standard deviations)                         │    │
│     └─────────────────────────────────────────────────────────────────────┘    │
│                                                                                 │
│  3. COLLAPSE REPEATED CHARS (custom)                                           │
│     ┌─────────────────────────────────────────────────────────────────────┐    │
│     │  "WWWW" → "W"                                                        │    │
│     │  "AAAAAA" → "A"                                                      │    │
│     │  "LOOOOOL" → "LOL"                                                    │    │
│     └─────────────────────────────────────────────────────────────────────┘    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```
