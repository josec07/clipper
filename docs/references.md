# References

Academic citations for algorithms used in CTIC.

## Core Algorithms

### Levenshtein Distance

```bibtex
@article{wagner_fischer_1974,
  author = {Wagner, Robert A. and Fischer, Michael J.},
  title = {The String-to-String Correction Problem},
  journal = {Journal of the ACM},
  volume = {21},
  number = {1},
  pages = {168--173},
  year = {1974}
}
```

### Fuzzy String Matching

```bibtex
@article{navarro_2001_approximate,
  author = {Navarro, Gonzalo},
  title = {A Guided Tour to Approximate String Matching},
  journal = {ACM Computing Surveys},
  volume = {33},
  number = {1},
  pages = {31--88},
  year = {2001}
}
```

### Z-Score Anomaly Detection

```bibtex
@article{lucas_saccucci_1990_ewma,
  author = {Lucas, James M. and Saccucci, Michael S.},
  title = {Exponentially Weighted Moving Average Control Schemes},
  journal = {Technometrics},
  volume = {32},
  number = {1},
  pages = {1--12},
  year = {1990}
}
```

### Welford's Algorithm (Online Variance)

```bibtex
@article{welford_1962_note,
  author = {Welford, B. P.},
  title = {Note on a Method for Calculating Corrected Sums of Squares and Products},
  journal = {Technometrics},
  volume = {4},
  number = {3},
  pages = {419--420},
  year = {1962}
}
```

## Twitch Chat Research

```bibtex
@inproceedings{ringer_2020_twitchchat,
  author = {Ringer, Charles and Nicolaou, Mihalis A. and Walker, James Alfred},
  title = {TwitchChat: A Dataset for Exploring Livestream Chat},
  booktitle = {AAAI Conference on AI and Interactive Digital Entertainment},
  pages = {100--106},
  year = {2020}
}

@phdthesis{ringer_2022_phd,
  author = {Ringer, Charles},
  title = {Multi-Modal Livestream Highlight Detection},
  school = {University of York},
  year = {2022}
}
```

## Alternative Algorithms

| Algorithm | Paper | Use Case |
|-----------|-------|----------|
| Grubbs' Test | Grubbs (1969) | Statistical outlier detection |
| CUSUM | Page (1954) | Small persistent shifts |
| Generalized ESD | Rosner (1983) | Multiple consecutive anomalies |
| S-H-ESD | Vallis et al. (2014) | Production-grade with seasonality |
| Page-Hinkley | Page (1954) | Online streaming detection |
