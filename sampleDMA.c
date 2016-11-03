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


uint32_t sample[NUM_SAMPLE] = {};
uint32_t dmaXfers = 0;

extern void DMA_IRQHandler(void);

void DMA_IRQHandler(void)
{
    Board_LED_Toggle(0);

    /* Increase DMA transfer count */
    dmaXfers++;

//	if (Chip_GPDMA_Interrupt(LPC_GPDMA, dmaCh) == SUCCESS) {
//		isDmaTxfCompleted = 1;
//	}
}

void disable_hsadc_interrupt(const int channel);

void initialize_hsadc(void);
void setup_dma(void);

int main(void) {
    SystemCoreClockUpdate();
    Board_Init();

    // Set the LED to the state of "On"
    Board_LED_Set(0, true);

    // Start M0APP slave processor
#if defined (__MULTICORE_MASTER_SLAVE_M0APP)
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);
#endif

    // Start M0SUB slave processor
#if defined (__MULTICORE_MASTER_SLAVE_M0SUB)
    cr_start_m0(SLAVE_M0SUB,&__core_m0sub_START__);
#endif




    Chip_USB0_Init(); /* Initialize the USB0 PLL to 480 MHz */
    Chip_Clock_SetDivider(CLK_IDIV_A, CLKIN_USBPLL, 2);
    /* Source DIV_A from USB0PLL, and set divider to 2 (Max div value supported is 4) [IN 480 MHz; OUT 240 MHz */

    Chip_Clock_SetDivider(CLK_IDIV_B, CLKIN_IDIVA, 3);
    /* Source DIV_B from DIV_A, [IN 240 MHz; OUT 80 MHz */
    Chip_Clock_SetBaseClock(CLK_BASE_ADCHS, CLKIN_IDIVB, true, false);
    /* Source ADHCS base clock from DIV_B */

    // interrupt priority: the highest for DMA, i2s
    NVIC_SetPriority(DMA_IRQn,   ((0x01<<3)|0x01));
    NVIC_SetPriority(I2S0_IRQn,  ((0x02<<3)|0x01));

    ////
    //HSADC settings
    ////
    const int kHSADC_CH0 = 0;
    const int kHSADC_CH1 = 1;
    disable_hsadc_interrupt(kHSADC_CH0);
    disable_hsadc_interrupt(kHSADC_CH1);

    initialize_hsadc();
    setup_dma();

    //
    //start HSADC and GPDMA
    //
    Chip_HSADC_SWTrigger(LPC_ADCHS);

    // start DMA
    LPC_GPDMA->CH[DMA_CH].CONFIG = (0x1 << 0); // enable bit, 1 enable, 0 disable
    NVIC_EnableIRQ(DMA_IRQn);

    //need to add wait for DMA complete


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


void initialize_hsadc(void)
{
    LPC_ADCHS->POWER_DOWN = 0;
    LPC_ADCHS->FLUSH = 1;
    Chip_HSADC_Init(LPC_ADCHS);
    LPC_ADCHS->FIFO_CFG = (15 << 1) /* FIFO_LEVEL*/
        | (1) /* PACKED_READ*/;
    LPC_ADCHS->DSCR_STS = 1;

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

    LPC_ADCHS->CONFIG= (0x90 << 6) /* RECOVERY_TIME*/
        | (0 << 5) /* CHANNEL_ID_EN*/
        | (0x01) /* TRIGGER_MASK*/;

    const uint8_t kDGEC = 0xE;
    LPC_ADCHS->ADC_SPEED = (kDGEC << 16)
        | (kDGEC << 12)
        | (kDGEC << 8)
        | (kDGEC << 4)
        | (kDGEC);

    //Didn't set threshold registers as they aren't used
    LPC_ADCHS->POWER_CONTROL = (1 << 18) /* BGAP*/
        | (1 << 17) /* POWER*/
        | (1 << 10) /* DC in ADC0*/
        | (1 << 4) | (0x4) /* CRS*/
        ;
}



void setup_dma(void)
{
    //////
    //DMA settings
    //////
    Chip_GPDMA_Init(LPC_GPDMA);
    LPC_GPDMA->CONFIG =   0x01;
    while( !(LPC_GPDMA->CONFIG & 0x01) );
    /* Clear all DMA interrupt and error flag */
    LPC_GPDMA->INTTCCLEAR = 0xFF; //clears channel terminal count interrupt
    LPC_GPDMA->INTERRCLR = 0xFF; //clears channel error interrupt.


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
}
