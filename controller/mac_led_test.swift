// Owner-local USB-MIDI LED check. Lists devices unless --blink is supplied.
import Foundation
import CoreMIDI
import Darwin
func name(_ endpoint: MIDIEndpointRef) -> String {
    var value: Unmanaged<CFString>?
    if MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &value) != noErr { return "unnamed" }
    return value?.takeRetainedValue() as String? ?? "unnamed"
}
let destinations = (0..<MIDIGetNumberOfDestinations()).map { MIDIGetDestination($0) }
for destination in destinations { print(name(destination)) }
guard CommandLine.arguments.contains("--blink") else { exit(0) }
let grids = destinations.filter { name($0).localizedCaseInsensitiveContains("OpenGrid") }
guard grids.count == 1 else {
    fputs("Expected exactly one OpenGrid MIDI destination; found \(grids.count). No messages sent.\n", stderr)
    exit(1)
}
var client = MIDIClientRef()
var port = MIDIPortRef()
guard MIDIClientCreate("OpenGrid LED test" as CFString, nil, nil, &client) == noErr,
      MIDIOutputPortCreate(client, "LED feedback" as CFString, &port) == noErr else {
    fputs("Could not create MIDI output.\n", stderr); exit(1)
}
defer { MIDIPortDispose(port); MIDIClientDispose(client) }
func send(_ bytes: [UInt8]) {
    var list = MIDIPacketList()
    let packet = MIDIPacketListInit(&list)
    let appended = bytes.withUnsafeBufferPointer {
        MIDIPacketListAdd(&list, MemoryLayout<MIDIPacketList>.size, packet, 0, $0.count, $0.baseAddress!)
    }
    guard appended != nil, MIDISend(port, grids[0], &list) == noErr else {
        fputs("MIDI send failed.\n", stderr); exit(1)
    }
}
print("Blinking top-left LED on \(name(grids[0])); channel 16, note 0.")
for _ in 0..<4 {
    send([0x9f, 0, 127]); Thread.sleep(forTimeInterval: 0.5)
    send([0x8f, 0, 0]); Thread.sleep(forTimeInterval: 0.5)
}
print("Finished. The normal keyboard lights should return after about three seconds.")
