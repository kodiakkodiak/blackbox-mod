#ifndef GRID_TEST_USB_H
#define GRID_TEST_USB_H
#include <stdint.h>
#include <stdio.h>
#include <deque>
#include <array>
#include <vector>
extern uint32_t testNow;
inline uint32_t millis(){return testNow;}
inline void delay(unsigned ms){testNow+=ms;}
inline void yield(){}
struct TestUSB {
 bool connected=true;
 void setManufacturerDescriptor(const char*){}
 void setProductDescriptor(const char*){}
 void setSerialDescriptor(const char*){}
 bool mounted(){return connected;}
 void detach(){connected=false;}
 void attach(){connected=true;}
};
extern TestUSB TinyUSBDevice;
struct TestSerial {void begin(unsigned){}};
extern TestSerial Serial;
class Adafruit_USBD_MIDI {
public:
 std::deque<std::array<uint8_t,4>> incoming;
 std::vector<std::array<uint8_t,4>> outgoing;
 bool writable=true;
 void setStringDescriptor(const char*){}
 void begin(){}
 bool readPacket(uint8_t *p){if(incoming.empty())return false;auto v=incoming.front();incoming.pop_front();for(unsigned i=0;i<4;++i)p[i]=v[i];return true;}
 bool writePacket(uint8_t *p){if(!writable)return false;outgoing.push_back({p[0],p[1],p[2],p[3]});return true;}
};
#endif
