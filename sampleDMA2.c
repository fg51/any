/*
===============================================================================
 Name        : sampleDMA.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

#if defined (__MULTICORE_MASTER_SLAVE_M0APP) | defined (__MULTICORE_MASTER_SLAVE_M0SUB)
#include "cr_start_m0.h"
#endif


#define HSADC_DMA_READ    (8)
#define DMA_TRANSFER_SIZE (4095) // max. 4095
#define DMA_CH            (7)
#define NUM_SAMPLE        DMA_TRANSFER_SIZE


volatile int32_t capture_count;

uint32_t sample[NUM_SAMPLE] = {};
uint32_t dmaXfers = 0;

extern void DMA_IRQHandler(void);

void DMA_IRQHandler(void)
{
    //Board_LED_Toggle(0);

    /* Increase DMA transfer count */
    dmaXfers++;

#if 0
	if (Chip_GPDMA_Interrupt(LPC_GPDMA, dmaCh) == SUCCESS) {
		isDmaTxfCompleted = 1;
	}
#endif


#if 0
  if (LPC_GPDMA->INTERRSTAT & 1) {
    LPC_GPDMA->INTERRCLR = 1;
  }

  if (LPC_GPDMA->INTTCSTAT & 1) {
    LPC_GPDMA->INTTCCLEAR = 1;
  }
#else
  if (LPC_GPDMA->INTERRSTAT & 1) {
    LPC_GPDMA->INTERRCLR = 1;
  }

  if (LPC_GPDMA->INTTCSTAT & 1) {
	LPC_GPDMA->INTTCCLEAR = 1;

	//TESTPOINT_ON();
    //if ((capture_count & 1) == 0) {
    //	cic_decimate(&cic_i, CAPTUREBUFFER0, CAPTUREBUFFER_SIZEHALF);
    //	cic_decimate(&cic_q, CAPTUREBUFFER0, CAPTUREBUFFER_SIZEHALF);
    //} else {
    //	cic_decimate(&cic_i, CAPTUREBUFFER1, CAPTUREBUFFER_SIZEHALF);
    //	cic_decimate(&cic_q, CAPTUREBUFFER1, CAPTUREBUFFER_SIZEHALF);
    //}
	//TESTPOINT_SPIKE();
	//fir_filter_iq();
	//TESTPOINT_SPIKE();
	//fm_demod();
	//TESTPOINT_SPIKE();
//#if STEREO
//	stereo_separate();
//	//TESTPOINT_SPIKE();
//	//stereo_matrix();
//	TESTPOINT_SPIKE();
//	resample_fir_filter_stereo();
//#else
//	resample_fir_filter();
//#endif

//	//audio_adjust_buffer();
//	TESTPOINT_OFF();
    capture_count ++;

    //HALT_DMA(); // halt DMA for inspecting contents of buffer

    {
        // toggle LED with every 1024 interrupts
        int c = capture_count % 1024;
        if (c == 0) {
            //Board_LED_On();
        } else if (c == 512) {
            //LED_OFF();
        }
    }
  }
#endif
    capture_count ++;

    //HALT_DMA(); // halt DMA for inspecting contents of buffer

    // toggle LED with every 1024 interrupts
    if ((capture_count % 1024) == 0) {
        Board_LED_Toggle(0);
    }
}

void disable_hsadc_interrupt(const int channel);

void initialize_hsadc(void);
void setup_dma(void);


int main(void) {
    SystemCoreClockUpdate(); // up to 204 [MHz]
    Board_Init();


    // Start M0APP slave processor
#if defined (__MULTICORE_MASTER_SLAVE_M0APP)
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);
#endif

    // Start M0SUB slave processor
#if defined (__MULTICORE_MASTER_SLAVE_M0SUB)
    cr_start_m0(SLAVE_M0SUB,&__core_m0sub_START__);
#endif


#if 0
    setup_pll0audio(PLL0_MSEL, PLL0_NSEL, PLL0_PSEL);

    SysTick_Config(CGU_GetPCLKFrequency(CGU_PERIPHERAL_M4CORE) / 1000);
    VADC_Init();
    vadc_setup();
#endif


    Chip_USB0_Init(); /* Initialize the USB0 PLL to 480 MHz */
    Chip_Clock_SetDivider(CLK_IDIV_A, CLKIN_USBPLL, 2);
    /* Source DIV_A from USB0PLL, and set divider to 2 (Max div value supported is 4) [IN 480 MHz; OUT 240 MHz */

    Chip_Clock_SetDivider(CLK_IDIV_B, CLKIN_IDIVA, 3);
    /* Source DIV_B from DIV_A, [IN 240 MHz; OUT 80 MHz */
    Chip_Clock_SetBaseClock(CLK_BASE_ADCHS, CLKIN_IDIVB, true, false);
    /* Source ADHCS base clock from DIV_B */


#if 0
    // Setup SysTick Timer to interrupt at 1 msec intervals
    const uint32_t kTICKRATE_Hz = 1000;  //NOTE: 1 [msec]
    SysTick_Config(SystemCoreClock / kTICKRATE_Hz);
#endif

    // Set the LED to the state of "On"
    Board_LED_Set(0, true);


    // interrupt priority: the highest for DMA, i2s
    NVIC_SetPriority(DMA_IRQn,   ((0x01<<3)|0x01));
    NVIC_SetPriority(I2S0_IRQn,  ((0x02<<3)|0x01));

    ////
    //HSADC settings
    ////
    initialize_hsadc();
    setup_dma();

    //
    //start HSADC and GPDMA
    //
    Chip_HSADC_SWTrigger(LPC_ADCHS);

    //NOTE: start DMA
#if 0
    LPC_GPDMA->CH[DMA_CH].CONFIG = (0x1 << 0); // enable bit, 1 enable, 0 disable
#endif
    NVIC_EnableIRQ(DMA_IRQn);


    //NOTE: need to add wait for DMA complete
    //for(int i =0; i<4094; i++) {
    for(;;){
      ;//Issues with DMA interrupt, wait for end of sampling
    }


    Chip_HSADC_FlushFIFO(LPC_ADCHS);
    uint32_t sts = Chip_HSADC_GetFIFOLevel(LPC_ADCHS);
    Chip_HSADC_DeInit(LPC_ADCHS);
    Chip_GPDMA_DeInit(LPC_GPDMA);

    return 0 ;
}


void disable_hsadc_interrupt(const int channel)
{
    LPC_ADCHS->INTS[channel].CLR_EN   = 0x7F; // disable interrupt 0
    LPC_ADCHS->INTS[channel].CLR_STAT = 0x7F; // clear interrupt status
    while(LPC_ADCHS->INTS[channel].STATUS & 0x7D);
}


#define FIFO_SIZE (8)
#define ADCCLK_MATCHVALUE (4 - 1)  //NOTE: 39.936[MHz] / 4 = 9.984[MHz]


void initialize_hsadc(void)
{
    const int kHSADC_CH0 = 0;
    disable_hsadc_interrupt(kHSADC_CH0);

    const int kHSADC_CH1 = 1;
    disable_hsadc_interrupt(kHSADC_CH1);

    LPC_ADCHS->POWER_DOWN = 0;
    LPC_ADCHS->FLUSH = 1;

    Chip_HSADC_Init(LPC_ADCHS);

#if 0
    LPC_ADCHS->FIFO_CFG = (15 << 1) /* FIFO_LEVEL*/
        | (1 << 0) /* PACKED_READ*/;
#else
    LPC_ADCHS->FIFO_CFG
        /* PACKED_READ:
         * 0= 1 sample packed into 32 bit,
         * 1= 2 samples packed into 32 bit
         */
        = (1 << 0)
        /*
         * FIFO_LEVEL: 
         * When FIFO contains this or more samples raise FIFO_FULL irq and DMA_Read_Req,
         * default is 8
         * */
        | (FIFO_SIZE << 1);
#endif

#if 0
    LPC_ADCHS->DSCR_STS = 1;
#else
  // Descriptors:
  LPC_ADCHS->DSCR_STS =
      (0<<0) |       /* ACT_TABLE:        0=table 0 is active, 1=table 1 is active */
      (0<<1);        /* ACT_DESCRIPTOR:   ID of the descriptor that is active */
#endif

#if 0
    LPC_ADCHS->CONFIG= (0x90 << 6) /* RECOVERY_TIME*/
        | (0 << 5) /* CHANNEL_ID_EN*/
        | (0x01) /* TRIGGER_MASK*/;
#else
  LPC_ADCHS->CONFIG = /* configuration register */
    (1<<0) |        /* TRIGGER_MASK:     0=triggers off, 1=SW trigger, 2=EXT trigger, 3=both triggers */
    (0<<2) |        /* TRIGGER_MODE:     0=rising, 1=falling, 2=low, 3=high external trigger */
    (0<<4) |        /* TRIGGER_SYNC:     0=no sync, 1=sync external trigger input */
    (0<<5) |        /* CHANNEL_ID_EN:    0=don't add, 1=add channel id to FIFO output data */
    (0x90<<6);      /* RECOVERY_TIME:    ADC recovery time from power down, default is 0x90 */
#endif


#if 0
    //NOTE: CH0
    LPC_ADCHS->DESCRIPTOR[0][0] = (1 << 31) /* UPDATE TABLE*/
        | (1 << 24)     /* RESET_TIMER*/
        | (0 << 22)     /* THRESH*/
        | (0xA00 << 8)  /* MATCH*/
        | (0x10 << 6)   /* BRANCH*/;

    //NOTE: CH1
    LPC_ADCHS->DESCRIPTOR[1][0] = (1 << 31) /* UPDATE TABLE*/
        | (1 << 24)     /* RESET_TIMER*/
        | (0 << 22)     /* THRESH*/
        | (0x01 << 8)   /* MATCH*/
        | (0x01 << 6)   /* BRANCH*/;
#else
  LPC_ADCHS->DESCRIPTOR[0][0] =
      (0<<0) /* CHANNEL_NR:    0=convert input 0, 1=convert input 1, ..., 5=convert input 5 */
    | (0<<3) /* HALT:          0=continue with next descriptor after this one, 1=halt after this and restart at a new trigger */
    | (0<<4) /* INTERRUPT:     1=raise interrupt when ADC result is available */
    | (0<<5) /* POWER_DOWN:    1=power down after this conversion */
    | (1<<6) /* BRANCH:        0=continue with next descriptor (wraps around after top) */
					 /*                1=branch to the first descriptor in this table */
					 /*                2=swap tables and branch to the first descriptor of the new table */
					 /*                3=reserved (do not store sample). continue with next descriptor (wraps around the top) */
    |  (ADCCLK_MATCHVALUE<<8)    /* MATCH_VALUE:   Evaluate this desciptor when descriptor timer value is equal to match value */
    | (0<<22)      /* THRESHOLD_SEL: 0=no comparison, 1=THR_A, 2=THR_B */
    | (1<<24)      /* RESET_TIME:    1=reset descriptor timer */
    | (1UL<<31)       /* UPDATE_TABLE:  1=update table with all 8 descriptors of this table */
    ;
#endif

#if 0
    const uint8_t kDGEC = 0xE;
    LPC_ADCHS->ADC_SPEED = (kDGEC << 16)
        | (kDGEC << 12)
        | (kDGEC << 8)
        | (kDGEC << 4)
        | (kDGEC);
#else
  /* DGECx:
   * For CRS=3 all should be 0xF,
   * for CRS=4 all should be 0xE,
   * for all other cases it should be 0
   */
#define ADCCLK_DGECI (0)
  LPC_ADCHS->ADC_SPEED = ADCCLK_DGECI;
#endif

#if 0
    //Didn't set threshold registers as they aren't used
    LPC_ADCHS->POWER_CONTROL = (1 << 18) /* BGAP*/
        | (1 << 17) /* POWER*/
        | (1 << 10) /* DC in ADC0*/
        | (1 << 4) | (0x4) /* CRS*/
        ;
#else
  LPC_ADCHS->POWER_CONTROL =
    (0 /*crs*/ << 0) |    /* CRS:          current setting for power versus speed programming */
    (1 << 4) |      /* DCINNEG:      0=no dc bias, 1=dc bias on vin_neg slide */
    (0 << 10) |     /* DCINPOS:      0=no dc bias, 1=dc bias on vin_pos slide */
    (0 << 16) |     /* TWOS:         0=offset binary, 1=two's complement */
    (1 << 17) |     /* POWER_SWITCH: 0=ADC is power gated, 1=ADC is active */
    (1 << 18);      /* BGAP_SWITCH:  0=ADC bandgap reg is power gated, 1=ADC bandgap is active */
#endif

}

#if 0
#define LPC_VADC_BASE             0x400F0000
#else
//#define LPC_ADCHS_BASE 0x400F0000
#endif

#if 1
#define VADC_DMA_WRITE (7)
#define VADC_DMA_READ  (8)
#define VADC_DMA_READ_SRC  (LPC_ADCHS_BASE + 512)  /* VADC FIFO */
#else
#define ADCHS_DMA_WRITE  7
#define ADCHS_DMA_READ   8
#define ADCHS_DMA_READ_SRC  (LPC_ADCHS_BASE + 512)  /* VADC FIFO */
#endif

#define DMA_LLI_NUM (16)
#if 0
static GPDMA_LLI_Type DMA_LTable[DMA_LLI_NUM];
#else
static DMA_TransferDescriptor_t DMA_LTable[DMA_LLI_NUM];
#endif

#define CAPTUREBUFFER_SIZE (0x10000)
#if 0
#define CAPTUREBUFFER0 ((uint8_t*)0x20000000)   //NOTE: RamAHB32, size:0x8000
#define CAPTUREBUFFER1 ((uint8_t*)0x20008000)   //NOTE: RamAHB16, size:0x4000
#else
__DATA(RamAHB32) uint8_t capture_buf0[100] = {};
uint8_t *CAPTUREBUFFER0 = capture_buf0;

__DATA(RamAHB16) uint8_t capture_buf1[100] = {};
uint8_t *CAPTUREBUFFER1 = capture_buf1;
#endif
#define CAPTUREBUFFER_SIZEHALF (0x8000)



void setup_dma(void)
{
#if 1
    Chip_GPDMA_Init(LPC_GPDMA);
#else
    LPC_GPDMA->CONFIG =   0x01;
    while( !(LPC_GPDMA->CONFIG & 0x01) );
    /* Clear all DMA interrupt and error flag */
    LPC_GPDMA->INTTCCLEAR = 0xFF; //clears channel terminal count interrupt
    LPC_GPDMA->INTERRCLR = 0xFF; //clears channel error interrupt.
#endif

#if 0
    LPC_GPDMA->CH[DMA_CH].SRCADDR  =  (uint32_t) &LPC_ADCHS->FIFO_OUTPUT[0];
    LPC_GPDMA->CH[DMA_CH].DESTADDR = ((uint32_t) &sample);
    LPC_GPDMA->CH[DMA_CH].CONTROL  =  (DMA_TRANSFER_SIZE)   // transfer size
        | (0x0  << 12)  // src burst size
        | (0x0  << 15)  // dst burst size
        | (0x2  << 18)  // src transfer width
        | (0x2  << 21)  // dst transfer width
        | (0x1  << 24)  // src AHB master select
        | (0x0  << 25)  // dst AHB master select
        | (0x0  << 26)  // src increment: 0, src address not increment after each trans
        | (0x1  << 27)  // dst increment: 1, dst address     increment after each trans
        | (0x1  << 31); // terminal count interrupt enable bit: 1, enabled

    LPC_GPDMA->CH[DMA_CH].CONFIG   =  (0x1 << 0)   // enable bit: 1 enable, 0 disable
        | (HSADC_DMA_READ     << 1)   // src peripheral: set to 8   - HSADC
        | (0x0 << 6)   // dst peripheral: no setting - memory
        | (0x6 << 11)  // flow control: peripheral to memory - DMA control
        | (0x1 << 14)  // IE  - interrupt error mask
        | (0x1 << 15)  // ITC - terminal count interrupt mask
        | (0x0 << 16)  // lock: when set, this bit enables locked transfer
        | (0x1 << 18); // Halt: 1, enable DMA requests; 0, ignore further src DMA req
    LPC_GPDMA->CH[DMA_CH].LLI =  0;
#else
    #if 1
    /* Setup the DMAMUX */
    LPC_CREG->DMAMUX &= ~(0x3<<(VADC_DMA_WRITE*2));
    LPC_CREG->DMAMUX |= 0x3<<(VADC_DMA_WRITE*2);  /* peripheral 7 vADC Write(0x3) */
    LPC_CREG->DMAMUX &= ~(0x3<<(VADC_DMA_READ*2));
    LPC_CREG->DMAMUX |= 0x3<<(VADC_DMA_READ*2);  /* peripheral 8 vADC read(0x3) */
    #else
    configDMAMux(GPDMA_CONN_SCT_0);
    configDMAMux(GPDMA_CONN_SCT_1);

/* Do a DMA transfer M2M, M2P,P2M or P2P */
Status Chip_GPDMA_Transfer(LPC_GPDMA_T *pGPDMA,
						   uint8_t ChannelNum,
						   uint32_t src,
						   uint32_t dst,
						   GPDMA_FLOW_CONTROL_T TransferType,
						   uint32_t Size);

/* Do a DMA scatter-gather transfer M2M, M2P,P2M or P2P using DMA descriptors */
Status Chip_GPDMA_SGTransfer(LPC_GPDMA_T *pGPDMA,
							 uint8_t ChannelNum,
							 const DMA_TransferDescriptor_t *DMADescriptor,
							 GPDMA_FLOW_CONTROL_T TransferType);

    #endif


    LPC_GPDMA->CONFIG = 0x01;  /* Enable DMA channels, little endian */
    while ( !(LPC_GPDMA->CONFIG & 0x01) );

    // The size of the transfer is in multiples of 32bit copies (hence the /4)
    // and must be even multiples of FIFO_SIZE.
    uint8_t *buffer = CAPTUREBUFFER0;
    uint32_t blocksize = CAPTUREBUFFER_SIZE / DMA_LLI_NUM;
    uint32_t transfersize = blocksize / 4;

#if 1
    uint8_t dma_ch = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, 100);
    //uint32_t PeripheralConnection_ID);
#endif

    for (int i = 0; i < DMA_LLI_NUM; i++) {
        if (i == DMA_LLI_NUM / 2) {
            buffer = CAPTUREBUFFER1;
        }
        DMA_LTable[i].src = VADC_DMA_READ_SRC;
        DMA_LTable[i].dst = (uint32_t)buffer;
        DMA_LTable[i].lli = (uint32_t)(&DMA_LTable[(i+1) % DMA_LLI_NUM]);
        DMA_LTable[i].ctrl
            = (transfersize << 0) // Transfersize (does not matter when flow control is handled by peripheral)
            | (0x2 << 12)   // Source Burst Size
            | (0x2 << 15)   // Destination Burst Size
            // | (0x0 << 15) // Destination Burst Size
            | (0x2 << 18)   // Source width // 32 bit width
            | (0x2 << 21)   // Destination width   // 32 bits
            | (0x1 << 24)   // Source AHB master 0 / 1
            | (0x0 << 25)   // Dest AHB master 0 / 1
            | (0x0 << 26)   // Source increment(LAST Sample)
            | (0x1 << 27)   // Destination increment
            | (0x0UL << 31) // Terminal count interrupt disabled
            ;
        buffer += blocksize;
    }

    /*
     * Let the last LLI in the chain cause a terminal count interrupt to notify
     * when the capture buffer is completely filled
     */
    DMA_LTable[DMA_LLI_NUM / 2 - 1].ctrl |= (0x1UL << 31); // Terminal count interrupt enabled
    DMA_LTable[DMA_LLI_NUM - 1].ctrl |= (0x1UL << 31); // Terminal count interrupt enabled

#if 1
    LPC_GPDMA->CH[0].SRCADDR  = DMA_LTable[0].src;
    LPC_GPDMA->CH[0].DESTADDR = DMA_LTable[0].dst;
    LPC_GPDMA->CH[0].CONTROL  = DMA_LTable[0].ctrl;
    LPC_GPDMA->CH[0].LLI      = (uint32_t)(&DMA_LTable[1]); // must be pointing to the second LLI as the first is used when initializing
    LPC_GPDMA->CH[0].CONFIG
#else
    LPC_GPDMA->CH[DMA_CH].SRCADDR  = DMA_LTable[0].src;
    LPC_GPDMA->CH[DMA_CH].DESTADDR = DMA_LTable[0].dst;
    LPC_GPDMA->CH[DMA_CH].CONTROL  = DMA_LTable[0].ctrl;
    LPC_GPDMA->CH[DMA_CH].LLI      = (uint32_t)(&DMA_LTable[1]); // must be pointing to the second LLI as the first is used when initializing
    LPC_GPDMA->CH[DMA_CH].CONFIG
#endif
        = (0x1) // Enable bit
        | (VADC_DMA_READ << 1) // SRCPERIPHERAL - set to 8 - VADC
        | (0x0 << 6)           // Destination peripheral - memory - no setting
        | (0x2 << 11)          // Flow control - peripheral to memory - DMA control
        | (0x6 << 11)          // Flow control - peripheral to memory - peripheral control
        | (0x1 << 14)          // Int error mask
        | (0x1 << 15)          // ITC - term count error mask
        ;
#endif
}
