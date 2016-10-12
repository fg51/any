#include <stdint.h>
#include <memory.h>

#if 0
#include <arm_math.h>
#else
typedef uint32_t q15_t;
#endif


#define I_FIR_STATE     ((q15_t*)0x10080000)
#define I_FIR_BUFFER    ((q15_t*)0x10080040)
#define Q_FIR_STATE     ((q15_t*)0x10081000)
#define Q_FIR_BUFFER    ((q15_t*)0x10081040)


#define NCO_SIN_TABLE		((int16_t*)0x1008F000)
#define NCO_COS_TABLE		((int16_t*)0x1008F800)


typedef struct cic_state_t {
    q15_t *dest;
    int16_t *nco_base;
    int32_t dest_idx;
    int32_t s0;
    int32_t s1;
    int32_t s2;
    int32_t d0;
    int32_t d1;
    int32_t d2;
    uint32_t dc_offset;
} cic_state_t;

cic_state_t cic_i;
cic_state_t cic_q;


void dsp_init(void);
static void cic_init(void);

void update_adc_dc_offset(void);
static uint32_t compute_adc_dc_offset(uint8_t *buf, int len);


void dsp_init(void)
{
    cic_init();
    //generate_test_tone(440);
}



static void cic_init(void)
{
    memset(&cic_i, 0, sizeof cic_i);
    cic_i.dest      = I_FIR_BUFFER;
    cic_i.nco_base  = NCO_SIN_TABLE;
    cic_i.dc_offset = 0x08800880;

    memset(&cic_q, 0, sizeof cic_q);
    cic_q.dest      = Q_FIR_BUFFER;
    cic_q.nco_base  = NCO_COS_TABLE;
    cic_q.dc_offset = 0x08800880;
}


#define CAPTUREBUFFER0      ((uint8_t*)0x20000000)
#define CAPTUREBUFFER1      ((uint8_t*)0x20008000)
#define CAPTUREBUFFER_SIZE  (0x10000)
#define CAPTUREBUFFER_SIZEHALF (0x8000)

void update_adc_dc_offset(void)
{
    uint32_t offset = compute_adc_dc_offset(CAPTUREBUFFER0, CAPTUREBUFFER_SIZEHALF);
    cic_i.dc_offset = offset;
    cic_q.dc_offset = offset;
}


static uint32_t compute_adc_dc_offset(uint8_t *buf, const int length)
{
    const uint16_t *const capture = (uint16_t*)buf;
    const int count = length / sizeof(uint16_t);

    uint32_t acc = 0;
    for (int i = 0; i < count; i++) {
        acc += capture[i];
    }
#if 0
    uint16_t offset = acc / count;
    //NOTE: place same 2 offset values on uint32_t
    return __PKHBT(offset, offset, 16);
#else
    uint32_t offset = acc / count;
    return (offset << 15) + offset;
#endif
}


