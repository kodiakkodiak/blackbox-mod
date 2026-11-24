"""Copy the owner's existing FREEPOET keypad dependency; never download or redistribute it."""
from pathlib import Path
import argparse
import hashlib
import shutil
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',type=Path,help='existing Open-Grid-Controller sw/src folder')
args=parser.parse_args()
expected={'keypad.cpp':'20fda39ac5275bd9bebf75f66c865a18e21a6b543946a1eefcee0cf104aa6c38','keypad.h':'d73a1a4459feebd1ba4e5b3c3c8fd807d30007444fd47c8e2d87b3c484f46a22'}
for name,digest in expected.items():
 src=args.source/name;dst=Path(__file__).resolve().parent/'src'/name
 assert hashlib.sha256(src.read_bytes()).hexdigest()==digest,f'{name}: source revision differs; review before building'
 if dst.exists():assert dst.read_bytes()==src.read_bytes(),f'{name}: local dependency differs; review before replacing'
 else:shutil.copyfile(src,dst)
 print(f'Verified {name}')
