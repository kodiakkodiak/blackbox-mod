/* Experimental original-Blackbox PHONES routing. MIT; see repository LICENSE. */
#ifndef PHONES_H
#define PHONES_H
#include <stdint.h>
#define P_PHONES_MODE 0x14b
#define PHONES_OFFSET (0x56800u + 0x3e120u)
#define PHONES_BYTES 0x6100u
void phones_register(void *table);
void phones_defaults(void *store);
void phones_boot(uint8_t *master);
void *phones_pad_bus(void *bufs, int destination);
void phones_finish(void *engine, void *outputs);
#endif
