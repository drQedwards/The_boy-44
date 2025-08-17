# boy-44 · polished README

<p align="center">
  <img alt="boy-44 logo" src="https://img.shields.io/badge/boy--44-PMLL%20signal%20agent-blue" />
  <a href="https://github.com/drQedwards/boy-44/actions"><img alt="build" src="https://img.shields.io/github/actions/workflow/status/drQedwards/boy-44/ci.yml?branch=main"></a>
  <a href="https://github.com/drQedwards/boy-44/releases"><img alt="semver" src="https://img.shields.io/github/v/release/drQedwards/boy-44"></a>
  <img alt="license" src="https://img.shields.io/badge/license-MIT-green" />
  <img alt="lang" src="https://img.shields.io/badge/core-C-informational" />
  <img alt="lang" src="https://img.shields.io/badge/agent-Python-informational" />
  <img alt="downloads" src="https://img.shields.io/github/downloads/drQedwards/boy-44/total">
</p>

**boy-44** is a conversational *PMLL* agent that logs symbolic signals (like `44`), links them to players/teams, and runs a **high‑fidelity sportsbook decoder** (probabilities, edge, CLV, fractional‑Kelly).  
It ships with a fast **C core** for ingestion/lookup and a **Python LLM layer** for natural chat.

> ⚠️ For research and education. No profit guarantees. Obey local laws and practice strict bankroll management.

---

## Install

### Option A — via PPM
```bash
# assuming PPM is installed on your system
ppm install boy-44
ppm run boy-44 demo     # quick smoke test
```

### Option B — from source
```bash
git clone https://github.com/drQedwards/boy-44.git
cd boy-44/core-c
make -j
../core-c/bin/pmll_agent --base ../state ingest 44
../core-c/bin/pmll_agent --base ../state who-wears 44 --active
../core-c/bin/pmll_agent --base ../state recall --last 10
```

State files (Excel‑friendly) appear in `state/`:
- `signals_log.csv` — ts, number, source
- `beliefs.csv` — count, last_ts per number
- `cooc_number_<N>.csv` — co‑occurrence log (players found for number N)

---

## Run the “boy” chat agent
```bash
# configure provider; defaults to local model names if you run it that way
cp .env.example .env
# edit PROVIDER=local|openai|vllm, MODEL_NAME=..., ODDS_API_KEY=...

cd agent
python boy44_agent.py --base ../state
```

Try these:
```
44
which players today are wearing number 44?
decode BOS @ NYK moneyline BOS -135
recall last 10
```

---

## Features

- **Signals → Memory:** record numbers/patterns; persistent CSV + versioned snapshots.  
- **Roster queries:** “who wears #X?” with pluggable feeds.  
- **Book decoder:** de‑vig odds, compute model p, edge, CLV, and fractional‑Kelly stake.  
- **Explainability:** always print assumptions and audit trail (timestamps + snapshot id).  
- **Fast core:** zero‑dependency C binary for logging and queries.

---

## Architecture

```
[C core: pmll_agent]  <-- O(1) CSV appends, roster filtering
        │
        ├── state/signals_log.csv
        ├── state/beliefs.csv
        └── state/cooc_number_<N>.csv
[Python LLM layer]
        ├── natural language routing (signal/query/decode/recall/stats)
        └── adapters: rosters, schedules, odds APIs
[PMLL]
        └── versioned belief updates + optional KG snapshots
```

---

## CLI reference

```
pmll_agent --base <dir> ingest <number>              # record a signal
pmll_agent --base <dir> who-wears <number> --active  # list current players
pmll_agent --base <dir> recall --last N              # print last N signals
```

---

## Configuration

Set in `.env` (or pass as flags to the agent):

- `PROVIDER` = `local` | `openai` | `vllm`  
- `MODEL_NAME` = model or endpoint name  
- `ODDS_API_KEY` = key for odds provider (optional)  
- `FEEDS_ROSTERS_CSV` = path to daily roster CSV (defaults to `core-c/data/rosters_mock.csv`)

---

## Contributing

1. Fork → feature branch → PR.  
2. Add/extend tests under `tests/`.  
3. Keep the README examples runnable (`make -j` then the three demo commands).

---

## License

MIT © Dr. Josef Kurk Edwards

---

### Badge snippets (copy/paste)

```md
![build](https://img.shields.io/github/actions/workflow/status/drQedwards/boy-44/ci.yml?branch=main)
![release](https://img.shields.io/github/v/release/drQedwards/boy-44)
![license](https://img.shields.io/badge/license-MIT-green)
![core](https://img.shields.io/badge/core-C-informational)
![agent](https://img.shields.io/badge/agent-Python-informational)
![downloads](https://img.shields.io/github/downloads/drQedwards/boy-44/total)
```
