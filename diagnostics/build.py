"""Build a standalone, stock-3.1.9-only PHONES diagnostic. No firmware redistribution."""
import hashlib
from pathlib import Path
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'patches'))
from thumb import bl, symbols
ROOT = Path(__file__).resolve().parents[1]
SHA = '281ae303d32e5eb52adca7817a648a34e4bd3848017f26673dd9df3905f1341d'
BASE, CAVE = 0x08040000, 0x080f1e78

def patch(stock, payload, sym):
    if hashlib.sha256(stock).hexdigest() != SHA:
        raise ValueError('Requires original stock 3.1.9, not a JOEBOX image')
    if len(stock) != CAVE - BASE or not payload or CAVE + len(payload) > 0x08100000:
        raise ValueError('Invalid stock length or diagnostic size')
    result = bytearray(stock)
    for at, name in ((0x0804cf62, 'phones_diag_left'), (0x0804cf7c, 'phones_diag_right')):
        off = at - BASE
        if stock[off:off+4] != bl(at, 0x0806002c):
            raise ValueError('Converter call mismatch')
        result[off:off+4] = bl(at, sym[name])
    return bytes(result) + payload

def main():
    stock = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / 'firmware/BLACKBOX-3.1.9.bin').read_bytes()
    if hashlib.sha256(stock).hexdigest() != SHA:
        raise SystemExit('Requires original stock 3.1.9; input hash mismatch')
    out = ROOT / 'out/phones-diagnostic'
    out.mkdir(parents=True, exist_ok=True)
    ld = out / 'diagnostic.ld'
    ld.write_text('SECTIONS { . = 0x080f1e78; .text : { *(.text*) *(.rodata*) } /DISCARD/ : { *(.ARM.exidx*) *(.comment*) } }')
    elf = out / 'diagnostic.elf'
    subprocess.run(['arm-none-eabi-gcc', '-mcpu=cortex-m7', '-mthumb', '-mfloat-abi=hard', '-mfpu=fpv5-d16', '-Os', '-ffreestanding', '-nostdlib', '-fno-builtin', '-Wall', '-Wextra', '-Werror', '-T', str(ld), '-Wl,--entry=phones_diag_left', '-o', str(elf), str(ROOT / 'diagnostics/phones_diag.c')], check=True)
    payload = out / 'diagnostic.bin'
    subprocess.run(['arm-none-eabi-objcopy', '-O', 'binary', str(elf), str(payload)], check=True)
    data = patch(stock, payload.read_bytes(), symbols(str(elf)))
    (out / 'BLACKBOX.bin').write_bytes(data)
    print(f'Local only: {out / "BLACKBOX.bin"}; {len(payload.read_bytes())} diagnostic bytes; SHA-256 {hashlib.sha256(data).hexdigest()}')

if __name__ == '__main__':
    main()
