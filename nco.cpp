#include <cstdint>
#include <limits.h>

#define PI (3.14)

#define AUDIO_RATE (48000)
#define IF_RATE    (13 * AUDIO_RATE / 2)
#define CIC_DECIMATION_RATIO (16)
#define FIR_DECIMATION_RATIO (2)
#define ADC_RATE (CIC_DECIMATION_RATIO * FIR_DECIMATION_RATIO * IF_RATE)

//{{{ dummy
typedef float float32_t;
float32_t arm_cos_32t();
//}}} dummy


#define NCO_SIN_TABLE ((int16_t*)0x1008F000)
#define NCO_COS_TABLE ((int16_t*)0x1008F800)
#define NCO_TABLE_SIZE (0x800)
#define NCO_SAMPLES (1024)
//#define NCO_AMPL (32)
//#define NCO_AMPL (64)
#define NCO_AMPL (SHRT_MAX / 128)
//#define NCO_AMPL (SHRT_MAX / 64)
//#define NCO_AMPL (SHRT_MAX / 32)
//#define NCO_AMPL (SHRT_MAX / 16)
//#define NCO_AMPL (SHRT_MAX / 4)

#define NCO_CYCLE (1024)
//#define NCO_SAMPLES (1024)
#define NCO_COS_OFFSET (NCO_CYCLE/4)


//void nco_set_frequency(float32_t freq) __RAMFUNC(RAM)

//__RAMFUNC(RAM)
void nco_set_frequency(float32_t freq)
{
    int16_t *costbl = NCO_SIN_TABLE;
    int16_t *sintbl = NCO_COS_TABLE;
    int f;
    int i;

    freq -= (int)(freq / ADC_RATE) * ADC_RATE;
    f = (int)(freq / ADC_RATE * NCO_CYCLE);
    for (i = 0; i < NCO_SAMPLES; i++) {
        float32_t phase = 2*PI*f*(i+0.5)/NCO_CYCLE;
        costbl[i] = (int16_t)(arm_cos_f32(phase) * NCO_AMPL);
        sintbl[i] = (int16_t)(arm_sin_f32(phase) * NCO_AMPL);
    }
}
