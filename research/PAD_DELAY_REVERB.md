# Next phase: independent delay and reverb per pad

Owner's revised priority: effects belong to each of the 16 pads, ahead of output
routing. Each pad needs its own delay and reverb controls and memory; changing a
pad's destination must not attach it to another pad's effect state. This replaces
the earlier request for effects per physical output. Not implemented in JBOX 2P.

The existing insert hooks are the starting point: they create a private pad mix
for Sample/Multisample and Clip/Slicer before sends and output assignment. A pad
with an enabled delay/reverb must take that path too. Proposed processing order:
existing insert → independent delay → independent reverb → existing sends/output.
Stereo and mono pad semantics, mute behavior and tails must be tested explicitly.

The stock early return for pads with no voices also needs extension: effect tails
must keep rendering after a voice stops. Idle instances should skip DSP only after
their tails have drained. Bypass, effect changes, preset changes and sample changes
must have deliberate state-clearing behavior, with no cross-pad carryover.

## Existing code candidates

- `rev4_step` in `src/rev4.c` already takes separate `struct rev4` and line-memory
  pointers. It is a candidate for independent instances. It is a small mono-input,
  stereo-output reverb, not the full Room/Plate algorithm.
- `room` in `src/fx2_room.c` has buffer pointers in its state argument but uses a
  fixed global `S` at backup SRAM `0x38800a00`. Calling it for multiple pads as-is
  would share positions and filters. It needs an explicit per-instance state
  argument before reuse; the existing shared FX2 instance must keep working.
- The shared delay-slot replacement supports several algorithms, but its slot
  assumptions and fixed state cannot simply be called sixteen times. Begin with
  a separate stereo delay core with explicit state and fixed reserved storage.

## Initial resource measurements (software only)

Current ARM build: 128 calls to `rev4_step`, 215 emulated instructions per frame
on the exercised no-wrap path. Sixteen instances at 48 kHz imply approximately
165 million instructions/second for this reverb alone. This excludes delay,
interpolation, pad rendering, modulation, UI, USB and the remaining firmware.
Instruction counts are not cycles and do not establish real-time feasibility.
Other buffer-wrap branches also need timing.

Storage examples, not a chosen delay-time limit:

| Component | One pad | Sixteen pads |
| --- | ---: | ---: |
| rev4 lines + state | 48,984 B | 783,744 B |
| Stereo float delay, 2 seconds at 48 kHz | 768,000 B | 12,288,000 B |

The combined example is about 12.47 MiB before scratch buffers and parameter
state. Actual allocatable sample memory must be measured on the original unit;
these figures are requirements, not a claim that memory is available. Storage
format, processing rate and maximum delay length can trade memory against CPU.

Before implementation: measure available RAM and base audio load on the unit;
choose and document the DSP quality/maximum length; reserve storage before audio
starts; check existing 96-entry pad-store capacity for the new controls; add save/
load defaults and cross-pad isolation tests. No heap allocation in the audio
callback. CPU testing must include active tails on several pads alongside normal
sample playback, not merely a single synthetic engine call.

The current routing build also leaves only **721 bytes** below the bank-1 image
limit. New DSP belongs in bank 2 behind its existing validity checks; additional
bank-1 hooks/UI will require deliberate space planning. Existing JOEBOX features
must remain intact. Flash capacity and DSP instruction counts are separate limits.
