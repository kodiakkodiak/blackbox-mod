#ifndef GRID_TEST_HARDWARE_H
#define GRID_TEST_HARDWARE_H
#include "Adafruit_TinyUSB.h"
#define KEYPAD_H
class Keypad {
public:
 bool press[128]={},release[128]={};
 uint8_t pixels[128]={};
 unsigned updates=0;
 bool begin(){return true;}
 void scanKeypad(){}
 bool wasKeyPressed(uint8_t i)const{return press[i];}
 bool wasKeyReleased(uint8_t i)const{return release[i];}
 void setPixelColor(uint8_t i,uint8_t v){pixels[i]=v;}
 void update(){++updates;}
};
#endif
