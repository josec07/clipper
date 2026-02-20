# Algorithm Index

*Quick reference guide for finding algorithm implementations and their academic sources*

## How to Use This Index

1. **Find the algorithm** you're looking for in the list below
2. **Check the file location** to see where it's implemented
3. **Look for the comment** in the code (marked with `// Ref:`)
4. **Search docs/REFERENCES.md** for the section name to get full citations

---

## Current Algorithms (Implemented)

### Spike Detection & Anomaly Detection

| Algorithm | Implementation | References Section |
|-----------|----------------|-------------------|
| **Z-Score Anomaly Detection** | `src/spike_detector.cpp:25` | "Z-Score Anomaly Detection" |
| **Z-Score Normalization** | `src/spike_detector.cpp:39` | "Z-Score Anomaly Detection" |
| **Welford's Online Variance** | `src/spike_detector.cpp:62` | "Online Variance" |
| **Running Mean** | `src/spike_detector.cpp:73` | "Online Variance" |
| **Welford's StdDev** | `src/spike_detector.cpp:84` | "Online Variance" |
| **Sliding Window** | `src/spike_detector.cpp:13` | "Moving Window Statistics" |
| **Moving Baseline** | `src/spike_detector.cpp:56` | "Moving Window Statistics" |

### String Matching & Fuzzy Search

| Algorithm | Implementation | References Section |
|-----------|----------------|-------------------|
| **Levenshtein Distance** | `src/keyword_matcher.cpp:63` | "Levenshtein Distance" |
| **Fuzzy Matching** | `src/keyword_matcher.cpp:19` | "Fuzzy String Matching" |
| **Text Normalization** | `src/keyword_matcher.cpp:96` | "Text Normalization" |
| **Similarity Scoring** | `src/keyword_matcher.cpp:48` | "Fuzzy String Matching" |

### Chat Buffer & Time Windows

| Algorithm | Implementation | References Section |
|-----------|----------------|-------------------|
| **Circular Buffer** | `src/chat_buffer.cpp:11` | "Circular Buffer" |
| **Time Window Queries** | `src/chat_buffer.cpp:19` | "Circular Buffer" |
| **Message Rate Calculation** | `src/chat_buffer.cpp:39` | "Circular Buffer" |
| **Auto-Cleanup** | `src/chat_buffer.cpp:49` | "Circular Buffer" |

### Scoring & Detection Logic

| Algorithm | Implementation | References Section |
|-----------|----------------|-------------------|
| **Weighted Scoring** | `src/scoring_engine.cpp:11` | "Weighted Scoring" |
| **Threshold Detection** | `src/scoring_engine.cpp:25` | "Weighted Scoring" |
| **Clip Event Generation** | `src/clip_detector.cpp:103` | "Cooldown Mechanism" |
| **Cooldown Timer** | `src/clip_detector.cpp:91` | "Cooldown Mechanism" |

---

## Alternative Algorithms (For Future Reference)

Located in: `docs/REFERENCES.md` → "Extra Resources" section

| Algorithm | Use Case | Complexity | References Section |
|-----------|----------|------------|-------------------|
| **Grubbs' Test** | Statistical rigor with p-values | O(n) | "Extra Resources → Grubbs' Test" |
| **CUSUM** | Small persistent shifts | O(1) | "Extra Resources → CUSUM" |
| **Generalized ESD** | Multiple consecutive peaks | O(kn) | "Extra Resources → Generalized ESD" |
| **Twitter S-H-ESD** | Production-grade with seasonality | O(n) | "Extra Resources → S-H-ESD" |
| **Page-Hinkley Test** | Streaming drift detection | O(1) | "Extra Resources → Page-Hinkley" |
| **Adaptive Thresholds** | Time-varying sensitivity | O(1) | "Extra Resources → Adaptive Thresholds" |
| **Multi-Scale Detection** | Multiple window sizes | O(k) | "Extra Resources → Multi-Scale" |
| **Burst Detection** | Sustained hype periods | O(n) | "Extra Resources → Burst Detection" |

---

## Quick Search Guide

### By Algorithm Name
- **Z-Score** → `src/spike_detector.cpp:25,39` + refs "Z-Score Anomaly Detection"
- **Welford** → `src/spike_detector.cpp:62,73,84` + refs "Online Variance"
- **Levenshtein** → `src/keyword_matcher.cpp:63` + refs "Levenshtein Distance"
- **Moving Window** → `src/spike_detector.cpp:13,56` + refs "Moving Window Statistics"

### By Use Case
- **Detecting spikes** → Z-Score (`src/spike_detector.cpp:24`)
- **Stable variance** → Welford (`src/spike_detector.cpp:83`)
- **Fuzzy matching** → Levenshtein + Normalize (`src/keyword_matcher.cpp:54,85`)
- **Time windows** → Circular Buffer (`src/chat_buffer.cpp:16`)
- **Preventing spam** → Cooldown (`src/clip_detector.cpp:82`)

### By File
- **spike_detector.cpp** → Z-Score, Welford, Moving Window
- **keyword_matcher.cpp** → Levenshtein, Fuzzy Matching, Normalization
- **chat_buffer.cpp** → Circular Buffer, Time Windows
- **scoring_engine.cpp** → Weighted Scoring
- **clip_detector.cpp** → Event Detection, Cooldown

---

## Academic Paper Index

Find papers by the algorithm they describe:

### Core Papers
- **Wagner & Fischer (1974)** → Levenshtein Distance implementation
- **Navarro (2001)** → Fuzzy/approximate string matching survey
- **Welford (1962)** → Online variance algorithm
- **Lucas & Saccucci (1990)** → Z-Score and EWMA moving statistics

### Alternative/Extension Papers
- **Grubbs (1969)** → Formal outlier detection with hypothesis testing
- **Page (1954)** → CUSUM and Page-Hinkley tests
- **Rosner (1983)** → Generalized ESD for multiple outliers
- **Vallis et al. (2014)** → Twitter's S-H-ESD for production use

---

## File Structure for Reference

```
docs/
├── INDEX.md              <- You are here
├── REFERENCES.md         <- Full citations and extra resources
├── literature/           <- BibTeX files
│   ├── levenshtein.bib
│   ├── ringer_2020_twitchchat.bib
│   ├── ringer_2022_phd.bib
│   └── barbieri_2017_emotes.bib
└── ARCHITECTURE.md       <- System design and data flow

src/
├── spike_detector.cpp    <- Z-Score, Welford, Moving Window
├── keyword_matcher.cpp   <- Levenshtein, Fuzzy Matching
├── chat_buffer.cpp       <- Circular Buffer
├── scoring_engine.cpp    <- Weighted Scoring
└── clip_detector.cpp     <- Event Detection
```

---

## Common Lookup Patterns

**"I want to understand spike detection"**
1. Look up: Z-Score → `src/spike_detector.cpp:24`
2. See comment: `// Ref: Z-Score anomaly detection`
3. Search REFERENCES.md: "Z-Score Anomaly Detection"
4. Find: Lucas & Saccucci (1990) citation

**"I want to improve variance calculation"**
1. Look up: Welford → `src/spike_detector.cpp:83`
2. See comment: `// Ref: Welford's variance formula`
3. Search REFERENCES.md: "Online Variance"
4. Find: Welford (1962), Knuth (1997) citations
5. Extra: Check "Extra Resources" for alternatives

**"I want production-grade anomaly detection"**
1. Search REFERENCES.md: "Extra Resources → Twitter S-H-ESD"
2. Find: Vallis et al. (2014) + implementation notes
3. Consider: Seasonal decomposition, robust statistics

---

*Last updated: 2026-02-14*
*Index maintained alongside code*
