"""Actual ARM heartbeat and stock USB-MIDI packer. Requires GRID_LED_DIAG=1 build."""
import os
from pathlib import Path
import struct
import unittest
from unicorn import UC_HOOK_MEM_WRITE
from emu import Emu, STACK
ROOT=Path(__file__).resolve().parent
IMAGE=os.environ.get('GRID_LED_IMAGE','out/solo+slice+duck+chord+cond+filter+cpu+od+comp+seqfix+fx2+munchi+master+fault+cave/BLACKBOX.bin')
ELF=os.environ.get('GRID_LED_ELF','out/cave.elf')
MASTER,STATE=0xc0000000,0xc009a960
DST,LEN=0x24070000,0x24070400
MGR,QUEUE=0x24002e88,0x24002e88+0x4b68
USB,USBSTATE=0x40040000,0x38800680
class GridLedTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not Path(IMAGE).exists() or not Path(ELF).exists():raise unittest.SkipTest('Build GRID_LED_DIAG=1 first')
        e=Emu(IMAGE,ELF)
        if 'grid_led_append' not in e.sym:raise unittest.SkipTest('Opt-in GRID_LED_DIAG build only')
    def setUp(self):
        self.e=e=Emu(IMAGE,ELF)
        e.uc.mem_map(MASTER,0xa0000);e.uc.mem_map(0x38800000,0x1000)
        e.uc.mem_map(0x58024000,0x1000);e.uc.mem_map(USB,0x4000)
        e.w32(0x38800f70,MASTER);e.w32(0x38800f74,MASTER^0x5052534d)
        e.call('phones_boot',MASTER)
        e.uc.mem_write(DST,b'\xa5'*64);e.w32(LEN,0)
    def append(self,capacity=4):
        self.e.call('grid_led_append',DST,capacity,LEN)
        return self.e.arg(0)
    def pending(self):self.e.uc.mem_write(STATE,struct.pack('<HBB',188,0,0))
    def test_half_second_transitions_and_guards(self):
        for _ in range(187):self.assertEqual(self.append(),0)
        self.assertEqual(self.append(),1)
        self.assertEqual(bytes(self.e.uc.mem_read(DST,8)),bytes.fromhex('099f007f')+b'\xa5'*4)
        self.assertEqual(self.e.r32(LEN),4)
        self.e.w32(LEN,0)
        for _ in range(187):self.assertEqual(self.append(),0)
        self.assertEqual(self.append(),1)
        self.assertEqual(bytes(self.e.uc.mem_read(DST,4)),bytes.fromhex('088f0000'))
    def test_music_full_buffer_holds_one_pending_transition(self):
        self.pending();self.e.w32(LEN,4)
        for _ in range(300):self.assertEqual(self.append(),0)
        self.assertEqual(self.e.r16(STATE),188)
        self.assertEqual(self.e.r8(STATE+2),0)
        self.assertEqual(self.e.r32(LEN),4)
        self.assertEqual(bytes(self.e.uc.mem_read(DST,8)),b'\xa5'*8)
        self.e.w32(LEN,0);self.assertEqual(self.append(),1)
        self.assertEqual(self.e.r16(STATE),0)
    def test_capacity_alignment_and_invalid_lengths(self):
        for capacity,length in ((0,0),(3,0),(4,1),(4,5),(7,4),(8,6),(0xffffffff,0xfffffffe)):
            self.pending();self.e.w32(LEN,length)
            self.assertEqual(self.append(capacity),0,(capacity,length))
            self.assertEqual(self.e.r32(LEN)&0xffffffff,length)
            self.assertEqual(bytes(self.e.uc.mem_read(DST,16)),b'\xa5'*16)
        self.pending();self.e.w32(LEN,4);self.assertEqual(self.append(8),1)
        self.assertEqual(bytes(self.e.uc.mem_read(DST,12)),b'\xa5'*4+bytes.fromhex('099f007f')+b'\xa5'*4)
    def test_missing_memory_and_null_arguments(self):
        self.pending();self.e.w32(0x38800f70,0);self.e.w32(0x38800f74,0x5052534d)
        self.assertEqual(self.append(),0)
        self.e.w32(0x38800f70,MASTER);self.e.w32(0x38800f74,MASTER^0x5052534d)
        self.e.call('grid_led_append',0,4,LEN);self.assertEqual(self.e.arg(0),0)
        self.e.call('grid_led_append',DST,4,0);self.assertEqual(self.e.arg(0),0)
    def test_boot_clears_previous_phase(self):
        self.e.w32(STATE,0xa5a5a5a5);self.e.call('phones_boot',MASTER)
        self.assertEqual(self.e.r32(STATE),0)
    def put(self,*messages):
        e=self.e
        for m in messages:
            w=e.r32(QUEUE+0x504)&0xffffffff
            e.uc.mem_write(QUEUE+(w&255)*5,bytes(m).ljust(4,b'\0')+bytes([len(m)]))
            e.w32(QUEUE+0x504,w+1)
    def pump(self,capacity):
        self.e.w32(STACK,LEN);self.e.call('usbdev_midi_out',MGR,QUEUE,DST,capacity)
        return self.e.arg(0),bytes(self.e.uc.mem_read(DST,self.e.r32(LEN)))
    def test_stock_music_has_priority_over_led(self):
        self.pending();self.put([0x90,60,100])
        result,data=self.pump(4)
        self.assertEqual((result,data),(1,bytes.fromhex('09903c64')))
        self.assertEqual(self.e.r16(STATE),188)
        self.assertEqual(self.e.r32(QUEUE+0x500),1)
        result,data=self.pump(4)
        self.assertEqual((result,data),(1,bytes.fromhex('099f007f')))
        self.assertEqual(self.e.r32(QUEUE+0x504),1)
        self.assertEqual(self.e.r32(QUEUE+0x500),1)
    def test_power_port_mirror_has_music_only(self):
        e=self.e
        e.w32(0x580244d8,1<<25)
        e.uc.mem_write(USBSTATE+10,bytes([3,1]))
        fifo=[]
        e.uc.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,a,size,value,data:fifo.append(value),begin=USB+0x2000,end=USB+0x2000)
        self.pending();self.put([0x90,60,100],[0xf8])
        result,data=self.pump(12)
        self.assertEqual((result,data),(1,bytes.fromhex('09903c640ff80000099f007f')))
        self.assertEqual(fifo,[0x643c9009,0x0000f80f])
        self.assertEqual(e.r32(QUEUE+0x504),2)
        self.assertEqual(e.r32(QUEUE+0x500),2)
    def test_idle_host_packet_does_not_echo_led_to_power_port(self):
        e=self.e;e.w32(0x580244d8,1<<25);e.uc.mem_write(USBSTATE+10,bytes([3,1]))
        fifo=[];e.uc.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,a,size,value,data:fifo.append(value),begin=USB+0x2000,end=USB+0x2000)
        self.pending();self.assertEqual(self.pump(4),(1,bytes.fromhex('099f007f')))
        self.assertEqual(fifo,[])
if __name__=='__main__':unittest.main()
