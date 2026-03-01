# CTIC - Chat Twitch Intelligence CLI

Detect clip-worthy moments from Twitch chat in real-time.

**Keywords:** twitch clip detection, twitch highlights, chat analysis, streaming tools, clip finder, twitch moments, chat signal processing

## Install

```bash
git clone https://github.com/your-repo/ctic
cd ctic
make
```

## Usage

```bash
./ctic add tfue https://twitch.tv/tfue
./ctic run
```

Results saved to `.ctic/outputs/tfue/`

## Features

- **Zero-setup IRC** - Anonymous Twitch chat connection (no OAuth)
- **Multi-creator monitoring** - Watch multiple channels simultaneously
- **Configurable detection** - Tune thresholds via JSON profiles
- **CSV output** - Full metadata for analysis

## How It Works

CTIC detects when chat "explodes" with similar reactions simultaneously using statistical burst detection and fuzzy word matching.

No ML. No GPU. Just algorithms.

## Docs

- [tutorial.md](docs/tutorial.md) - How to use and tune
- [references.md](docs/references.md) - Algorithm citations

## Config

Edit `.ctic/profiles/balanced.json` to adjust detection sensitivity.

## License

GPL v3
