# The_boy-44
Pmll
# boy-44

**boy-44** is a special assistant that acts like a *“boy”* you can talk to about sports numbers, players, and betting lines.  
It remembers what you tell it, logs signals like the number **44**, and helps you ask questions like *“who is wearing 44 today?”* or *“what do the odds mean for this game?”*.

At its heart, boy-44 is a **decoder of the book** (sportsbook odds).  
It doesn’t just repeat the betting lines — it breaks them down, calculates the *real probability*, and compares that to what the market is offering.

⚠️ **Disclaimer:** This project is for research and learning. It does not guarantee winning bets. Always follow your local laws and never gamble more than you can afford to lose.

---

## Why it exists

The idea started with wanting to “talk to it like a boy.”  
You feed it *signals* (like the number `44` you see repeatedly), and it keeps track of them.  
Then you can ask questions naturally, and it ties those signals to real-world data like players, rosters, and betting odds.

Instead of keeping clunky Excel sheets, boy-44 gives you a **conversational memory** with structure and recall.

---

## How it works (high level)

1. **Signals**  
   - You can enter numbers or patterns (like `44` or “signal 12”).  
   - These get logged to a CSV file (`signals_log.csv`) with timestamps.

2. **Players and Rosters**  
   - boy-44 can answer: *“Which players are wearing number 44 today?”*  
   - It looks into roster data (mock data now, but can be swapped for live feeds).

3. **Odds Decoder**  
   - You can feed it betting odds like “BOS -135 moneyline.”  
   - boy-44 converts that into a probability, removes the sportsbook margin (the “vig”), and compares it to your own model estimate.  
   - It tells you the *edge* (your advantage or disadvantage).

4. **Memory Loop (PMLL)**  
   - Every time you log a signal or decode odds, it remembers.  
   - You can recall past signals: *“What were my last 10 signals?”*  
   - Co-occurrence logs keep track of connections between signals and players.

---

## What’s inside

- **C core** → Very fast program for logging signals, querying rosters, and writing results to CSV.  
- **Python/LLM layer** → Lets you chat naturally, ask questions, and run the “decoder” logic.  
- **CSV logs** → All memory is saved in simple files you can open in Excel.  

---

## Example conversations

**Recording a signal**

```
You: 44
boy: Recorded signal 44 (seen 7 times, last today at 1:07am)
```

**Asking who wears 44**

```
You: who is wearing 44 today?
boy: NBA BOS — Robert Williams III (#44)
     NFL DET — Malcolm Rodriguez (#44)
```

**Decoding a line**

```
You: decode BOS @ NYK moneyline BOS -135
boy: Market says BOS should win 56.5% of the time (after removing vig)
     My model says BOS has 60% chance
     Edge = +3.5%
     Suggested Kelly stake = 1.2% of bankroll
     Rationale: BOS rested, NYK on back-to-back, injuries updated
```

---

## Why it’s useful

- **You can talk to it** instead of juggling spreadsheets.  
- **It remembers** your signals and queries over time.  
- **It decodes the book** into probabilities and edges.  
- **It’s explainable**: always shows the math, assumptions, and rationale.

---

## Technical details (for developers)

- Written in **C** for speed, packaged with **PPM**.  
- Logs data to CSVs for transparency and auditing.  
- Optional **LLM front-end** for chat-based interaction.  
- Modular adapters for rosters, odds feeds, fight data.  
- Versioned memory loop (**PMLL**) keeps track of belief updates.  

---

## State files (saved in `state/`)

- `signals_log.csv` — Every signal you record (number, time, source).  
- `beliefs.csv` — Counts and timestamps for each number.  
- `cooc_number_<N>.csv` — Players linked to a number when you query them.  
- `odds_snapshots.csv` — Snapshots of odds lines you decode.  

---

## Roadmap

- Add support for real-time roster and odds APIs.  
- Build web/Discord/Telegram chat interface.  
- Add “stats” mode (top numbers, weekly summaries).  
- Build boxing/matchup module.  

---

## Final note

boy-44 is like a digital sidekick.  
It’s not magic. It’s a **memory + math assistant** to help you keep track of signals, decode betting lines, and stay organized.
