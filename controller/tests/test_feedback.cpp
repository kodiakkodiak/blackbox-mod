#include "mocks/hardware.h"
#include "../opengrid/src/grid_feedback.h"
#include <cassert>
#include <cstdio>
uint32_t testNow=0;
TestUSB TinyUSBDevice;
TestSerial Serial;
extern Adafruit_USBD_MIDI midi;
extern Keypad grid;
extern GridFeedback feedback;
extern uint8_t root,scaleIndex;
extern bool menu,physical[128],held[128];
void setup();void loop();
uint8_t channelFor(uint8_t);uint8_t noteFor(uint8_t);bool disabledKey(uint8_t);
static void step(unsigned ms=4){testNow+=ms;loop();for(unsigned i=0;i<128;++i)grid.press[i]=grid.release[i]=false;}
static void receive(uint8_t cin,uint8_t status,uint8_t note,uint8_t velocity){midi.incoming.push_back({cin,status,note,velocity});step(32);}
int main(){
 GridFeedback f;
 const uint8_t on[]={9,0x9f,0,127},off[]={8,0x8f,0,0};
 assert(!f.active(0));assert(f.receive(on,100));assert(f.active(100));assert(f.level(0)==15);
 assert(f.receive(off,101));assert(f.level(0)==0);
 for(unsigned ch=0;ch<15;++ch){uint8_t p[]={9,uint8_t(0x90|ch),0,127};assert(!f.receive(p,102));}
 for(unsigned column: {7u,8u})for(unsigned y=0;y<8;++y){uint8_t p[]={9,0x9f,uint8_t(column*8+y),127};assert(!f.receive(p,103));}
 for(unsigned value=0;value<128;++value){uint8_t p[]={9,0x9f,8,uint8_t(value)};assert(f.receive(p,200));assert(f.level(8)==(value*15+126)/127);}
 for(auto p: {std::array<uint8_t,4>{0x19,0x9f,0,127},{9,0x8f,0,127},{8,0x9f,0,127},{9,0x9f,128,127},{9,0x9f,0,128},{15,0xf8,0,0}})assert(!f.receive(p.data(),201));
 assert(!f.active(3200));assert(f.receive(on,0xfffffff0u));assert(f.active(20));assert(!f.active(0x1000));
 f.reset();assert(!f.active(0));assert(f.level(0)==0);
 puts("PASS receiver: channel/cable/status filtering, brightness, center gap, timeout and timer wrap");
 setup();step();step(32);assert(!menu);
 for(unsigned scale=0;scale<7;++scale)for(unsigned r=0;r<12;++r){scaleIndex=scale;root=r;for(unsigned q=0;q<4;++q){bool seen[128]={};unsigned count=0;for(unsigned i=0;i<128;++i)if(!disabledKey(i)&&channelFor(i)==q){auto n=noteFor(i);assert(n<128&&!seen[n]);seen[n]=true;++count;}assert(count==28);}}
 root=scaleIndex=0;
 for(unsigned i: {56u,57u,63u,64u,65u,71u})grid.press[i]=true;
 step(32);for(unsigned i: {56u,57u,63u,64u,65u,71u})assert(!physical[i]&&!held[i]&&grid.pixels[i]==0);
 for(unsigned i: {3u,75u,7u,79u}){ // one key in each of the four quadrants
  auto ch=channelFor(i),note=noteFor(i);midi.outgoing.clear();grid.press[i]=true;step();
  assert(!midi.outgoing.empty());auto p=midi.outgoing.back();assert(p[1]==(0x90|ch)&&p[2]==note&&p[3]==100);
  grid.release[i]=true;step();p=midi.outgoing.back();assert(p[1]==(0x80|ch)&&p[2]==note);
 }
 puts("PASS existing keyboard: seven-wide unique notes, four channels, note release and disabled keys");
 auto sent=midi.outgoing.size();receive(9,0x9f,0,127);
 assert(feedback.active(testNow)&&grid.pixels[0]==15);
 for(unsigned i=1;i<128;++i)assert(grid.pixels[i]==0);
 receive(8,0x8f,0,0);for(auto v:grid.pixels)assert(v==0);
 assert(midi.outgoing.size()==sent); // receiving LED feedback must never echo a note
 receive(9,0x90,0,127);assert(grid.pixels[0]==0);
 step(3100);assert(!feedback.active(testNow)&&grid.pixels[0]>0);
 puts("PASS integrated USB feedback: one LED on/off, no echo, music channel ignored, keyboard view restored");
 receive(9,0x9f,0,127);
 for(unsigned i:{0u,7u,120u,127u})grid.press[i]=true;step();step(600);step(32);
 assert(menu);for(unsigned y=0;y<8;++y)assert(grid.pixels[56+y]==0&&grid.pixels[64+y]==0);
 for(unsigned i:{0u,7u,120u,127u})grid.release[i]=true;step();
 grid.press[120]=true;step();grid.release[120]=true;step();assert(!menu);
 TinyUSBDevice.connected=false;step(32);assert(!feedback.active(testNow));
 puts("PASS menu and disconnect: menu remains usable, center gap stays dark, feedback clears");
}
