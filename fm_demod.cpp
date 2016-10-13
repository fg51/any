#include <cstdint>
#include <cstdbool>

typedef int16_t q15_t;

int32_t __SMUAD(int32_t x, int32_t y);
int32_t __SMUSDX(int32_t x, int32_t y);
uint16_t __SSAT(uint16_t x, uint16_t y);


#define DEMOD_BUFFER 		((q15_t*)0x10088000)
#define DEMOD_BUFFER_SIZE	0x800
//#define DEMOD_GAINBITS		6	/* 0 ~ 6 */
#define DEMOD_GAINBITS		9	/* 0 ~ 10 */

#define RESAMPLE_STATE 		((q15_t*)0x10089000)
#define RESAMPLE_STATE_SIZE	0x100
#define RESAMPLE_BUFFER 	((q15_t*)0x10089100)
#define RESAMPLE_BUFFER_SIZE 0x400
#define RESAMPLE_GAINBITS	1	/* 0 ~ 6 */

#define Q15_PI_4 (25736)    // 3.14159 / 4 * 32768


typedef struct fm_demod_state_t {
    uint32_t last;
    int32_t carrier;
} fm_demod_state_t;

static fm_demod_state_t fm_demod_state;


#include "arctan_table.h"


/*
 * update fm_demod_state{.last, .carrier,}
 * update RESAMPLE_BUFFER via DEMOD_BUFFER
 */
void fm_demod(void)
{
    //NOTE: update fm_demod_state.carrier
    uint32_t last = fm_demod_state.last;
    fm_demod_state.carrier = __SMUAD(last, last) >> DEMOD_GAINBITS;

    //NOTE: update fm_demod_state.last
    const int32_t kLENGTH = DEMOD_BUFFER_SIZE / sizeof(uint32_t);
    const uint32_t *const src = (uint32_t *)DEMOD_BUFFER;
    int16_t *const dest = (int16_t *)RESAMPLE_BUFFER;

    for (int i = 0; i < kLENGTH; i++) {
        uint32_t next = src[i];
#if 1
        //{{{ gen d, is_negative, d
        int32_t real = __SMUAD(next, last);   // I0 * I1 + Q0 * Q1
        int32_t imag = __SMUSDX(next, last);  // I0 * Q1 - I1 * Q0
        int32_t angle = 0;
        uint8_t is_negative = false;
        if (real < 0) {
            real = -real;
            is_negative = !is_negative;
            angle += -Q15_PI_4 * 4;
        }
        if (imag < 0) {
            imag = -imag;
            is_negative = !is_negative;
        }
        if (imag >= real) {
            int32_t tmp_swap = imag;
            imag = real;
            real = tmp_swap;
            is_negative = !is_negative;
            angle = -angle - Q15_PI_4 * 2;
        }
#if 1
        uint32_t d = imag << 0;
        d /= real >> 16;
#else
        float32_t x = (float32_t)im * 65536;
        d = x / re;
#endif
        //}}} gen d, is_negative, d

        // whats d? f?
        int idx = (d >> 8) & 0xff;
        uint32_t f = d & 0xff;
        const int32_t kARC_TAN1 = arctantbl[idx];
        const int32_t kARC_TAN2 = arctantbl[idx+1];
        angle += kARC_TAN1 + (((kARC_TAN2 - kARC_TAN1) * f) >> 8);
        if (is_negative) {
            angle = -angle;
        }
        dest[i] = __SSAT(angle / 16, 16);
#endif
        last = next;
    }
    fm_demod_state.last = last;
}


