/* Opt-in single-LED USB-A diagnostic, not a sequencer visualization.
 * Original work, MIT. Compiled out unless GRID_LED_DIAG=1.
 * Channel 16 / note 0 is reserved by this test; only attach the test grid.
 */
#ifdef GRID_LED_DIAG
#include <stdint.h>
#include "phones.h"
uint8_t *master_block(void);
struct grid_led_state { uint16_t blocks; uint8_t on, reserved; };
_Static_assert(sizeof(struct grid_led_state) == 4, "grid state reservation");
_Static_assert(GRID_LED_STATE_OFFSET + 4 <= PHONES_OFFSET + PHONES_BYTES, "grid state out of bounds");

int grid_led_append(uint8_t *dst, unsigned capacity, unsigned *bytes)
{
    uint8_t *mem = master_block();
    if (!mem || !dst || !bytes) return 0;
    struct grid_led_state *s = (struct grid_led_state *)(mem + GRID_LED_STATE_OFFSET);
    /* One attempt every ~0.5 s at 48 kHz / 128 frames per block. Hold a single
       pending transition while music fills the buffer; no unbounded queue. */
    if (s->blocks < 188) ++s->blocks;
    if (s->blocks < 188 || *bytes > capacity || (*bytes & 3u) || capacity - *bytes < 4)
        return 0;
    s->on = !s->on;
    uint8_t *p = dst + *bytes;
    p[0] = s->on ? 0x09 : 0x08;
    p[1] = s->on ? 0x9f : 0x8f;
    p[2] = 0;
    p[3] = s->on ? 127 : 0;
    *bytes += 4;
    s->blocks = 0;
    return 1;
}
#endif
