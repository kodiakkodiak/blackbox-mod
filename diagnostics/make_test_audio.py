"""Generate owner-local, stereo 48 kHz reference loops for the physical test."""
from pathlib import Path
import math
import struct
import wave
out = Path(__file__).resolve().parents[1] / 'out/phones-diagnostic/test-audio'
out.mkdir(parents=True, exist_ok=True)
for bus, pair in enumerate(((375, 750), (1125, 2250), (2625, 3375)), 1):
    data = bytearray()
    for i in range(48000):
        data.extend(struct.pack('<hh', *(round(1024 * math.sin(2*math.pi*f*i/48000)) for f in pair)))
    path = out / f'Out{bus}-{pair[0]}L-{pair[1]}R.wav'
    with wave.open(str(path), 'wb') as w:
        w.setparams((2, 2, 48000, 0, 'NONE', 'not compressed'))
        w.writeframes(data)
    print(path)
