/* Owner's quadrants/menu firmware recovered from the prior Codex chat,
 * with the recorded seven-wide revision reapplied. FREEPOET keypad driver
 * remains an owner-local dependency; see ../README.md for provenance.
 */
#include <Adafruit_TinyUSB.h>
#ifndef GRID_HOST_TEST
#include "keypad.h"
#endif
#include "grid_feedback.h"
Adafruit_USBD_MIDI midi;
Keypad grid;
GridFeedback feedback;
bool physical[128]={},held[128]={};
bool sent[4][128]={};
uint8_t desired[4][128]={};
bool showRootLabel=false;
bool menu=false,gestureLatched=false,waitingRelease=false,mountedBefore=false;
uint8_t root=0,scaleIndex=0,panicChannel=0;
uint32_t lastScan=0,lastLed=0,gestureStart=0,labelStart=0;
struct Scale {const char* name;uint8_t count;uint8_t notes[12];};
const Scale scales[]={
 {"MAJ",7,{0,2,4,5,7,9,11}},
 {"MIN",7,{0,2,3,5,7,8,10}},
 {"MAJ  P",5,{0,2,4,7,9}},
 {"MIN  P",5,{0,3,5,7,10}},
 {"CHROM",12,{0,1,2,3,4,5,6,7,8,9,10,11}},
 {"DORIAN",7,{0,2,3,5,7,9,10}},
 {"MIXO",7,{0,2,4,5,7,9,10}}
};
const char* roots[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
struct Glyph {char c; uint8_t small[5]; uint8_t tall[8];};
const Glyph font[]={
 {'T',{7,2,2,2,2},{7,7,2,2,2,2,2,2}},
 {'O',{2,5,5,5,2},{2,5,5,5,5,5,5,2}},
 {'C',{3,4,4,4,3},{3,4,4,4,4,4,4,3}},
 {'A',{2,5,7,5,5},{2,5,5,5,7,5,5,5}},
 {'U',{5,5,5,5,7},{5,5,5,5,5,5,5,7}},
 {'H',{5,5,7,5,5},{5,5,5,7,5,5,5,5}},
 {'P',{6,5,6,4,4},{6,5,5,6,4,4,4,4}},
 {'L',{4,4,4,4,7},{4,4,4,4,4,4,4,7}},
 {'Y',{5,5,2,2,2},{5,5,5,2,2,2,2,2}},
 {'I',{7,2,2,2,7},{7,2,2,2,2,2,2,7}},
 {'G',{3,4,5,5,3},{3,4,4,4,5,5,5,3}},
 {'J',{1,1,1,5,2},{1,1,1,1,1,1,5,2}},
 {'D',{6,5,5,5,6},{6,5,5,5,5,5,5,6}},
 {'E',{7,4,6,4,7},{7,4,4,6,4,4,4,7}},
 {'F',{7,4,6,4,4},{7,4,4,6,4,4,4,4}},
 {'B',{6,5,6,5,6},{6,5,5,6,5,5,5,6}},
 {'M',{5,7,7,5,5},{5,7,7,5,5,5,5,5}},
 {'N',{5,7,7,7,5},{5,7,7,7,5,5,5,5}},
 {'R',{6,5,6,5,5},{6,5,5,6,5,5,5,5}},
 {'S',{3,4,2,1,6},{3,4,4,2,1,1,1,6}},
 {'X',{5,5,2,5,5},{5,5,5,2,2,5,5,5}},
 {'#',{5,7,5,7,5},{5,7,5,5,7,5,5,5}},
 {' ',{0,0,0,0,0},{0,0,0,0,0,0,0,0}}
};

bool disabledKey(uint8_t i){return i/8==7 || i/8==8;}
uint8_t displayColumn(uint8_t x){return x<7?x:x+2;}
uint8_t channelFor(uint8_t i){return (i/8>=8?1:0)+(i%8>=4?2:0);}
uint8_t noteFor(uint8_t i){uint8_t col=i/8,x=col<7?col:col-9,y=(i%8)%4,degree=x+7*(3-y);const Scale &s=scales[scaleIndex];return 36+root+12*(degree/s.count)+s.notes[degree%s.count];}
bool message(uint8_t status,uint8_t a,uint8_t b){uint8_t p[]={uint8_t(status>>4),status,a,b};return midi.writePacket(p);}
bool allReleased(){for(bool v:physical)if(v)return false;return true;}
void silence(){for(auto &v:held)v=false;}
void toggleMenu(uint32_t now){menu=!menu;showRootLabel=false;silence();waitingRelease=true;labelStart=now;}
uint8_t glyphRow(char c,uint8_t row){for(auto &g:font)if(g.c==c)return g.small[row];return 0;}
void menuFrame(uint32_t now){
 for(uint8_t i=0;i<128;++i)grid.setPixelColor(i,0);
 for(uint8_t x=0;x<12;++x)grid.setPixelColor(displayColumn(x)*8,x==root?15:((x==1||x==3||x==6||x==8||x==10)?2:5));
 for(uint8_t x=0;x<7;++x)grid.setPixelColor(x*8+1,x==scaleIndex?15:4);
 grid.setPixelColor(120,4); // Return button, 4/15 brightness.
 char label[24];snprintf(label,sizeof(label),"%s",showRootLabel?roots[root]:scales[scaleIndex].name);
 int width=0;
 for(uint8_t l=0;label[l];++l){width+=label[l]==' '?1:3;if(label[l]!=' '&&label[l+1]&&label[l+1]!=' ')++width;}
 uint32_t elapsed=now-labelStart;
 // Hold the left-aligned beginning, scroll out, then pause before repeating.
 const uint32_t holdMs=1600, stepMs=110, gapMs=500;
 uint32_t phase=elapsed%(holdMs+uint32_t(width)*stepMs+gapMs);
 int origin=width<=14?0:-(phase<holdMs?0:int((phase-holdMs)/stepMs));
 int cursor=origin;
 for(uint8_t l=0;label[l];++l){
  if(label[l]==' '){++cursor;continue;}
  for(uint8_t y=0;y<5;++y)for(uint8_t c=0;c<3;++c){int x=cursor+c;if(x>=0&&x<14&&(glyphRow(label[l],y)&(4>>c)))grid.setPixelColor(displayColumn(x)*8+y+3,10);}
  cursor+=3;if(label[l+1]&&label[l+1]!=' ')++cursor;
 }
}
void setup(){
 TinyUSBDevice.setManufacturerDescriptor("OpenGrid");TinyUSBDevice.setProductDescriptor("OpenGrid Quadrants MIDI");TinyUSBDevice.setSerialDescriptor("opengrid-quads-01");
 Serial.begin(115200);midi.setStringDescriptor("OpenGrid Quadrants MIDI");midi.begin();TinyUSBDevice.detach();delay(20);TinyUSBDevice.attach();grid.begin();
}
void loop(){
 yield();uint32_t now=millis();bool mounted=TinyUSBDevice.mounted();
 if(mounted!=mountedBefore){feedback.reset();silence();for(auto &ch:sent)for(auto &v:ch)v=false;panicChannel=0;waitingRelease=true;mountedBefore=mounted;}
 uint8_t packet[4];for(uint8_t n=0;n<16&&midi.readPacket(packet);++n){feedback.receive(packet,now);}
 if(now-lastScan>=4){
  lastScan=now;grid.scanKeypad();
  for(uint8_t i=0;i<128;++i){if(disabledKey(i)){physical[i]=false;held[i]=false;continue;}if(grid.wasKeyPressed(i))physical[i]=true;if(grid.wasKeyReleased(i))physical[i]=false;}
  bool corners=physical[0]&&physical[7]&&physical[120]&&physical[127];
  if(corners){
   silence();
   if(!gestureLatched){if(!gestureStart)gestureStart=now;if(now-gestureStart>=500){toggleMenu(now);gestureLatched=true;}}
  }else{gestureStart=0;if(!physical[0]&&!physical[7]&&!physical[120]&&!physical[127])gestureLatched=false;}
  if(waitingRelease){silence();if(allReleased())waitingRelease=false;}
  else if(!corners){
   for(uint8_t i=0;i<128;++i){
    if(disabledKey(i))continue;
    if(menu){
     if(grid.wasKeyReleased(i) && !gestureLatched){
      uint8_t x=i/8,y=i%8;
      if(x==15&&y==0){toggleMenu(now);break;}
      uint8_t logicalX=x<7?x:x-2;
      if(y==0&&logicalX<12){root=logicalX;showRootLabel=true;labelStart=now;}
      if(y==1&&x<7){scaleIndex=x;showRootLabel=false;labelStart=now;}
     }
    }else{
     if(grid.wasKeyPressed(i))held[i]=true;
     if(grid.wasKeyReleased(i))held[i]=false;
    }
   }
  }
 }
 for(auto &ch:desired)for(auto &v:ch)v=0;
 if(!menu)for(uint8_t i=0;i<128;++i)if(!disabledKey(i)&&held[i])++desired[channelFor(i)][noteFor(i)];
 if(mounted){
  while(panicChannel<4){if(!message(0xB0|panicChannel,123,0))break;++panicChannel;}
  if(panicChannel==4){
   for(uint8_t ch=0;ch<4;++ch)for(uint8_t n=0;n<128;++n)if(sent[ch][n]&&!desired[ch][n]&&message(0x80|ch,n,0))sent[ch][n]=false;
   for(uint8_t ch=0;ch<4;++ch)for(uint8_t n=0;n<128;++n)if(!sent[ch][n]&&desired[ch][n]&&message(0x90|ch,n,100))sent[ch][n]=true;
  }
 }
 if(now-lastLed>=32){lastLed=now;
  if(menu)menuFrame(now);
  else if(feedback.active(now))for(uint8_t i=0;i<128;++i)grid.setPixelColor(i,disabledKey(i)?0:held[i]?15:feedback.level(i));
  else for(uint8_t i=0;i<128;++i)grid.setPixelColor(i,disabledKey(i)?0:held[i]?15:((noteFor(i)-root)%12==0?7:2));
  for(uint8_t y=0;y<8;++y){grid.setPixelColor(7*8+y,0);grid.setPixelColor(8*8+y,0);}
  grid.update();
 }
}

