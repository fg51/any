#include <cstdint>

//{{{ dummy
typedef int32_t q31_t;
uint32_t __SMLAD(uint32_t x, uint32_t y, uint32_t z);
uint32_t __PKHBT(uint32_t x, uint32_t y, uint32_t z);
uint32_t __SSAT(uint32_t x, uint32_t y);
uint32_t __SMLADX(uint32_t x, uint32_t y, uint32_t z);
//}}} dummy


#include "fir_coeff.h"


#define I_FIR_STATE  ((q15_t*)0x10080000)
#define I_FIR_BUFFER ((q15_t*)0x10080040)
#define Q_FIR_STATE  ((q15_t*)0x10081000)
#define Q_FIR_BUFFER ((q15_t*)0x10081040)



#define DEMOD_BUFFER ((q15_t*)0x10088000)
#define DEMOD_BUFFER_SIZE (0x800)
//#define DEMOD_GAINBITS (6)    /* 0 ~ 6 */
#define DEMOD_GAINBITS (9)    /* 0 ~ 10 */


//__RAMFUNC(RAM)
void fir_filter_iq(void)
{
    const int32_t kLENGTH = FIR_BUFFER_SIZE / sizeof(uint32_t);

    const uint32_t *const coeff = (uint32_t*)fir_coeff;
    const uint32_t *in_i = (const uint32_t *)I_FIR_STATE;
    const uint32_t *in_q = (const uint32_t *)Q_FIR_STATE;
    uint32_t *dest = (uint32_t *)DEMOD_BUFFER;

    for (int i = 0; i < kLENGTH / 2; i++) {
        const q31_t kACC0_I = 0; const q31_t kACC0_Q = 0;
        const q31_t kACC1_I = 0; const q31_t kACC1_Q = 0;
        uint32_t in_i0 = in_i[0];
        uint32_t in_q0 = in_q[0];

        uint32_t acc0_i2; uint32_t acc0_q2;
        uint32_t acc1_i2; uint32_t acc1_q2;

        for (int j = 0; j < FIR_NUM_TAPS / 2; ) {
            const uint32_t kCOEFF0 = coeff[j++];
            const uint32_t kIN_I2 = in_i[j];
            const uint32_t kIN_Q2 = in_q[j];

            acc0_i2 = __SMLAD(in_i0, kCOEFF0, kACC0_I);
            acc0_q2 = __SMLAD(in_q0, kCOEFF0, kACC0_Q);

            acc1_i2 = __SMLADX(__PKHBT(kIN_I2, in_i0, 0), kCOEFF0, kACC1_I);
            acc1_q2 = __SMLADX(__PKHBT(kIN_Q2, in_q0, 0), kCOEFF0, kACC1_Q);

            in_i0 = kIN_I2;
            in_q0 = kIN_Q2;
        }

        dest[i*2] = __PKHBT(
                __SSAT((acc0_i2 >> 15), 16),
                __SSAT((acc0_q2 >> 15), 16),
                16);
        dest[i*2+1] = __PKHBT(
                __SSAT((acc1_i2 >> 15), 16),
                __SSAT((acc1_q2 >> 15), 16),
                16);
        in_i += 2;
        in_q += 2;
    }

    uint32_t *state_i = (uint32_t *)I_FIR_STATE;
    for (int i = 0; i < FIR_STATE_SIZE; i += 4) {
        //*state_i++ = *in_i++;
        __asm__ volatile (
                "ldr r0, [%0, %2]\n"
                "str r0, [%1, %2]\n"
                :: "l"(in_i),
                "l"(state_i),
                "X"(i): "r0"
        );
    }

    uint32_t *state_q = (uint32_t *)Q_FIR_STATE;
    for (int i = 0; i < FIR_STATE_SIZE; i += 4) {
        //*state_q++ = *in_q++;
        __asm__ volatile (
                "ldr r0, [%0, %2]\n"
                "str r0, [%1, %2]\n"
                :: "l"(in_q),
                "l"(state_q),
                "X"(i): "r0"
        );
    }
}
