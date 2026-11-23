# PHONES independence diagnostic — original Blackbox 3.1.9

Status: diagnostic built and ARM-emulator tested; **no hardware verification**.
Do not infer that a fourth physical output works from these tests. Full pad routing,
Phones stereo/L/R selectors and selectable monitor mix are deliberately gated on a
successful physical test. No claim of hardware failure has been made either.

Based on Justin Joe's JOEBOX 2 repository, upstream commit
`da84b1bfcfcf289aa1eb97cf5a894e5ffad6d3e7`. Original MIT license and attribution remain
in LICENSE; third-party notices remain in THIRD_PARTY.md.

## Inputs and reproducible build

Owner's stock file: `/Users/J/Desktop/Blackbox mods/BLACKBOX.bin`.
Expected SHA-256: `281ae303d32e5eb52adca7817a648a34e4bd3848017f26673dd9df3905f1341d`.
The patcher rejects every other input, including Joe's already-patched image.

Owner's Joe image SHA-256:
`899da8ed2449740f90475665733d771a97c6e5ab3682676b91956d7f1deaf2a3`.
Owner's BBEXTRA.BIN SHA-256:
`16ad8e16cd4b3dcd81fff0d2d89f7095131356e0d37ef83f6830eb51ee0090fb`.
Those two files are reference/recovery inputs, not modified or redistributed here.
The supplied BLACKBOX.bin hash matches upstream’s published JOEBOX 2 patch manifest.
The current source has not been rebuilt to prove byte identity with that pair.

Requires Python 3 and ARM GNU tools (`arm-none-eabi-gcc`, `objcopy`, `nm`). From the
repository root:

```sh
python3 diagnostics/build.py '/Users/J/Desktop/Blackbox mods/BLACKBOX.bin'
python3 -m venv .venv
.venv/bin/pip install unicorn capstone numpy
.venv/bin/python diagnostics/test_diagnostic.py -v
python3 diagnostics/make_test_audio.py
```

Outputs stay in ignored `out/phones-diagnostic/`. Diagnostic code: 392 bytes with the
installed compiler. Local diagnostic image SHA-256:
`6b9052f53e4488b79632c163bbbc555f8469842ad155e4777a0180801f4e7cea`.
Built with arm-none-eabi-gcc 16.2.0 (Homebrew). Toolchain changes may change that output hash. No firmware images belong in Git,
releases, attachments or issue reports. This is a standalone stock diagnostic,
not a full JOEBOX build, and needs no BBEXTRA.BIN.

## Audio path: observations and limits

Joe's `src/usbdev.c` reports recordings identifying the converter arguments:
`a = Phones`, `b = Out 3`, `c = Out 1`, and the separate converter `d = Out 2`.
These are upstream observations; they are not new hardware verification by this project.

Local examination of the hash-verified stock image establishes these code locations
(address notes only; no stock listings are distributed):

- Audio task calls block function `0x0804cb1c` at `0x0804150e` / `0x0804152a`,
  selecting alternate destination buffers. The block passes its output-pointer
  structure at application offset `0x8fe4` into engine `0x08053164` at `0x0804ce74`.
- Output-pointer structure: offsets `0/4` Phones L/R, `8/0xc` Out 1 L/R,
  `0x10/0x14` Out 2 L/R, `0x18/0x1c` Out 3 L/R; frame count at `0x2c`.
  This jack naming combines the stock pointer trace with Joe's bus mapping.
- FX buses 10 and 11 merge into bus 0 at `0x08053506` / `0x08053516`.
  Bus 0 stereo is copied into Phones and Out 1 at `0x08053554` through
  `0x08053572`. Bus 1 stereo is copied into Out 2 at `0x08053596` / `0x080535a0`
  and added to Phones at `0x08053d82` / `0x08053d8c` when materialized.
  Bus 2 stereo is copied into Out 3 at `0x080535ca` / `0x080535d4`, then added
  to Phones at `0x08053d6c` / `0x08053d76`. Add helper: `0x080617c0`.
  Additional mono buses also add to the appropriate physical-side buffer and
  both headphone sides (for example `0x08053e20` / `0x08053e2e` / `0x08053e3c`).
- Engine gain passes run at `0x08053afa` through `0x08053b5a`. The outer block
  applies a further per-channel gain pass at `0x0804ceea` through `0x0804cf4a`.
  Precise UI gain semantics across every routing path remain to be measured.
- Final packer calls at `0x0804cf62` / `0x0804cf7c` take Phones/Out3/Out1,
  left then right. `0x0806002c` packs three clipped signed 20-bit fields into
  two 32-bit words, with a four-word frame stride. Phones occupies bits 12–31
  of the first word, Out3 spans its low 12 bits and the second word's high 8,
  Out1 occupies bits 4–23 of the second word. Right starts 8 bytes after left.
  Out2 uses `0x0805feb4` at `0x0804cf8e` / `0x0804cfa0` in a separate stream.

The diagnostic redirects ONLY the two three-input conversion calls. It substitutes
flash-resident 32-frame tones for `a`, keeping `b`, `c`, destination and stride.
It never edits the original float buffers. Small chunks avoid persistent RAM,
heap allocation and a large stack buffer. At 48 kHz, Phones L is 1500 Hz and R is
3000 Hz, peak 0.03125 (about -30.1 dBFS). Stock blocks are multiples of 32;
nonmultiple blocks reset tone phase at the next block and can click.

The trace reaches the packed output buffers handed to the stock audio transport.
DMA/SAI register initialization, converter model/slot configuration, PCB wiring,
and analog headphone amplification have **not** been independently established.
The retained stock transport is the candidate path to the converter; only the
physical test can prove these fields independently reach PHONES. This diagnostic
also tests whether the retained transport overwrites or remixes them later.

## Install and recover

1. Back up the SD card, presets, original firmware and Joe's matched two-file pair.
   Use a spare card if possible. Do not save working presets during this test.
2. Copy locally generated `out/phones-diagnostic/BLACKBOX.bin` to the card root.
   Set aside any BBEXTRA.BIN on the test card to avoid confusing build identities;
   this diagnostic neither reads nor writes bank 2 or calibration.
3. Following Joe's documented update procedure, power on holding BACK + INFO;
   release when the install screen appears. Keep power stable until complete.
   This diagnostic still displays stock 3.1.9: identify it by the image hash and tones.
4. To recover stock, replace card-root BLACKBOX.bin with the owner's original
   stock file (verify the expected hash above) and repeat BACK + INFO at power-on.
   Existing bank-2 code from Joe is unused by stock.
5. To return to Joe, restore BOTH owner-supplied matching files to card root and
   use the same update procedure. Allow the additional first-boot restart described
   in Joe's README. Do not mix BBEXTRA.BIN from another release.

Installation/recovery instructions follow upstream README; neither has been
performed on this owner's device in this investigation.

## Physical test protocol and pass gate

Use an audio interface/recorder and TRS stereo-to-two-mono breakout cables.
PHONES is stereo unbalanced: tip = L, ring = R, sleeve = ground. Record its
sides as two separate channels, not a balanced mono input (which subtracts L/R).
Use line-level inputs with phantom power off. Start input gain low; do not wear
headphones for this fixed-tone test. **Headphone volume, pad mute, Stop and normal
monitor controls do not stop these injected tones.** Power-off stops them.

1. On stock first, make a new test preset. Load the three generated one-second
   stereo WAVs onto three pads. Route each to its matching Out 1, 2 or 3 stereo,
   unity pad level, neutral pan, FX sends off. Loop and trigger all three.
   Save this test preset on the test card. Record a stock baseline for all jacks.
2. Install the diagnostic and load that same preset. Trigger all three loops.
   Record eight channels simultaneously if available, or each pair in turn using
   the same settings. Include at least ten seconds of steady playback.

| Physical pair | Expected L | Expected R |
| --- | ---: | ---: |
| Out 1 | 375 Hz | 750 Hz |
| Out 2 | 1125 Hz | 2250 Hz |
| Out 3 | 2625 Hz | 3375 Hz |
| PHONES | 1500 Hz | 3000 Hz |

3. Inspect spectra and listen to recordings at a sensible level. Phones should
   contain its two tones without the six pad tones. Outs 1–3 should match their
   stock baselines without 1500/3000 Hz. Distinguish analog noise/crosstalk from
   coherent duplicated audio: record unwanted-tone level relative to the intended
   tone in dB, plus the stock noise floor. A provisional goal is below -60 dBc;
   report actual measurements rather than claiming perfect isolation.
4. Mute/unmute each pad, Stop/Start, change each Out level and headphone level,
   and turn sends on separately. Phones' injected tones should stay fixed while
   intended line-output behavior changes. Record any leakage, swaps, dropouts,
   distortion, watchdog resets, unexpected volume coupling or loss of audio.
5. Power cycle and repeat. Restore stock and confirm its normal monitor sum.
   If PHONES is silent, still follows the sum, or the test affects line outs,
   stop and report recordings/setup before any routing UI work. Check cable and
   baseline first: a failed diagnostic alone does not prove a hardware limitation.

A pass requires distinct L/R at the actual jack and measured separation from all
six line channels, stable operation and successful recovery. Send recordings
(original WAVs preferred), the output hash, device revision if known, routing
settings, interface/cable setup, volume observations and pass/fail notes. Do not
send firmware images publicly. No PHONES routing interface is authorized by a
software-only pass.

## Results and remaining work

Three focused unittest cases pass: actual ARM tone hooks against the real stock
packer (0/1/31/32/128/256 frames, both sides, preserved source buffers, converter
fields and destination guards); patch scope/preset-code identity; wrong-image and
oversize rejection. These are function-level emulator tests, not full boot tests.
Only eight stock instruction bytes are eligible for replacement; appended code
is 392 bytes. Preset reader/writer and parameter data remain byte-identical.
Preset round trips on hardware remain untested.

Hardware-verified by this investigation: **nothing**.
Unverified: independent physical PHONES, physical channel order, boot/real-time
load, analog levels/crosstalk, gain behavior, recovery and preset round trips.

After a physical pass: implement Phones stereo/L/R with existing enum values
preserved, a separately selectable normal monitor mix, gain placement, pad/mono/
stereo/clip/slicer coverage, FX return routing and preset compatibility tests.
Out-1-only master processing and resampling/recording taps need explicit review.
This diagnostic has no new pad or FX routing, no new saved parameters and no
selectable monitor mode; normal monitoring returns by reinstalling stock/Joe.

## Owner's requested next phase

After PHONES independence works, investigate effects on each output bus, with
four stereo pairs or eight mono channels. Working interpretation: bus processing
of all material routed to that destination. Clarify whether the intended controls
are output insert effects, independent send levels, or both before implementing.
The existing two shared send effects do not establish eight independent effects
engines. CPU budget, stereo/mono pairing, wet-return feedback, master gain order
and saved state need investigation after the physical gate passes.
