"""Compiled ARM private routing plus actual stock bus mixer. No physical-jack claims."""
import struct
import unittest
from pathlib import Path
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_SP
from emu import Emu, STACK

IMAGE = 'out/solo+slice+duck+chord+cond+filter+cpu+od+comp+seqfix+fx2+munchi+master+fault+cave/BLACKBOX.bin'
MASTER, REGION = 0xc0000000, 0xc0094920
ENGINE, OUTPUT, SRC, SL, SR = 0x24010000, 0x24040000, 0x24041000, 0x24042000, 0x24043000
SETTINGS, ITEMS = 0x24020088 + 0x8c90, 0x24044000

class PhonesTests(unittest.TestCase):
    def setUp(self):
        self.e = e = Emu(IMAGE, 'out/cave.elf')
        e.uc.mem_map(0xc0000000, 0xa0000)
        e.uc.mem_map(0x38800000, 0x1000)
        e.uc.mem_map(0x58024000, 0x1000)
        e.w32(0x38800f70, MASTER); e.w32(0x38800f74, MASTER ^ 0x5052534d)
        e.call('phones_boot', MASTER)
        self.n = 128
        self.ptrs = [0x24050000 + i * 0x2000 for i in range(8)]
        for i,p in enumerate(self.ptrs): e.w32(OUTPUT + i * 4, p)
        e.w32(OUTPUT + 0x2c, self.n)
        e.uc.mem_write(ENGINE + 0x28d64, struct.pack('<f', 1.0))
        e.stub(e.sym["duck_block"], value=1)  # orchestrator only; actual stock bus helpers run below
        self.reset()

    def mode(self, mode):
        e=self.e
        e.uc.mem_write(ITEMS, struct.pack('<HHi', 0x14b, 0, mode))
        e.w32(SETTINGS+4, ITEMS); e.uc.mem_write(SETTINGS+10, struct.pack('<H',1))

    def write(self, at, values): self.e.uc.mem_write(at, struct.pack(f'<{len(values)}f', *values))
    def read(self, at): return struct.unpack(f'<{self.n}f', self.e.uc.mem_read(at, self.n*4))

    def reset(self, mode=0):
        self.mode(mode)
        for i,p in enumerate(self.ptrs): self.write(p, [0.125 * (i+1)] * self.n)
        self.e.call('phones_engine', ENGINE, OUTPUT, 0)

    def bus(self, route):
        self.e.call('phones_pad_bus', ENGINE, route)
        return self.e.arg(0)

    def mix(self, route, left, right=None):
        self.write(SL,[left]*self.n); self.write(SR,[right or 0]*self.n)
        self.e.uc.mem_write(SRC, struct.pack('<IIIIBBH',self.n,self.n,SL,SR,0,int(right is not None),0))
        self.e.call(0x0804da6c, self.bus(route), SRC)

    def finish(self): self.e.call('phones_finish', ENGINE, OUTPUT)

    def test_old_routes_and_new_private_buses(self):
        for route in range(9):
            self.assertEqual(self.bus(route), ENGINE + 0x150e4 + route*20)
        for route in (9,10,11):
            p=self.bus(route)
            self.assertEqual(p, REGION + 4 + (route-9)*20)
            self.assertEqual(self.e.r32(p),self.n)
            self.assertEqual(self.e.r8(p+16),1)
            self.assertNotEqual(p,ENGINE+0x150e4+route*20)
        for site,name in ((0x080596d2,'phones_pad_bus'),(0x08067fc6,'phones_pad_bus'),(0x0804ce74,'phones_engine'),(0x0808d12e,'phones_output_register')):
            import sys
            sys.path.insert(0,'patches')
            from thumb import bl
            self.assertEqual(bytes(self.e.uc.mem_read(site,4)),bl(site,self.e.sym[name]))

    def test_actual_sample_and_clip_process_bus_lookup(self):
        from unicorn.arm_const import UC_ARM_REG_R1, UC_ARM_REG_LR
        from emu import RET
        e=self.e
        pad,vt=0x24045000,0x24047000
        e.w32(pad,vt);e.w32(vt+0x54,0x08046a15)
        for entry,after in ((0x080596b0,0x080596d6),(0x08067fa4,0x08067fca)):
            for route in range(12):
                e.uc.mem_write(pad+0x1e,struct.pack('<H',route))
                e.uc.reg_write(UC_ARM_REG_R0,pad)
                e.uc.reg_write(UC_ARM_REG_R1,ENGINE)
                e.uc.reg_write(UC_ARM_REG_SP,STACK)
                e.uc.reg_write(UC_ARM_REG_LR,RET|1)
                e.uc.emu_start(entry|1,after,count=10000)
                actual=e.arg(0)
                want=ENGINE+0x150e4+route*20 if route<9 else REGION+4+(route-9)*20
                self.assertEqual(actual,want,(hex(entry),route))

    def test_independent_stereo_and_mono_and_no_line_leak(self):
        for route,want in ((9,(.25,-.125)),(10,(.0625,0)),(11,(0,.0625))):
            self.reset(1)
            untouched=[self.read(p) for p in self.ptrs[2:]]
            self.mix(route,.25,-.125)
            self.finish()
            self.assertEqual(self.read(self.ptrs[0]),(want[0],)*self.n)
            self.assertEqual(self.read(self.ptrs[1]),(want[1],)*self.n)
            self.assertEqual(untouched,[self.read(p) for p in self.ptrs[2:]])
        for route,want in ((9,(.25,.25)),(10,(.25,0)),(11,(0,.25))):
            self.reset(1);self.mix(route,.25);self.finish()
            for p,x in zip(self.ptrs,want):self.assertEqual(self.read(p),(x,)*self.n)

    def test_monitor_gain_sum_and_block_reset(self):
        self.reset(0)
        before=[self.read(p) for p in self.ptrs]
        self.finish()
        self.assertEqual(before,[self.read(p) for p in self.ptrs])
        self.mix(9,.25,-.125);self.mix(10,.5);self.mix(11,.125)
        self.e.uc.mem_write(ENGINE+0x28d64,struct.pack('<f',.5))
        self.finish()
        self.assertEqual(self.read(self.ptrs[0]),(.5,)*self.n)
        self.assertEqual(self.read(self.ptrs[1]),(.25,)*self.n)
        self.reset(1) # previous block's private audio must not repeat
        self.assertEqual(self.read(self.ptrs[0]),(0.,)*self.n)
        self.assertEqual(self.read(self.ptrs[1]),(0.,)*self.n)

    def test_frame_boundaries_and_memory_guards(self):
        e=self.e
        guard=b'\xa5'*16
        for n in (1,31,256,1024):
            self.n=n;e.w32(OUTPUT+0x2c,n)
            for p in self.ptrs:e.uc.mem_write(p+n*4,guard)
            e.uc.mem_write(REGION+0x6040,guard)
            self.reset(1);self.mix(9,.25,-.125);self.finish()
            self.assertEqual(self.read(self.ptrs[0]),(.25,)*n)
            self.assertEqual(self.read(self.ptrs[1]),(-.125,)*n)
            for p in self.ptrs:self.assertEqual(bytes(e.uc.mem_read(p+n*4,len(guard))),guard)
            self.assertEqual(bytes(e.uc.mem_read(REGION+0x6040,len(guard))),guard)

    def test_fx_returns_share_private_routes(self):
        e=self.e; appfx=0x24020088+0x2fc0
        for row in (0,1):
            for dest,route in ((3,9),(4,10),(5,11)):
                self.reset(1)
                self.write(SL,[.25]*self.n);self.write(SR,[-.125]*self.n)
                e.uc.mem_write(SRC,struct.pack('<IIIIBBH',self.n,self.n,SL,SR,0,1,0))
                e.uc.mem_write(ITEMS+0x100,struct.pack('<HHi',0x148,0,dest))
                e.w32(appfx+row*0x18+4,ITEMS+0x100)
                e.uc.mem_write(appfx+row*0x18+10,struct.pack('<H',1))
                e.call('fxret_mix',ENGINE+0x150e4,SRC,ENGINE,row)
                self.assertEqual(e.r8(self.bus(route)+16),0)
                self.finish()
                target=(.25,-.125) if route==9 else ((.0625,0) if route==10 else (0,.0625))
                for p,x in zip(self.ptrs,target):self.assertEqual(self.read(p),(x,)*self.n)

    def test_registration_and_default(self):
        e=self.e; calls=[]
        def reg():
            sp=e.uc.reg_read(UC_ARM_REG_SP);n=e.r32(sp)
            calls.append((e.arg(1),[e.cstr(e.r32(e.arg(3)+i*4)) for i in range(n)],e.cstr(e.r32(sp+4))))
        e.stub(0x0808c0d8,reg)
        e.w32(STACK,9);e.w32(STACK+4,0x080cc1cc)
        e.call('phones_output_register',0,0x95,0x080cc1cc,0)
        self.assertEqual(calls[0],(0x95,['Out1','Out2','Out3','Out1L','Out1R','Out2L','Out2R','Out3L','Out3R','Phones stereo','Phones L','Phones R'],'outputbus'))
        e.call('phones_register',0)
        self.assertEqual(calls[1],(0x14b,['Monitor Mix','Independent'],'phonesmode'))
        defaults=[];e.stub(0x08093ef6,lambda:defaults.append((e.arg(1),e.arg(2))))
        e.call('phones_defaults',SETTINGS)
        self.assertEqual(defaults,[(0x14b,0)])

    def test_missing_allocation_and_invalid_block(self):
        e=self.e
        e.w32(0x38800f70,0);e.w32(0x38800f74,0x5052534d)
        self.assertEqual(self.bus(9),ENGINE+0x150e4)
        e.w32(0x38800f70,MASTER);e.w32(0x38800f74,MASTER^0x5052534d)
        e.w32(OUTPUT+0x2c,1025)
        e.call('phones_engine',ENGINE,OUTPUT,0)
        self.assertEqual(self.bus(9),ENGINE+0x150e4)

if __name__ == '__main__': unittest.main()
