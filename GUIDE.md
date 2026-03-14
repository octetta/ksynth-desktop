# ksynth-desktop — guide

## overview

ksynth-desktop is a native live-coding environment for the ksynth synthesis language. It runs on Linux, macOS, and Windows with no browser required.

The interface has four functional areas:

- **slot strip** — two rows of 8 across the top, slots `0`–`F`. Holds audio buffers ready to trigger.
- **notebook** — the left pane. An append-only log of every script you have run, with waveforms and bank buttons.
- **editor** — the right pane. Where you write ksynth code, run it, and inspect values in the REPL strip.
- **pad window** — a floating 4×4 trigger grid, opened with the `pads` button.

The divider between notebook and editor is draggable. The two panes resize independently.

---

## tutorial

This section walks from zero to a playable melodic instrument. No prior ksynth knowledge required.

### step 1 — your first sound

Type the following into the editor and press `Ctrl+J`:

```
N: 4410
T: !N
W: w s +\(N#(440*(6.28318%44100)))
```

You should hear a 100ms sine wave at 440 Hz. A cell appears in the notebook showing the waveform. What each line does:

- `N: 4410` — 4410 samples = 100ms at 44100 Hz
- `T: !N` — time index: a ramp from 0 to N−1
- `W: ...` — the output buffer. `w s` writes samples, `+\(N#...)` is a running phase accumulator, `440*(6.28318%44100)` is the per-sample phase increment for 440 Hz

`W` is always the output. Every script must set it.

### step 2 — a bell sound

FM synthesis produces bell tones naturally. A carrier oscillator is phase-modulated by a modulator: the modulation index (how much the modulator deflects the carrier's phase) starts high — bright and complex at the attack — then decays fast, leaving a cleaner tone. Amplitude decays more slowly.

```
N: 88200
T: !N
A: e(T*(0-3%N))
I: 3.5*e(T*(0-40%N))
C: 440*(6.28318%44100)
M: 440*(6.28318%44100)
P: +\(N#C)
Q: +\(N#M)
W: w A*(s P+(I*s Q))
```

- `A` — amplitude envelope, decays over ~2 seconds
- `I` — modulation index: starts at 3.5, decays to near zero in ~50ms. The fast transient is the clang of the strike
- `C`, `M` — carrier and modulator phase increments (same frequency = ratio 1:1)
- `P`, `Q` — phase accumulators
- `W` — right-associative parse gives `A × sin(P + I×sin(Q))`, which is the FM formula

**Experiment:** change `40` (index decay) to `80` for a drier attack, or `20` for a longer metallic clang. Change `3.5` (peak index) to `5` for something harsher. Change the modulator frequency from `440` to `616` (ratio 1.4:1) for a tubular bell quality.

### step 3 — bank it to a slot

After running the bell script, find its cell in the notebook. Click the `→0` button to bank the audio buffer to slot 0. The slot card at the top updates, showing the waveform.

Click the slot card to play it back. Right-click it for the context menu.

### step 4 — play it melodically

The buffer you synthesised is a fixed recording at one pitch. The pad panel plays it back at different rates to hit different pitches — no re-synthesis needed.

1. With the bell in slot 0, click the `pads` button to open the pad window.
2. Click **melodic** at the top of the panel.
3. All 16 pads now play slot 0 at different semitone offsets:

```
row 0 (top)    −7   −6   −5   −4   st
row 1          −3   −2   −1    0   st
row 2          +1   +2   +3   +4   st
row 3 (btm)   +5   +6   +7   +8   st
```

The playback rate for each pad is `2^(semitones/12)`. At 0 semitones the bell plays at its original pitch. At +12 it plays an octave up. Click the pads — you are playing the bell at different pitches.

**Tip:** right-click a slot card and choose **Set tuning** to shift all pads on that slot by a fixed offset, effectively transposing the whole instrument.

### step 5 — a second voice

Run a softer marimba-like tone and bank it to slot 1:

```
N: 132300
T: !N
A: e(T*(0-1.5%N))
I: 2*e(T*(0-40%N))
C: 220*(6.28318%44100)
M: 220*(6.28318%44100)
P: +\(N#C)
Q: +\(N#M)
W: w A*(s P+(I*s Q))
```

220 Hz (an octave below the bell), 3-second decay, index 2 for a gentler attack. Bank it to slot 1.

Now right-click some pads and assign them to slot 1 with their own semitone offsets. You have two voices: some pads play the bell, others play the soft tone, each at its own pitch.

### step 6 — save the session

Go to **File → Save session...** and save a `.json` file. When you reopen the app later, **File → Load session...** restores the slots, pad assignments, notebook cells, and editor text exactly as you left them. The session format is also compatible with the ksynth web app.

---

## editor

Write ksynth code one assignment per line. Comments use `/`.

```
/ 440 Hz sine with envelope
N: 44100
T: !N
F: 440*(6.28318%44100)
P: +\(N#F)
E: e(T*(0-6%N))
W: w (s P)*E
```

**`Ctrl+J`** or **`Ctrl+Enter`** — run the script. A cell is appended to the notebook. Variables `A`–`Z` are reset before each run; every script is a self-contained program.

**`clear`** button — empties the editor.

**`pads`** button — opens or closes the pad window.

### REPL strip

At the bottom of the editor is a single-line calculator. Type any ksynth expression and press **Enter**. The result appears on the line above — no slot or notebook involvement.

```
2+2                  =>  4
440*(6.28318%44100)  =>  0.0627...
N                    =>  44100        (if N was set in the last run)
```

Variables persist between REPL entries. The REPL shares the same variable state as the main editor, so you can run a script then inspect intermediate values in the REPL.

If the result is a vector, the first 6 values and the total length are shown:

```
!8     =>  [0 1 2 3 4 5 ...]  len=8
```

---

## notebook

Each run appends a cell. Successful cells show the status line, source code, waveform, and bank buttons `>0`–`>F`. Failed cells show the error in red.

- **click the source code** — copies it back into the editor
- **click the waveform** — auditions the buffer without banking
- **`x` button** — removes the cell

Cells are preserved across sessions when you save and load.

---

## slots

16 slots in a 2×8 grid across the top, indexed `0`–`F`. Each holds a PCM buffer, a label, and a base tuning offset.

**Banking** — click any `>N` button in a notebook cell.

**Playing** — click a filled slot card (triggers on mousedown).

**Context menu** (right-click):

| Item | Action |
|------|--------|
| Play | Trigger the slot |
| Set tuning... | Base pitch offset in semitones (±24). Shifts all pads on this slot together. |
| Save WAV... | Export the slot buffer as a 16-bit mono WAV file |
| Rename... | Set a custom label |
| Clear slot | Empty the slot |

---

## pad window

Click **`pads`** to open. Click it again or close the window to hide it.

4×4 grid of 16 trigger pads. Each pad shows:

- **large hex number** (top centre) — the slot it is assigned to
- **key hint** (top left, small) — keyboard shortcut
- **slot label** (centre) — the label of the assigned slot
- **semitone offset** (bottom right) — e.g. `+7st`, `0st`

**Keyboard triggers** — work whenever the editor does not have focus, even with the pad window closed:

| Keys | Pads |
|------|------|
| `1`–`9` | pads 0–8 |
| `0` | pad 9 |
| `a`–`f` | pads 10–15 |

**Scroll wheel** over any pad — adjusts its semitone offset up or down.

**Right-click** any pad — opens a dialog to set its slot and semitone offset precisely.

**Presets** (buttons at the top of the pad window):

- **drum** — pad N plays slot N at 0 semitones. One voice per pad, good for kits.
- **melodic** — all pads play slot 0 at offsets −7 through +8. Good for pitched play of a single voice.

These are starting points; each pad is independently configurable after applying a preset.

Final playback rate for any pad:

```
rate = 2^(slot_tuning/12) × 2^(pad_semitones/12)
```

---

## save / load

**File → Save session...** — saves the current state as a `.json` file containing:

- all 16 slot audio buffers (base64-encoded float32 PCM), labels, and tuning offsets
- all 16 pad slot assignments and semitone offsets
- the full notebook history with waveforms
- the current editor text

**File → Load session...** — restores a saved session. The notebook rebuilds from the history, slots are repopulated, pad grid restored, and editor text reloaded. Audio is decoded directly from the file — no scripts are re-run.

Session files are compatible with the ksynth web app. You can save from the browser and load on the desktop, or vice versa. (The web app saves notebook history too; the desktop will restore it on load.)

---

## file → open .ks

**File → Open .ks...** — loads a `.ks` patch file into the editor. The script is not run automatically — review it and press `Ctrl+J` when ready.

You can also pass `.ks` files on the command line to `ksplay`:

```sh
./ksplay kick.ks snare.ks hihat.ks
```

`ksplay` evaluates each file, loads the result into consecutive slots, and plays each one.

---

## notes and limitations

**Variable isolation** — `A`–`Z` reset before each main editor run. Each script is a self-contained program. The REPL preserves state between entries but shares the same namespace, so running a script in the editor affects what the REPL sees.

**W is the output** — every script must end with `W: w ...` to produce audio. If `W` is not set, the run produces an error cell.

**Literal arrays** — every number after the first in a vector literal must start with a digit `0`–`9`. Use `0.5` not `.5`, or the parser stops early. For example: `A: 1 0.5 0.333 0.25` is correct; `A: 1 .5 .333 .25` silently fails.

**Sample rate** — fixed at 44100 Hz.

**Polyphony** — up to 16 simultaneous voices. If more than 16 are triggered, the oldest is evicted.

**Pitch shifting** — playback-rate resampling is used (linear interpolation). This is fast and works well for moderate offsets (±12 semitones). Large offsets (±24) will show mild resampling artifacts. No formant preservation.

**WAV export** — 16-bit mono, 44100 Hz. The exported file contains only the PCM data from the slot, at the slot's original pitch (base tuning is not baked into the export).
