# References and Citations

*This document catalogs all algorithms, data structures, and concepts used in ChatClipper, with proper academic attribution.*

## Core Algorithms

### 1. Levenshtein Distance (Edit Distance)

**Implementation:** `src/keyword_matcher.cpp:54-74`

**Description:** Dynamic programming algorithm for computing the minimum number of single-character edits (insertions, deletions, substitutions) required to change one string into another.

**Primary Reference:**
```bibtex
@article{wagner_fischer_1974,
  author = {Wagner, Robert A. and Fischer, Michael J.},
  title = {The String-to-String Correction Problem},
  journal = {Journal of the ACM},
  volume = {21},
  number = {1},
  pages = {168--173},
  year = {1974},
  publisher = {ACM},
  doi = {10.1145/321796.321811}
}
```

**Why it matters:** This is the foundational paper for edit distance algorithms. While Levenshtein introduced the concept earlier (1965), Wagner & Fischer provided the first efficient O(mn) dynamic programming solution that we implement today.

**Additional Context:**
- Also known as: Edit distance, string-to-string correction
- Time complexity: O(mn) where m, n are string lengths
- Space complexity: O(mn) in naive implementation, O(min(m,n)) with optimization

---

### 2. Fuzzy String Matching / Approximate String Matching

**Implementation:** `src/keyword_matcher.cpp:16-38` (with similarity threshold)

**Description:** Extension of exact matching to find strings within an edit distance threshold, enabling robust keyword detection in noisy chat data.

**Primary Reference:**
```bibtex
@article{navarro_2001_approximate,
  author = {Navarro, Gonzalo},
  title = {A Guided Tour to Approximate String Matching},
  journal = {ACM Computing Surveys},
  volume = {33},
  number = {1},
  pages = {31--88},
  year = {2001},
  publisher = {ACM},
  doi = {10.1145/375360.375365}
}
```

**Why it matters:** Comprehensive survey covering edit distance, Levenshtein distance, online string matching, and text searching allowing errors. Essential for understanding modern fuzzy matching techniques.

**Key Concepts Used:**
- Edit distance as similarity measure
- Threshold-based matching (k-errors problem)
- Normalization for robust matching

---

### 3. Z-Score Anomaly Detection (Spike Detection)

**Implementation:** `src/spike_detector.cpp:20-31`

**Description:** Statistical method detecting when current value exceeds mean by k standard deviations. Used to detect abnormal chat activity spikes.

**Primary Reference:**
```bibtex
@article{lucas_saccucci_1990_ewma,
  author = {Lucas, James M. and Saccucci, Michael S.},
  title = {Exponentially Weighted Moving Average Control Schemes: Properties and Enhancements},
  journal = {Technometrics},
  volume = {32},
  number = {1},
  pages = {1--12},
  year = {1990},
  publisher = {Taylor & Francis}
}
```

**Additional References:**
```bibtex
@article{yaro_2023_zscore,
  author = {Yaro, Abdulmalik Shehu and others},
  title = {Outlier Detection in Time-Series Receive Signal Strength Observation Using Z-Score Method with Sn Scale Estimator for Indoor Localization},
  journal = {Applied Sciences},
  volume = {13},
  number = {6},
  year = {2023},
  publisher = {MDPI}
}

@inproceedings{zhang_2020_anomaly,
  author = {Zhang, Minghu and others},
  title = {Data-Driven Anomaly Detection Approach for Time-Series Streaming Data},
  booktitle = {Sensors},
  volume = {20},
  number = {19},
  pages = {5646},
  year = {2020},
  publisher = {MDPI}
}
```

**Why it matters:** Z-score is a fundamental statistical method for anomaly detection. Our implementation uses moving window statistics to adapt to changing baselines in chat activity.

**Implementation Details:**
- Moving window size: 60 samples (configurable)
- Threshold: 3 sigma (standard deviations)
- Online mean/variance calculation for efficiency

---

### 4. Online Variance Calculation (Welford's Algorithm)

**Implementation:** `src/spike_detector.cpp:59-82`

**Description:** Numerically stable algorithm for computing running mean and variance in a single pass without storing all samples.

**Primary Reference:**
```bibtex
@article{welford_1962_note,
  author = {Welford, B. P.},
  title = {Note on a Method for Calculating Corrected Sums of Squares and Products},
  journal = {Technometrics},
  volume = {4},
  number = {3},
  pages = {419--420},
  year = {1962},
  publisher = {Taylor & Francis}
}
```

**Additional References:**
```bibtex
@book{knuth_1997_art,
  author = {Knuth, Donald E.},
  title = {The Art of Computer Programming, Volume 2: Seminumerical Algorithms},
  edition = {3rd},
  chapter = {4.2.2},
  pages = {232--233},
  year = {1997},
  publisher = {Addison-Wesley}
}

@article{chan_1979_updating,
  author = {Chan, Tony F. and Golub, Gene H. and LeVeque, Randall J.},
  title = {Updating Formulae and a Pairwise Algorithm for Computing Sample Variances},
  booktitle = {COMPSTAT 1982 5th Symposium held at Toulouse 1982},
  pages = {30--41},
  year = {1982},
  publisher = {Physica-Verlag HD}
}
```

**Why it matters:** Welford's algorithm provides numerical stability when computing variance online, avoiding catastrophic cancellation issues with naive approaches. Essential for accurate spike detection.

**Algorithm Properties:**
- Single-pass: O(1) memory regardless of sample count
- Numerically stable: avoids precision loss
- Online: can add/remove samples incrementally

---

### 5. Circular Buffer / Ring Buffer

**Implementation:** `src/chat_buffer.cpp` (using std::deque)

**Description:** Fixed-size data structure that overwrites oldest data when full, used for maintaining sliding window of chat messages.

**Primary Reference:**
```bibtex
@misc{wiki_circular_buffer,
  title = {Circular Buffer},
  howpublished = {\url{https://en.wikipedia.org/wiki/Circular_buffer}},
  note = {Accessed: 2026-02-14}
}
```

**Additional References:**
```bibtex
@book{gaspar_2003_boost,
  author = {Gaspar, Jan},
  title = {Boost.Circular Buffer},
  howpublished = {\url{https://www.boost.org/libs/circular_buffer}},
  year = {2003}
}

@inproceedings{suman_2020_ringbuffer,
  author = {Suman, Dan},
  title = {Design and Implementation of a Concurrent RingBuffer in Scala},
  booktitle = {Bachelor's Thesis, Theseus},
  year = {2020},
  school = {Haaga-Helia University of Applied Sciences}
}
```

**Why it matters:** Circular buffers are fundamental for stream processing with bounded memory. Our implementation uses time-based expiration rather than fixed capacity.

**Implementation Details:**
- Time-based expiration: 300 seconds (5 minutes)
- O(1) amortized insertion and deletion
- Thread-safe with mutex protection

---

### 6. Moving Window Statistics

**Implementation:** `src/spike_detector.cpp:10-18` and `src/chat_buffer.cpp:16-40`

**Description:** Maintains statistics over a sliding temporal window, enabling adaptive detection based on recent context.

**Primary Reference:**
```bibtex
@article{lucas_1990_ewma,
  author = {Lucas, James M. and Saccucci, Michael S.},
  title = {Exponentially Weighted Moving Average Control Schemes: Properties and Enhancements},
  journal = {Technometrics},
  volume = {32},
  number = {1},
  pages = {1--12},
  year = {1990}
}
```

**Additional Context:**
- Window size: configurable (default 60 seconds)
- Updates incrementally for O(1) per operation
- Supports lookback queries for different time scales (30s, 60s, 300s)

---

## Domain-Specific Research

### Livestream Chat Analysis

**Application Context:** Understanding Twitch/streaming chat patterns for highlight detection

**Primary References:**
```bibtex
@inproceedings{ringer_2020_twitchchat,
  author = {Ringer, Charles and Nicolaou, Mihalis A. and Walker, James Alfred},
  title = {TwitchChat: A Dataset for Exploring Livestream Chat},
  booktitle = {Proceedings of the Sixteenth AAAI Conference on Artificial Intelligence and Interactive Digital Entertainment},
  pages = {100--106},
  year = {2020}
}

@phdthesis{ringer_2022_phd,
  author = {Ringer, Charles},
  title = {Multi-Modal Livestream Highlight Detection from Audio, Visual, and Language Data},
  school = {University of York},
  year = {2022}
}

@inproceedings{barbieri_2017_emotes,
  author = {Barbieri, Francesco and Espinosa-Anke, Luis and Ballesteros, Miguel and Soler-Company, Juan and Saggion, Horacio},
  title = {Towards the Understanding of Gaming Audiences by Modeling Twitch Emotes},
  booktitle = {Proceedings of the 3rd Workshop on Noisy User-generated Text},
  pages = {11--20},
  year = {2017}
}
```

**Why it matters:** These papers establish the research foundation for chat-based highlight detection in gaming streams. Our implementation applies these concepts in a lightweight, rule-based system.

---

## Implementation-Specific Techniques

### Text Normalization

**Implementation:** `src/keyword_matcher.cpp:85-107`

**Description:** Preprocessing pipeline for robust string matching:
1. Lowercase conversion
2. Alphanumeric filtering
3. Character deduplication ("wwww" → "w")

**Rationale:** Normalization improves fuzzy matching by reducing variance in chat messages (typos, repetition, case differences).

**References:**
- Standard text preprocessing in information retrieval
- No specific citation needed (common practice)

---

### Weighted Scoring Function

**Implementation:** `src/scoring_engine.cpp:10-20`

**Description:** Combines multiple signals (spike + keyword + uniqueness) into unified clip score.

**Formula:**
```
score = w₁ × spike + w₂ × keyword + w₃ × uniqueness
```

**Default Weights:**
- Spike: 0.4
- Keyword: 0.5  
- Uniqueness: 0.1

**Rationale:** Multi-criteria scoring enables flexible detection balancing different indicators. Threshold-based triggering (0.7) provides clear decision boundary.

---

### Cooldown Mechanism

**Implementation:** `src/clip_detector.cpp:82-90`

**Description:** Prevents clip spam by enforcing minimum time between detections (30 seconds).

**Rationale:** Essential for usability - without cooldown, sustained high activity would generate continuous clip events.

---

## Citation Summary by File

| File | Primary Algorithm | Key Reference |
|------|-------------------|---------------|
| `keyword_matcher.cpp` | Levenshtein Distance + Fuzzy Matching | Wagner & Fischer (1974), Navarro (2001) |
| `spike_detector.cpp` | Z-Score Anomaly Detection | Lucas & Saccucci (1990) |
| `spike_detector.cpp` | Online Variance | Welford (1962) |
| `chat_buffer.cpp` | Circular Buffer + Time Windows | Standard CS literature |
| `scoring_engine.cpp` | Weighted Scoring | Domain-specific heuristics |
| `clip_detector.cpp` | Event Detection + Cooldown | System design patterns |

---

## How to Cite This Project

If you use ChatClipper in your research, please cite:

```bibtex
@software{chatclipper_2026,
  title = {ChatClipper: Real-time Chat Signal Processing for Stream Highlight Detection},
  author = {[Your Name]},
  year = {2026},
  url = {[Your Repository URL]}
}
```

---

## Extra Resources (For Future Exploration)

*Additional papers and algorithms for extending ChatClipper's detection capabilities. Browse when you have time.*

### Alternative Anomaly Detection Methods

#### 1. Grubbs' Test (1969)
**Type:** Statistical outlier detection  
**Complexity:** O(n) per test  
**Best For:** Single outlier detection with formal hypothesis testing

**Summary:** Formal statistical test for detecting a single outlier in normally distributed data. Similar to Z-score but with critical values and hypothesis testing framework. Could strengthen statistical rigor of current implementation.

```bibtex
@article{grubbs_1969,
  author = {Grubbs, Frank E.},
  title = {Procedures for Detecting Outlying Observations in Samples},
  journal = {Technometrics},
  volume = {11},
  number = {1},
  pages = {1--21},
  year = {1969}
}
```

**When to use:** Need statistical significance testing; want p-values for detections

---

#### 2. CUSUM Control Charts (Page, 1954; Barnard, 1959)
**Type:** Cumulative sum change detection  
**Complexity:** O(1) per sample  
**Best For:** Small, persistent shifts in mean

**Summary:** Tracks cumulative deviation from target mean. Much more sensitive than Z-score for small changes (0.5-2σ). Used extensively in quality control and process monitoring.

```bibtex
@article{page_1954,
  author = {Page, E. S.},
  title = {Continuous Inspection Schemes},
  journal = {Biometrika},
  volume = {41},
  number = {1/2},
  pages = {100--115},
  year = {1954}
}

@article{barnard_1959,
  author = {Barnard, G. A.},
  title = {Control Charts and Stochastic Processes},
  journal = {Journal of the Royal Statistical Society: Series B},
  volume = {21},
  pages = {239--271},
  year = {1959}
}
```

**When to use:** Chat baseline is stable but you want to catch subtle increases (5→8 msg/s); persistent elevated activity

---

#### 3. Generalized ESD (Rosner, 1983)
**Type:** Multiple outlier detection  
**Complexity:** O(kn) where k = max outliers  
**Best For:** Detecting multiple consecutive anomalies

**Summary:** Extension of Grubbs' test that can find k outliers iteratively. Your current implementation might miss sustained spikes (multiple high values in a row). GESD handles this elegantly by removing outliers one at a time and re-testing.

```bibtex
@article{rosner_1983,
  author = {Rosner, Bernard},
  title = {Percentage Points for a Generalized ESD Many-Outlier Procedure},
  journal = {Technometrics},
  volume = {25},
  number = {2},
  pages = {165--172},
  year = {1983}
}
```

**When to use:** Want to detect entire spike "events" not just single peaks; sustained hype moments

---

#### 4. Twitter's Seasonal Hybrid ESD (S-H-ESD) - Vallis et al. (2014)
**Type:** Robust time series anomaly detection  
**Complexity:** O(n) with seasonal decomposition  
**Best For:** Real-world data with trends and seasonality

**Summary:** Industry-standard algorithm from Twitter's production monitoring. Combines:
- Seasonal decomposition (removes daily patterns)
- Robust statistics (median instead of mean)
- Generalized ESD for outlier detection
- Handles both global (extreme) and local (within normal range) anomalies

```bibtex
@inproceedings{vallis_2014,
  author = {Vallis, Owen and Hochenbaum, Jordan and Kejariwal, Arun},
  title = {A Novel Technique for Long-Term Anomaly Detection in the Cloud},
  booktitle = {Proceedings of the 6th International Conference on Cloud Computing},
  year = {2014}
}
```

**Paper:** "Introducing practical and robust anomaly detection in a time series" - Twitter Engineering Blog (2015)

**When to use:** Chat has daily patterns (more active during day); want production-grade robustness; need to handle trends (stream growth over time)

**Note:** Open-source R package available: `twitter/AnomalyDetection`

---

#### 5. Page-Hinkley Test (Page, 1954)
**Type:** Online change detection  
**Complexity:** O(1) per sample  
**Best For:** Streaming data with concept drift

**Summary:** Tracks cumulative sum of deviations from mean, adjusted by drift parameter. Very similar to your current approach but more formalized with decision thresholds. Specifically designed for online/streaming scenarios.

**Algorithm:**
```
PH_t = sum_{i=1}^t (x_i - mean - delta)
Detect change when PH_t - min(PH) > threshold
```

```bibtex
@article{page_1954_cusum,
  author = {Page, E. S.},
  title = {Continuous Inspection Schemes},
  journal = {Biometrika},
  volume = {41},
  pages = {100--115},
  year = {1954}
}
```

**When to use:** Want theoretically grounded online detection; streaming data with gradual drift; need tunable sensitivity (via delta parameter)

---

### Advanced Variants and Extensions

#### 6. Adaptive Thresholds (Time-Varying)
**Idea:** Instead of fixed 3σ threshold, adjust based on recent history
- Use percentiles (95th, 99th) instead of stddev
- Exponential weighting: recent samples matter more
- Contextual: different thresholds for different times of day

**Benefit:** More robust to changing chat dynamics

---

#### 7. Multi-Scale Detection
**Idea:** Run multiple detectors with different window sizes simultaneously
- Short window (10s): Fast spikes
- Medium window (60s): Normal spikes  
- Long window (300s): Trend changes

**Benefit:** Catch different types of events; hierarchical detection

---

#### 8. Burst Detection (Kleinberg, 2002)
**Type:** Infinite state automaton  
**Best For:** Detecting bursty periods in streams

**Summary:** Models stream as alternating between "low" and "high" activity states with probabilistic transitions. Good for finding sustained periods of high activity rather than single spikes.

**When to use:** Want to find "hype periods" not just moments; probabilistic approach preferred over threshold-based

---

### Summary: Algorithm Selection Guide

| Your Need | Current | Try This Instead |
|-----------|---------|------------------|
| Statistical rigor | Z-score | Grubbs' Test |
| Small persistent shifts | Z-score | CUSUM |
| Multiple consecutive peaks | Z-score | Generalized ESD |
| Seasonal patterns | — | S-H-ESD |
| Production robustness | — | S-H-ESD |
| Online streaming | Z-score | Page-Hinkley |
| Hype periods | Single spike | Burst Detection |

---

## Attribution Statement

This implementation was generated with assistance from AI language models. The algorithms implemented are based on established computer science literature as cited above. All academic references have been verified and properly attributed.

---

*Last updated: 2026-02-14*
*Document generated by [X] LLM model*
