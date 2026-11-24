# OpenGrid sequence feedback: investigation

The owner identified the controller as the custom switchable firmware made in
Codex. The earlier chat titled "Build Open Grid Controller" records the latest
quadrants/menu build, followed by the seven-wide revision: channels 1–4, scale/root
menu, and physical center columns 8/9 disabled (including LEDs). This is different
from the public monome-serial-only firmware.

The preserved source snapshot uses `Adafruit_USBD_MIDI`. Its main loop calls
`midi.readPacket` in a bounded loop but discards the received packets. Its LED
rendering shows local held-note/root state. Thus USB-MIDI receive is present in
that source, but external LED feedback is not implemented. The subsequent
seven-wide changes do not change that receive loop. This is source-history
evidence, not confirmation of the firmware currently installed on the controller.

The source was recorded at
`/Users/J/Desktop/OpenGrid-Firmware/quadrants-menu-source/src/main.cpp`.
That folder is currently absent. The snapshot and later edits are available in
the prior chat; no controller firmware was edited or flashed during this inquiry.

## Proposed connection

Blackbox USB-A host → OpenGrid USB device, with music notes/buttons going toward
the Blackbox and a distinct feedback stream going back. First confirm that the
Blackbox enumerates the controller's composite USB-MIDI interface and can deliver
one outbound message that changes one LED. No computer would be required for the
finished direct connection if that handshake succeeds.

An initial feedback protocol can use Note On/Off on a reserved MIDI channel
outside the existing button channels 1–4: note number identifies a physical LED,
velocity its brightness. This fits the note traffic the stock USB-host packer
already supports. Do not assume arbitrary SysEx or CC is forwarded unchanged:
`src/usbdev.c` documents filtering in the stock packer.

Keep feedback out of the normal musical note path and out of the USB power-port
MIDI mirror. Queue/append feedback only to the host-bound packet buffer with
strict bounds and priority for musical notes/clock. The controller must update
an external LED plane without treating received feedback as button presses or
sending it back again.

For a complete view, Blackbox must provide stored pattern contents as well as
playhead, transport and pattern/edit changes. Clock and note events alone cannot
describe notes that have not played. JOEBOX's `seqfix.c` and `rotate.c` already
identify event lists and playback position, but snapshotting those safely for a
USB/UI task still needs design and tests.

Start with one LED, then a moving playhead, then a selected-pattern view. Preserve
existing play/menu modes. The owner's disabled center columns leave 14 active
columns, so paging/layout must explicitly respect that gap. Bidirectional step
editing can follow after display synchronization works.

## Single-LED diagnostic implementation

Branch `grid-led-diagnostic` implements the opt-in JBOX G1 heartbeat in
`src/grid_led.c` and appends it after stock music packing in `src/usbdev.c`.
The controller source and verification steps are in
`controller/opengrid/README.md`. Software checks exercise the actual ARM packer,
packet bounds/music priority, power-port isolation, and the recovered controller
loop including timeout/menu/key handling. Both target builds compile.

Owner-reported hardware evidence: changing the wall adapter restored touchscreen
operation while the grid was connected; normal controller operation was reported.
This does not verify outbound LED delivery. No G1 flash, direct LED handshake,
sequence visualization, or physical Out4 separation has been verified. Independent
per-pad delay/reverb remains future work.
