"""Generate owner-local, stereo 48 kHz reference loops for the physical test."""
from pathlib import Path
import math
import argparse
import struct
import wave
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--phones', action='store_true', help='also generate a fourth stereo loop for the routing build')
parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1] / 'out/phones-diagnostic/test-audio')
args = parser.parse_args()
out = args.output
out.mkdir(parents=True, exist_ok=True)
pairs = [('Out1', (375, 750)), ('Out2', (1125, 2250)), ('Out3', (2625, 3375))]
if args.phones:
    pairs.append(('Phones', (1500, 3000)))
for bus, pair in pairs:
    data = bytearray()
    for i in range(48000):
        data.extend(struct.pack('<hh', *(round(1024 * math.sin(2*math.pi*f*i/48000)) for f in pair)))
    path = out / f'{bus}-{pair[0]}L-{pair[1]}R.wav'
    with wave.open(str(path), 'wb') as w:
        w.setparams((2, 2, 48000, 0, 'NONE', 'not compressed'))
        w.writeframes(data)
    print(path)
