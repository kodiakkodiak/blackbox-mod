/*
 * Experimental fourth stereo output: original Blackbox, stock 3.1.9.
 * Hardware independence is ASSUMED, not verified. See research/PHONES_ROUTING.md.
 *
 * Only the two pad-process bus lookups are intercepted: values 9..11 must NEVER
 * enter stock's bus table, where 9 is a recording bus and 10/11 are send returns.
 * The appended pad values use three private stock-compatible buffer objects.
 * FX return destination 3..5 uses those same objects. No stock enum is renumbered.
 *
 * The engine wrapper clears the private buses BEFORE running the engine, then
 * adds them to Phones AFTER stock makes its normal monitor sum, BEFORE the outer
 * output gain and converter. In Independent mode it replaces the monitor sum.
 * Private buses bypass Out1's master FX/compressor/clipper. Pad inserts/sends and
 * the existing pad level/pan processing run normally. L/R routes fold stereo to
 * mono at half gain per side; mono is passed at unity.
 */
#include "phones.h"
#define FN(a) ((a) | 1u)
#define MAX_FRAMES 1024
#define MAGIC 0x50484f4eu
#define SETTINGS ((const void *)(0x24020088u + 0x8c90u))
uint8_t *master_block(void);
typedef int (*setting_fn)(const void *, unsigned);
typedef void *(*bus_fn)(void *, int);
int duck_block(uint8_t *engine, void *outputs, void *position);
typedef void (*reg_fn)(void *, int, const char *, const char *const *, int, const char *);
typedef void (*add_fn)(void *, int, int);
#define fw_update ((add_fn)FN(0x08093ecc))
#define fw_setting ((setting_fn)FN(0x08093e9c))
#define fw_bus ((bus_fn)FN(0x0805f37c))
#define fw_reg ((reg_fn)FN(0x0808c0d8))
#define fw_add ((add_fn)FN(0x08093ef6))
struct buffer { int cap, len; float *l, *r; uint8_t silent, stereo; uint16_t pad; };
struct state { uint32_t magic; struct buffer bus[3]; float samples[3][2][MAX_FRAMES]; };
_Static_assert(sizeof(struct state) <= PHONES_BYTES, "Phones memory exceeds reserved region");
_Static_assert(sizeof(struct buffer) == 20, "stock buffer ABI");
static struct state *state(void)
{
    uint8_t *m = master_block();
    return m ? (struct state *)(m + PHONES_OFFSET) : 0;
}
void phones_boot(uint8_t *master)
{
    if (master)
        ((struct state *)(master + PHONES_OFFSET))->magic = 0;
}
void phones_register(void *table)
{
    static const char *const modes[] = {"Monitor Mix", "Independent"};
    fw_reg(table, P_PHONES_MODE, "Phones:", modes, 2, "phonesmode");
}
void phones_defaults(void *store) { fw_add(store, P_PHONES_MODE, 0); }
/* Keep stock's nine labels verbatim and append new values to the SAME parameter. */
void phones_output_register(void *table, int param, const char *label, const char *const *old, int count, const char *xml)
{
    (void)old;
    (void)count;
    static const char *const names[] = {"Out1", "Out2", "Out3", "Out1L", "Out1R", "Out2L", "Out2R", "Out3L", "Out3R",
                                       "Phones stereo", "Phones L", "Phones R"};
    fw_reg(table, param, label, names, 12, xml);
}
/* XML 10/11 are legacy Out1+FX shortcuts. New XML uses 12..14; runtime/UI
   still uses appended menu indices 9..11. Only the nonlegacy loader path hooks here. */
void phones_output_load(void *store, int param, int value)
{
    if (value >= 12 && value <= 14) value -= 3;
    fw_update(store, param, value);
}
void *phones_pad_bus(void *bufs, int destination)
{
    if (destination >= 9 && destination <= 11) {
        struct state *s = state();
        if (s && s->magic == MAGIC)
            return &s->bus[destination - 9];
        /* Permanent-allocation failure: match the documented fallback to Out1. */
        destination = 0;
    }
    return fw_bus(bufs, destination);
}
static void begin(int frames)
{
    struct state *s = state();
    if (!s) return;
    if (frames <= 0 || frames > MAX_FRAMES) { s->magic = 0; return; }
    for (int k = 0; k < 3; k++) {
        s->bus[k].cap = frames;
        s->bus[k].len = 0;
        s->bus[k].l = s->samples[k][0];
        s->bus[k].r = s->samples[k][1];
        s->bus[k].silent = 1;
        s->bus[k].stereo = 0;
        s->bus[k].pad = 0;
    }
    s->magic = MAGIC;
}
void phones_finish(void *engine, void *outputs)
{
    struct state *s = state();
    float **out = (float **)outputs;
    int n = *(int *)((uint8_t *)outputs + 0x2c);
    if (!s || s->magic != MAGIC || n <= 0 || n > MAX_FRAMES)
        return;
    if (fw_setting(SETTINGS, P_PHONES_MODE) == 1)
        for (int i = 0; i < n; i++) out[0][i] = out[1][i] = 0.0f;
    /* Same engine-side Phones gain as stock's final mix (0x08053afa/0x08053b0a).
       The outer block's smoothed per-channel gains still run after this hook. */
    float gain = *(float *)((uint8_t *)engine + 0x28d64);
    for (int k = 0; k < 3; k++) {
        struct buffer *b = &s->bus[k];
        if (b->silent) continue;
        int count = b->len ? b->len : b->cap;
        if (count > n) count = n;
        for (int i = 0; i < count; i++) {
            float l = b->l[i], r = b->stereo ? b->r[i] : l;
            if (k == 0) {
                out[0][i] += l * gain;
                out[1][i] += r * gain;
            } else {
                out[k - 1][i] += (l + r) * (0.5f * gain);
            }
        }
    }
}
int phones_engine(void *engine, void *outputs, void *position)
{
    begin(*(int *)((uint8_t *)outputs + 0x2c));
    int result = duck_block(engine, outputs, position);
    if (result) phones_finish(engine, outputs);
    return result;
}
