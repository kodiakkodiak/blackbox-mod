# Experimental Phones routing — JBOX 2P

This branch continues after the owner explicitly requested proceeding on the
assumption that independent PHONES audio works, without an audio test. The owner
reports the expected 3.1.9 version on the preceding diagnostic. Physical separation
has NOT been confirmed. This routing build's boot, UI and real-time behavior have
NOT been tested on the Blackbox. The diagnostic remains in `diagnostics/`.

Original upstream: Justin Joe's JOEBOX 2, commit
`da84b1bfcfcf289aa1eb97cf5a894e5ffad6d3e7`. MIT license and all upstream/third-party
attributions remain intact. No stock or patched firmware images are committed,
pushed, or distributed. Outputs are for the owner's local build/install only.

## Use

- Each pad's existing Output selector retains values 0–8 and their labels, then
  adds **Phones stereo**, **Phones L**, **Phones R** as values 9–11.
- On **Tools**, select **Headphone**, then press **INFO**. The Phones page has
  the existing Headphone volume and a **Phones** mode selector.
- **Monitor Mix** (default) retains the original Out1–3 headphone monitor sum,
  adding any Phones-assigned pads/returns. Old presets keep normal monitoring.
- **Independent** removes that monitor sum: PHONES contains only material
  assigned to Phones stereo/L/R. Choose this mode for the fourth output.
- FX1/FX2 **OUT → Output** also appends the same three destinations. These route
  the shared wet returns; they do not instantiate independent effects per output.
- Mono samples use unity signal on their selected side(s). Stereo on Phones stereo
  preserves L/R; Phones L/R folds stereo as `(L + R) / 2` to the selected side.
  Opposite-polarity stereo material can cancel when folded, as with other mono sums.
- Headphone volume remains the overall Phones control. The new signal takes the
  same engine Phones gain and the subsequent stock per-channel gain as the stock
  monitor; pad gain/pan/inserts precede routing. Analog levels remain unmeasured.
- Stop does not generate the diagnostic tones in this build. This is the full
  JOEBOX feature build with routing added, not the fixed-tone diagnostic.

## Audio path and compatibility

See `diagnostics/README.md` for the stock sum, conversion and SAI/DMA address trace.
The engine copies/sums output buses into Phones in software. Its final packer has
separate Phones/Out3/Out1 fields on SAI1 A; Out2 uses SAI2 A. Independent packed
fields do not prove an independently controllable analog jack.

`phones_engine` wraps the existing `duck_block` (retaining LFO, insert, stretch,
repeat and duck updates), which calls the stock engine. Private Phones buffers
are reset before that engine pass. After it returns, `phones_finish` retains or
replaces the monitor sum, adds the private mix and applies the engine Phones gain.
The outer gain stage and existing converter/USB audio hooks then run as before.

Important: pad values 9–11 cannot simply be sent to the existing bus table:
stock bus 9 is used by recording and buses 10/11 by effect returns. Only the
Sample/Multisample and Clip/Slicer pad-process output lookups are intercepted and
redirected to private stock-compatible buffer objects. The FX-return router
explicitly uses those objects too. Stock recording/send bus indices are untouched.

The memory region is 0x6100 bytes, at offset `0x94920` in the master allocation,
after all existing users, including stretch's pointer. Total master allocation
is now 0x9aa20 bytes. The three private buses have capacity for up to 1024 frames
per channel; each block's active capacity is set from the actual frame count.
Silent flags prevent stale audio from being mixed on subsequent blocks. No
allocation or firmware flash operations occur in the audio callback.

Existing output enum values and `outputbus` key are unchanged. The new global
setting is `phonesmode` (id 0x14b), default 0. It uses the existing global settings
persistence path (`settings.tml`), rather than requiring a new preset format.
FX return destinations retain old values 0–2 and append 3–5 under `fxoutdest`.
New per-pad and FX-return selections use the existing preset reader/writer.
As in stock, pads without a sample filename have their routing reset on preset
load; configure/save these destinations on loaded pads.
The writer serializes Phones menu indices 9–11 as XML `outputbus` 12–14; the
loader maps those back. Stock's legacy XML 10/11 conversions to Out1 plus FX1/FX2
sends remain intact. The unexposed old pad value 9 (recording bus) is not supported
as a pad output by this branch; the nine public Out1–3 choices remain unchanged.

Do not open a Phones-routed preset in stock or unmodified JOEBOX: their audio
engine does not understand the appended pad destinations. For downgrade, use
original preset backups or first route every Phones pad/return back to Out1–3
and save a separate copy. Stock/Joe will ignore the additional mode key, but that
does not make the new output enum values safe for their engines.

## Paths and limits

| Path | Implementation status | Hardware status |
| --- | --- | --- |
| Sample/Multisample output | Private stereo/L/R bus lookup | Unverified |
| Clip/Slicer output | Same lookup, second pad process | Unverified |
| Per-pad level/pan/insert | Existing processing preserved before bus mix | Unverified with new route |
| FX1/FX2 wet return | Phones stereo/L/R added | Unverified |
| Headphone gain | Engine gain plus retained outer gain stage | Unverified physically |
| Normal headphone monitor | Default Monitor Mix retains stock sum | Unverified in this build |
| Out1 Master FX/compressor/clipper | Remain Out1-only; Phones bypasses them | No new hardware test |
| USB Audio | Still six channels, Outs1–3; Phones is excluded | Unchanged source path |
| Recording/resampling | Stock source choices; no new Phones capture source | Unverified; not extended |
| Live input | Can follow an existing live-input pad routed to Phones | Unverified end-to-end |
| Independent delay/reverb per pad | Owner’s revised next priority; not implemented here | CPU/memory feasibility not established |

If the master permanent allocation fails or an unsupported frame count is seen,
new pad/return selections fall back to Out1 and Phones retains the stock monitor.
This is a degraded-memory fallback, not proof that the Phones path works. It
must be reported if encountered; existing effects also depend on that allocation.

## Build and install locally

Use the original owner-supplied stock 3.1.9 image, SHA-256
`281ae303d32e5eb52adca7817a648a34e4bd3848017f26673dd9df3905f1341d`, at
`firmware/BLACKBOX-3.1.9.bin`. Do not patch an already-modded image.

```sh
RELEASE=1 ./build.sh
python3 patch.py solo slice duck chord cond filter cpu od comp seqfix fx2 munchi master fault cave
```

Copy the generated image from `out/solo+slice+duck+chord+cond+filter+cpu+od+comp+seqfix+fx2+munchi+master+fault+cave/BLACKBOX.bin`
and `out/BBEXTRA.BIN` to the card root. The owner's convenience copy is in ignored
`out/phones-routing/`, with a local SHA-256 manifest. Both files belong together.
Version is **JBOX 2P**, to distinguish this unverified branch from Joe's release.
The bank-2 payload currently matches the owner's supplied JOEBOX 2 BBEXTRA.BIN
byte-for-byte; it is nevertheless included in the local install pair.

Preserve original presets/settings and firmware on the Mac. Install using the
upstream procedure: hold BACK + INFO while powering on, release at the install
screen, and keep power stable until fully booted. Allow an extra reboot if the
bank-2 loader needs it. This build has no USB raw-memory write/debug interface.

Recover stock by restoring the owner's ORIGINAL BLACKBOX.bin to the card root
and repeating BACK + INFO at power-on. Return to Joe by restoring BOTH files of
the owner's matched JOEBOX 2 pair. Use original preset backups on downgrade.

## Hardware test still required

Start with the existing diagnostic test recordings if possible. For this build,
use a new preset, sends off, and choose Independent mode. Route distinct stereo
loops to Out1–3 and a fourth loop to Phones. Record all eight physical channels,
then test Phones L and Phones R separately. Mute/stop each pad, vary Headphone
and Out1–3 volumes, enable wet returns one at a time, save/reload and power cycle.
Check that only the intended channels change and no audio leaks to the line outs.
Restore Monitor Mix and confirm ordinary monitoring. Record the actual crosstalk,
channel order, levels, resets/dropouts, and firmware hash; do not infer a hardware
pass from menu labels or emulator results.

## Revised next phase

The owner subsequently prioritized **independent delay and reverb per pad** over
effects per output. Existing per-pad inserts and two shared send engines are
retained here. Per-pad delay/reverb needs its own resource budget, independent
states, bypass/tail behavior, preset keys and routing tests. No claim is made
that sixteen simultaneous delay/reverb pairs fit the original Blackbox.

Initial per-pad delay/reverb feasibility notes are in `research/PAD_DELAY_REVERB.md`.

## Software validation results

All **55 root-level test suites passed** during validation, including reruns after
installing the missing SciPy dependency and updating expectations for the extra
global setting. Routing, registry, UI, bank integrity and preset checks were
rerun after the preset-codec change. Eight focused `test_phones.py` cases cover
real stock buffer mixing, both actual pad-process lookup sites, stereo/mono and
side routing, monitor mode, gain, FX returns, block resets, missing-allocation
fallback and memory guards for blocks through 1024 frames.

The actual stock registry/loader and modified writer preserve old public routes,
new Phones routes, legacy XML 10/11 FX-send shortcuts and repeated load/save.
The large existing preset test also passes stock-byte comparisons, duplicate-key
handling, heap/stack checks and simulated file-error paths. Tests do not simulate
a complete boot or physical analog conversion.

Built with ARM GCC 16.2.0: bank-1 image **785,711 bytes**, **721 bytes** below its
limit; appended code 57,015 bytes. Local image SHA-256:
`cae5561754e5a605138d0aa0dbfc1457d707c154f929aca6d8cfdcd1c42def63`.
BBEXTRA.BIN is 97,920 bytes and still matches the owner's supplied pair.

Four distinct stereo test WAVs can be generated locally:

```sh
python3 diagnostics/make_test_audio.py --phones --output out/phones-routing/test-audio
```

Load these on four pads, route to their named destinations, select Independent,
and loop/trigger all four. PHONES should play 1500/3000 Hz, without the other
six frequencies. All new routing audio, physical isolation, UI rendering, real
CPU load and recovery of this build remain **hardware-unverified**.
