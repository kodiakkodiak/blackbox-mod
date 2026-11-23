/* Standalone PHONES diagnostic for original Blackbox 3.1.9.
 * MIT, retaining Justin Joe's repository license and attribution.
 * Fixed -30.1 dBFS peak tones, AFTER stock headphone gain. Start recording
 * equipment with low input gain. This test has no normal headphone monitor.
 * No RAM state, allocation, bank-2 code or firmware register changes.
 */
#include <stdint.h>
typedef void (*convert_fn)(const float *, const float *, const float *, int, uint32_t *, int);
#define CONVERT ((convert_fn)(0x0806002cu | 1u))
static const float tones[2][32] = {
    {0.000000000f, 0.006096573f, 0.011958857f, 0.017361570f, 0.022097087f, 0.025983425f, 0.028871235f, 0.030649540f, 0.031250000f, 0.030649540f, 0.028871235f, 0.025983425f, 0.022097087f, 0.017361570f, 0.011958857f, 0.006096573f, 0.000000000f, -0.006096573f, -0.011958857f, -0.017361570f, -0.022097087f, -0.025983425f, -0.028871235f, -0.030649540f, -0.031250000f, -0.030649540f, -0.028871235f, -0.025983425f, -0.022097087f, -0.017361570f, -0.011958857f, -0.006096573f},
    {0.000000000f, 0.011958857f, 0.022097087f, 0.028871235f, 0.031250000f, 0.028871235f, 0.022097087f, 0.011958857f, 0.000000000f, -0.011958857f, -0.022097087f, -0.028871235f, -0.031250000f, -0.028871235f, -0.022097087f, -0.011958857f, -0.000000000f, 0.011958857f, 0.022097087f, 0.028871235f, 0.031250000f, 0.028871235f, 0.022097087f, 0.011958857f, 0.000000000f, -0.011958857f, -0.022097087f, -0.028871235f, -0.031250000f, -0.028871235f, -0.022097087f, -0.011958857f}
};
static void run(const float *b, const float *c, int frames, uint32_t *dst, int stride, unsigned side)
{
    /* Stock's frames are multiples of 32. Each call starts at phase zero.
     * b/c and the original conversion remain untouched. */
    while (frames > 0) {
        int n = frames > 32 ? 32 : frames;
        CONVERT(tones[side], b, c, n, dst, stride);
        b += n;
        c += n;
        dst += n * stride;
        frames -= n;
    }
}
void phones_diag_left(const float *a, const float *b, const float *c, int frames, uint32_t *dst, int stride)
{
    (void)a;
    run(b, c, frames, dst, stride, 0);
}
void phones_diag_right(const float *a, const float *b, const float *c, int frames, uint32_t *dst, int stride)
{
    (void)a;
    run(b, c, frames, dst, stride, 1);
}
