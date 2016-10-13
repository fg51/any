#include <stdint.h>


//{{{ dummy
typedef int16_t q15_t;
typedef int32_t q31_t;

#define CAPTUREBUFFER0 (0x0) // address
#define CAPTUREBUFFER1 (0x0) // address
#define CAPTUREBUFFER_SIZEHALF (0) // address
#define DECIMATION_RATIO (0)

/*  0x800(2048) = 0x10000 / 2 / 16 */
#define FIR_BUFFER_SIZE		0x800
#define FIR_STATE_SIZE		0x40
#define FIR_GAINBITS		5	/* 0 ~ 6 */

//#define RESAMPLE_BUFFER ((q15_t *)0x10089100)
extern q15_t *RESAMPLE_BUFFER;
#define RESAMPLE_BUFFER_SIZE (0x400)
#define RESAMPLE_GAINBITS (1)   /* 0 ~ 6 */
extern uint16_t *RESAMPLE_STATE;
extern uint16_t *RESAMPLE2_STATE;
#define RESAMPLE_STATE_SIZE (0x100)


typedef struct {
	uint16_t write_current;
	uint16_t write_total;
	uint16_t read_total;
	uint16_t read_current;
	uint16_t rebuffer_count;
} audio_state_t;

audio_state_t audio_state = {};

struct {
    int32_t index;
} resample_state;


extern uint32_t *DEMOD_BUFFER;
extern int16_t *RESAMPLE_BUFFER;
extern int16_t *RESAMPLE2_BUFFER;
extern uint32_t DEMOD_BUFFER_SIZE;


typedef struct LPC_GPDMA_t {
  uint32_t INTERRCLR;
  uint32_t INTERRSTAT;
  uint32_t INTTCCLEAR;
  uint32_t INTTCSTAT;
} LPC_GPDMA_t;
//LPC_GPDMA_t LPC_GPDMA_dummy = {};
//LPC_GPDMA_t *LPC_GPDMA = &LPC_GPDMA_dummy;
extern LPC_GPDMA_t *LPC_GPDMA;

extern void TESTPOINT_ON(void);
extern void TESTPOINT_SPIKE(void);
extern void TESTPOINT_OFF(void);

extern void LED_ON(void);
extern void LED_OFF(void);

extern uint16_t *AUDIO_BUFFER;
extern uint32_t AUDIO_BUFFER_SIZE;

extern uint32_t __SSUB16(uint32_t x, uint32_t offset);
extern uint32_t __SSAT(uint32_t x, uint32_t f);
extern uint32_t __PKHBT(uint32_t x, uint32_t y, uint32_t z);
extern uint32_t __SMLAD(uint32_t x, uint32_t f, uint32_t s0);
extern uint32_t __SMLADX(uint32_t x, uint32_t f, uint32_t s0);

extern void arm_sqrt_q31(int32_t inn, int32_t *out);

//}}} dummy


//{{{ external
#define NCO_CYCLE (1024)
#define NCO_SAMPLES (1024)
#define NCO_COS_OFFSET (NCO_CYCLE / 4)



typedef struct {
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

cic_state_t cic_i = {};
cic_state_t cic_q = {};


extern uint32_t capture_count;
//}}} external

//{{{ prot-type functions
static void cic_decimate(cic_state_t *cic, uint8_t *buf, int len);
static void fir_filter_iq(void);
static void demod_distribute(void);
static void resample_fir_filter_amdemod(void);

//}}} prot-type functions


//__RAMFUNC(RAM)
void DMA_IRQHandler (void)
{
  if (LPC_GPDMA->INTERRSTAT & 1) {
    LPC_GPDMA->INTERRCLR = 1;
  }

  if (LPC_GPDMA->INTTCSTAT & 1) {
    LPC_GPDMA->INTTCCLEAR = 1;

    TESTPOINT_ON();
    if ((capture_count & 1) == 0) {
        cic_decimate(&cic_i, CAPTUREBUFFER0, CAPTUREBUFFER_SIZEHALF);
        cic_decimate(&cic_q, CAPTUREBUFFER0, CAPTUREBUFFER_SIZEHALF);
    } else {
        cic_decimate(&cic_i, CAPTUREBUFFER1, CAPTUREBUFFER_SIZEHALF);
        cic_decimate(&cic_q, CAPTUREBUFFER1, CAPTUREBUFFER_SIZEHALF);
    }

    TESTPOINT_SPIKE();
    fir_filter_iq();
    TESTPOINT_SPIKE();

	fm_demod();
	TESTPOINT_SPIKE();
#if STEREO
	stereo_separate();
	//TESTPOINT_SPIKE();
	//stereo_matrix();
	TESTPOINT_SPIKE();
	resample_fir_filter_stereo();
#else
	resample_fir_filter();
#endif

    //#if 1
    //demod_distribute();
    //TESTPOINT_SPIKE();
    //resample_fir_filter_amdemod();
    //#endif

    TESTPOINT_OFF();
    capture_count ++;

    { //HALT_DMA(); // halt DMA for inspecting contents of buffer
        // toggle LED with every 1024 interrupts
        const int cout = capture_count % 1024;
        if (cout == 0) {
            LED_ON();   //set_event_LED1_turn_on();
        } else if (cout == 512) { //NOTE: 512 (= 1024 / 2)
            LED_OFF();  //set_event_LED1_turn_off();
        }
    }
  }


#if 0
  {
    static int first = true;
    if (first) {
        pwmout_setupdma(
                (uint16_t*)AUDIO_BUFFER,
                AUDIO_BUFFER_SIZE / sizeof(uint16_t)
                );
        first = false;
    }
  }
#endif
}


//__RAMFUNC(RAM)
void cic_decimate(cic_state_t *cic, uint8_t *buf, int len)
{
    /*
     * update cic->s0, s1, s2, d0, d1, d2
    */
    const uint32_t offset = cic->dc_offset;
    int16_t *const dest = (int16_t*)cic->dest;
    uint32_t *const capture = (uint32_t*)buf;
    const uint32_t *const nco_base = (uint32_t*)cic->nco_base;

    int32_t s0 = cic->s0; int32_t s1 = cic->s1; int32_t s2 = cic->s2;
    int32_t d0 = cic->d0; int32_t d1 = cic->d1; int32_t d2 = cic->d2;

    int dest_idx = cic->dest_idx;
    for (int i = 0; i < len / 4; ) {
        for (int j = 0; j < NCO_SAMPLES / 2; ) {
            for (int k = 0; k < DECIMATION_RATIO / 2; k++, j++) {
                uint32_t x = capture[i++];
                uint32_t f = nco_base[j];
                x  = __SSUB16(x, offset);
                s0 = __SMLAD(x, f, s0);
                s1 += s0;
                s2 += s1;
            }
            int32_t e0 = d0 - s2;
            d0 = s2;

            int32_t e1 = d1 - e0;
            d1 = e0;

            int32_t e2 = d2 - e1;
            d2 = e1;

            dest[dest_idx++] = __SSAT(e2 >> (16 - FIR_GAINBITS), 16);
            dest_idx %=  FIR_BUFFER_SIZE / 2;
        }
    }
    cic->dest_idx = dest_idx;
    cic->s0 = s0; cic->s1 = s1; cic->s2 = s2;
    cic->d0 = d0; cic->d1 = d1; cic->d2 = d2;
}


extern uint32_t *fir_coeff;
extern uint32_t *I_FIR_STATE;
extern uint32_t *Q_FIR_STATE;

#include "fir_coeff.h"

//__RAMFUNC(RAM)
static void fir_filter_iq(void)
{
    const uint32_t *coeff = (uint32_t*)fir_coeff;
    const uint32_t *in_i = (const uint32_t *)I_FIR_STATE;
    const uint32_t *in_q = (const uint32_t *)Q_FIR_STATE;
    const int32_t kLENGTH = FIR_BUFFER_SIZE / sizeof(uint32_t);
    uint32_t *dest = (uint32_t *)DEMOD_BUFFER;

    for (int i = 0; i < kLENGTH / 2; i++) {
        q31_t acc0_i = 0; q31_t acc0_q = 0;
        q31_t acc1_i = 0; q31_t acc1_q = 0;
        uint32_t x0 = in_i[0];
        uint32_t y0 = in_q[0];

        for (int j = 0; j < FIR_NUM_TAPS / 2; ) {
            uint32_t c0 = coeff[j++];

            uint32_t x2 = in_i[j];
            uint32_t y2 = in_q[j];
            acc0_i = __SMLAD(x0, c0, acc0_i);
            acc0_q = __SMLAD(y0, c0, acc0_q);
            acc1_i = __SMLADX(__PKHBT(x2, x0, 0), c0, acc1_i);
            acc1_q = __SMLADX(__PKHBT(y2, y0, 0), c0, acc1_q);
            x0 = x2;
            y0 = y2;
        }

        dest[i*2]   = __PKHBT(
                __SSAT((acc0_i >> 15), 16),
                __SSAT((acc0_q >> 15), 16),
                16);
        dest[i*2+1] = __PKHBT(
                __SSAT((acc1_i >> 15), 16),
                __SSAT((acc1_q >> 15), 16),
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
                :: "l"(in_i), "l"(state_i), "X"(i): "r0");
    }
    uint32_t *state_q = (uint32_t *)Q_FIR_STATE;
    for (int i = 0; i < FIR_STATE_SIZE; i += 4) {
        //*state_q++ = *in_q++;
        __asm__ volatile (
                "ldr r0, [%0, %2]\n"
                "str r0, [%1, %2]\n"
                :: "l"(in_q), "l"(state_q), "X"(i): "r0");
    }
}



#include "fir_coeff.h"


struct {
	uint32_t last;
	int32_t carrier;
} fm_demod_state;



//__RAMFUNC(RAM)
