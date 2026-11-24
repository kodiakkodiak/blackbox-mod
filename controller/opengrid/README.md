# OpenGrid single-LED feedback diagnostic

This is a test of Blackbox → controller USB-MIDI delivery, not a sequencer display.
The matching Blackbox build says **JBOX G1**, retains Out4/Out4L/Out4R, and opts
into a channel-16, note-0 heartbeat on USB-A only. Do not attach another synth to
that diagnostic host port: it receives these test notes. Music is packed first;
feedback uses remaining packet space and never enters the power-port MIDI mirror.

## Source and build

The owner's custom seven-wide quadrants firmware was recovered from the earlier
“Build Open Grid Controller” Codex chat: the recorded main.cpp and subsequent
seven-wide edits. The old Desktop/OpenGrid-Firmware directory is absent. This is
a reconstructed source baseline, not a verified copy of the installed flash.
Existing four MIDI channels, scale/root menu, and two disabled center columns
are preserved. New feedback code is MIT; retain the repository's Joe/upstream
license and attribution.

The Freepoet keypad driver has no located redistribution license, so it is not
included. `prepare_keypad.py` verifies and copies the owner's existing local files;
read its hash checks before selecting a different driver. Build from repo root:

```
python3 controller/opengrid/prepare_keypad.py "/Users/J/Desktop/Open-Grid-Controller-main/sw/src"
.venv-grid/bin/pio run -d controller/opengrid
.venv/bin/python -m unittest test_grid_controller
GRID_LED_DIAG=1 RELEASE=1 ./build.sh
.venv/bin/python patch.py solo slice duck chord cond filter cpu od comp seqfix fx2 munchi master fault cave
.venv/bin/python -m unittest test_grid_led test_phones
.venv/bin/python test_usbdev.py
```

PlatformIO 6.2.0 was used with the pinned platform in platformio.ini and its
Arduino-Pico 5.6.0 framework / bundled TinyUSB 3.7.7. Generated binaries stay local
in ignored build folders; never commit 1010music firmware or patched images.

## Receive protocol

Cable 0, MIDI channel 16, Note On/Off; note = x*8+y for the 16×8 physical matrix.
Velocity controls brightness. Note On velocity zero is off. Center columns 7/8
remain dark. Other channels, malformed packets and other USB cables are ignored.
Receive processing is bounded to 16 packets per loop and never sends an echo.
External lights temporarily replace root lights; held keys remain visible, menu
has priority, and three seconds without feedback restores the original view.
Disconnect also clears feedback. The heartbeat blinks the top-left LED, nominally
half a second on/off (actual rate depends on Blackbox packer scheduling).

## Hardware verification and recovery

1. Connect the grid to the Mac first. Save its existing flash before replacing it;
   prefer an exact device backup over the reconstructed baseline UF2. In RP2040
   BOOTSEL mode, `picotool save -a BACKUP.uf2` saves all flash. Keep that file local.
2. Install the locally built OpenGrid-G1.uf2 using the Pico BOOTSEL drive. Check
   the ordinary keys, four quadrants and menu still work. The optional Mac helper
   `swiftc controller/mac_led_test.swift -o out/controller-tests/mac-led-test`
   lists destinations without sending; run it with `--blink` only with this grid
   attached. It sends four channel-16 LED cycles, then normal lights resume.
3. Back up the Blackbox SD install files. Copy the matching local G1 BLACKBOX.bin
   and BBEXTRA.BIN to its root, then follow the Blackbox firmware-update procedure
   already documented in out/phones-routing/INSTALL-AND-TEST.md. Confirm JBOX G1.
4. Use the working wall power adapter that resolved the touchscreen problem.
   Connect the grid directly to Blackbox USB-A; enable USB device MIDI output in
   Blackbox settings if needed. Check top-left blink with transport stopped and
   running, key notes, clock, touchscreen, menu, and unplug/replug recovery.
5. Record pass/fail for each check. Stop if touch/audio fails. A successful Mac
   blink alone does not prove the direct Blackbox link works.

Restore the exact grid backup via BOOTSEL to roll back. The separately built
OpenGrid-reconstructed-baseline.uf2 is a fallback from recovered source, not an
exact original backup. For Blackbox rollback use the owner's saved installer pair
or the normal local JBOX 2P2 pair; follow the stock recovery instructions in the
routing install document to restore the owner's stock 3.1.9 image. Never mix G1
and unrelated bank-2 images. No physical LED test or flash has yet been performed.
