/* Original feedback receiver, MIT; see repository LICENSE.
 * USB-MIDI cable 0, channel 16; note = physical x*8+y, velocity = brightness.
 * Input is display-only. No MIDI transmission, heap use or blocking calls here.
 */
#ifndef OPEN_GRID_FEEDBACK_H
#define OPEN_GRID_FEEDBACK_H
#include <stdint.h>
class GridFeedback {
public:
    static constexpr uint32_t kTimeoutMs = 3000;
    void reset() {
        for (auto &v : pixels_) v = 0;
        armed_ = false;
        last_ = 0;
    }
    bool receive(const uint8_t p[4], uint32_t now) {
        if (p[0] >> 4 || (p[1] & 15) != 15 || p[2] >= 128 || p[3] >= 128)
            return false;
        const uint8_t cin = p[0] & 15, status = p[1] & 0xf0;
        if (!((cin == 9 && status == 0x90) || (cin == 8 && status == 0x80)))
            return false;
        const uint8_t column = p[2] / 8;
        if (column == 7 || column == 8) return false;
        if (!active(now)) {
            for (auto &v : pixels_) v = 0;
        }
        const uint8_t velocity = status == 0x80 ? 0 : p[3];
        pixels_[p[2]] = uint8_t((unsigned(velocity) * 15u + 126u) / 127u);
        last_ = now;
        armed_ = true;
        return true;
    }
    bool active(uint32_t now) const { return armed_ && uint32_t(now - last_) < kTimeoutMs; }
    uint8_t level(uint8_t key) const { return key < 128 ? pixels_[key] : 0; }
private:
    uint8_t pixels_[128] = {};
    uint32_t last_ = 0;
    bool armed_ = false;
};
#endif
