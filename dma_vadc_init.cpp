#include "driver/dma_vadc.h"


#include <lpc43xx_cgu.h>
//using cgu::CGU_CLKSRC_PLL0_AUDIO;
//using cgu::CGU_BASE_ENET_CSR;

//#include <lpc_types.h>
//using cgu::ENABLE;


#include <lpc43xx_rgu.h> // add RGU_SIG_VADC in RGU_SIG
//using rgu::RGU_SoftReset;
//using rgu::RGU_GetSignalStatus;
//using rgu::RGU_SIG_DAC;


// in vadc_sample.h
#define CGU_BASE_VADC CGU_BASE_ENET_CSR



#include <LPC43xx.h>
////using lpc43xx::RESERVED7_IRQn;
//using lpc43xx::RESERVED7_IRQn; //= 45, /*!<  45  VADC */

#define VADC_IRQn RESERVED7_IRQn



typedef struct {  /*!< (@ 0x400F0000) VADC Structure         */
    __O  uint32_t FLUSH;      /*!< (@ 0x400F0000) Flushes FIFO */
    __IO uint32_t DMA_REQ;    /*!< (@ 0x400F0004) Set or clear DMA write request */
    __I  uint32_t FIFO_STS;   /*!< (@ 0x400F0008) Indicates FIFO fullness status */
    __IO uint32_t FIFO_CFG;   /*!< (@ 0x400F000C) Configures FIFO fullness level that triggers interrupt and packing 1 or 2 samples per word. */
    __O  uint32_t TRIGGER;    /*!< (@ 0x400F0010) Enable software trigger to start descriptor processing */
    __IO uint32_t DSCR_STS;   /*!< (@ 0x400F0014) Indicates active descriptor table and descriptor entry */
    __IO uint32_t POWER_DOWN; /*!< (@ 0x400F0018) Set or clear power down mode */
    __IO uint32_t CONFIG;     /*!< (@ 0x400F001C) Configures external trigger mode, store channel ID in FIFO and wakeup recovery time from power down. */
    __IO uint32_t THR_A; /*!< (@ 0x400F0020) Configures window comparator A levels. */
    __IO uint32_t THR_B; /*!< (@ 0x400F0024) Configures window comparator B levels. */
    __I  uint32_t LAST_SAMPLE[6];  /*!< (@ 0x400F0028)	Contains last converted sample of input M [M=0..5) and result of window comparator. */
    __I  uint32_t RESERVED0[48];
    __IO uint32_t ADC_DEBUG;       /*!< (@ 0x400F0100) Reserved  (ADC Debug pin inputs) */
    __IO uint32_t ADC_SPEED;       /*!< (@ 0x400F0104) ADC speed control */
    __IO uint32_t POWER_CONTROL;   /*!< (@ 0x400F0108) Configures ADC power vs. speed, DC-in biasing, output format and power gating. */
    __I  uint32_t RESERVED1[61];
    __I  uint32_t FIFO_OUTPUT[16]; /*!< (@ 0x400F0200 - 0x400F023C) FIFO output mapped to 16 consecutive address locations. An output contains the value and input channel ID of one or two converted samples  */
    __I  uint32_t RESERVED2[48];
    __IO uint32_t DESCRIPTOR_0[8]; /*!< (@ 0x400F0300) Table0  descriptor n, n= 0 to 7  */
    __IO uint32_t DESCRIPTOR_1[8]; /*!< (@ 0x400F0320) Table1  descriptor n, n= 0 to 7  */
    __I  uint32_t RESERVED3[752];
    __O  uint32_t CLR_EN0;         /*!< (@ 0x400F0F00) Interrupt0 clear mask */
    __O  uint32_t SET_EN0;         /*!< (@ 0x400F0F04) Interrupt0 set mask */
    __I  uint32_t MASK0;           /*!< (@ 0x400F0F08) Interrupt0 mask */
    __I  uint32_t STATUS0;         /*!< (@ 0x400F0F0C) Interrupt0 status. Interrupt0 contains FIFO fullness, descriptor status and ADC range under/overflow */
    __O  uint32_t CLR_STAT0;       /*!< (@ 0x400F0F10) Interrupt0 clear status  */
    __O  uint32_t SET_STAT0;       /*!< (@ 0x400F0F14) Interrupt0 set status  */
    __I  uint32_t RESERVED4[2];
    __O  uint32_t CLR_EN1;         /*!< (@ 0x400F0F20) Interrupt1 mask clear enable.  */
    __O  uint32_t SET_EN1;         /*!< (@ 0x400F0F24) Interrupt1 mask set enable  */
    __I  uint32_t MASK1;           /*!< (@ 0x400F0F28) Interrupt1 mask */
    __I  uint32_t STATUS1;         /*!< (@ 0x400F0F2C) Interrupt1 status. Interrupt1 contains window comparator results and register last LAST_SAMPLE[M] overrun. */
    __O  uint32_t CLR_STAT1;       /*!< (@ 0x400F0F30) Interrupt1 clear status  */
    __O  uint32_t SET_STAT1;       /*!< (@ 0x400F0F34) Interrupt1 set status  */
} LPC_VADC_Type;

#define LPC_VADC_BASE (0x400F0000)
#define LPC_VADC  ((LPC_VADC_Type *) LPC_VADC_BASE)


#define STATUS0_CLEAR_MASK (0x7f)

#define STATUS1_CLEAR_MASK (0x1fffffff)

#define FIFO_SIZE (8)


#define ADCCLK_MATCHVALUE (4 - 1) // 9.984 [MHz] (= 39.936 [MHz] / 4)
#define ADCCLK_DGECI (0)


namespace dma_vadc {

void dma_init_vadc(void)
{
    CGU_EntityConnect(CGU_CLKSRC_PLL0_AUDIO, CGU_BASE_VADC);
    CGU_EnableEntity(CGU_BASE_VADC, ENABLE);

    //RGU_SoftReset(RGU_SIG_DMA);
    //while(RGU_GetSignalStatus(RGU_SIG_DMA));

    //NOTE Reset the VADC block
    RGU_SoftReset(RGU_SIG_VADC);
    while(RGU_GetSignalStatus(RGU_SIG_VADC));

    // Disable the VADC interrupt
    NVIC_DisableIRQ(VADC_IRQn);

    LPC_VADC->CLR_EN0   = STATUS0_CLEAR_MASK; // disable interrupt0
    LPC_VADC->CLR_STAT0 = STATUS0_CLEAR_MASK; // clear interrupt status
    while(LPC_VADC->STATUS0 & 0x7d);  // wait for status to clear, have to exclude FIFO_EMPTY (bit 1)

    LPC_VADC->CLR_EN1   = STATUS1_CLEAR_MASK; // disable interrupt1
    LPC_VADC->CLR_STAT1 = STATUS1_CLEAR_MASK; // clear interrupt status
    while(LPC_VADC->STATUS1);         // wait for status to clear

    // Make sure the VADC is not powered down
    LPC_VADC->POWER_DOWN =
    (0<<0);        /* PD_CTRL:      0=disable power down, 1=enable power down */

    // Clear FIFO
    LPC_VADC->FLUSH = 1;

    // FIFO Settings
    LPC_VADC->FIFO_CFG =
        (1<<0) |         /* PACKED_READ:      0= 1 sample packed into 32 bit, 1= 2 samples packed into 32 bit */
        (FIFO_SIZE<<1);  /* FIFO_LEVEL:       When FIFO contains this or more samples raise FIFO_FULL irq and DMA_Read_Req, default is 8 */

        // Descriptors:
        LPC_VADC->DSCR_STS =
          (0<<0) |       /* ACT_TABLE:        0=table 0 is active, 1=table 1 is active */
          (0<<1);        /* ACT_DESCRIPTOR:   ID of the descriptor that is active */

        LPC_VADC->CONFIG = /* configuration register */
        (1<<0) |        /* TRIGGER_MASK:     0=triggers off, 1=SW trigger, 2=EXT trigger, 3=both triggers */
        (0<<2) |        /* TRIGGER_MODE:     0=rising, 1=falling, 2=low, 3=high external trigger */
        (0<<4) |        /* TRIGGER_SYNC:     0=no sync, 1=sync external trigger input */
        (0<<5) |        /* CHANNEL_ID_EN:    0=don't add, 1=add channel id to FIFO output data */
        (0x90<<6);      /* RECOVERY_TIME:    ADC recovery time from power down, default is 0x90 */

    LPC_VADC->DESCRIPTOR_0[0] =
        (0<<0) |       /* CHANNEL_NR:    0=convert input 0, 1=convert input 1, ..., 5=convert input 5 */
        (0<<3) |       /* HALT:          0=continue with next descriptor after this one, 1=halt after this and restart at a new trigger */
        (0<<4) |       /* INTERRUPT:     1=raise interrupt when ADC result is available */
        (0<<5) |       /* POWER_DOWN:    1=power down after this conversion */
        (1<<6) |       /* BRANCH:        0=continue with next descriptor (wraps around after top) */
                       /*                1=branch to the first descriptor in this table */
                       /*                2=swap tables and branch to the first descriptor of the new table */
                       /*                3=reserved (do not store sample). continue with next descriptor (wraps around the top) */
        (ADCCLK_MATCHVALUE<<8)  |    /* MATCH_VALUE:   Evaluate this desciptor when descriptor timer value is equal to match value */
        (0<<22) |      /* THRESHOLD_SEL: 0=no comparison, 1=THR_A, 2=THR_B */
        (1<<24) |      /* RESET_TIME:    1=reset descriptor timer */
        (1UL<<31);       /* UPDATE_TABLE:  1=update table with all 8 descriptors of this table */

    LPC_VADC->ADC_SPEED =
    ADCCLK_DGECI;   /* DGECx:      For CRS=3 all should be 0xF, for CRS=4 all should be 0xE, */
                       /*             for all other cases it should be 0 */

    LPC_VADC->POWER_CONTROL =
        (0 /*crs*/ << 0) |    /* CRS:          current setting for power versus speed programming */
        (1 << 4) |      /* DCINNEG:      0=no dc bias, 1=dc bias on vin_neg slide */
        (0 << 10) |     /* DCINPOS:      0=no dc bias, 1=dc bias on vin_pos slide */
        (0 << 16) |     /* TWOS:         0=offset binary, 1=two's complement */
        (1 << 17) |     /* POWER_SWITCH: 0=ADC is power gated, 1=ADC is active */
        (1 << 18);      /* BGAP_SWITCH:  0=ADC bandgap reg is power gated, 1=ADC bandgap is active */


}

} //namespace dma_vadc
