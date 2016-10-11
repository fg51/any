#include "driver/dma_vadc.h"


namespace gpdma {
#include <lpc43xx_gpdma.h>
} // namespace gpdma
using gpdma::GPDMA_LLI_Type;

#define 

#define ADC_CLK_MATCH_VALUE (4 - 1)  //NOTE: 9.984 [MHz] = 39.936 [MHz] / 4
#define ADC_CLK_DGECI (0)

#define FIFO_SIZE   (8)
#define DMA_LLI_NUM (16)


//{{{ globals
volatile int32_t g_capture_count;
//}}} globals


//{{{ static globals
static gpdma::GPDMA_LLI_Type DMA_LTable[DMA_LLI_NUM];
//}}} static globals



void setup_dma_vadc(void)
{
    //uint32_t transfersize;
    //uint32_t blocksize;
    //uint8_t *buffer;

    NVIC_DisableIRQ(DMA_IRQn);
    LPC_GPDMA->C0CONFIG = 0;

    /* clear all interrupts on channel 0 */
    LPC_GPDMA->INTTCCLEAR = 0x01;
    LPC_GPDMA->INTERRCLR  = 0x01;

    /* Setup the DMAMUX */
    LPC_CREG->DMAMUX &= ~(0x3<<(VADC_DMA_WRITE*2));
    LPC_CREG->DMAMUX |= 0x3<<(VADC_DMA_WRITE*2);  /* peripheral 7 vADC Write(0x3) */
    LPC_CREG->DMAMUX &= ~(0x3<<(VADC_DMA_READ*2));
    LPC_CREG->DMAMUX |= 0x3<<(VADC_DMA_READ*2);  /* peripheral 8 vADC read(0x3) */

    LPC_GPDMA->CONFIG = 0x01;  /* Enable DMA channels, little endian */
    while ( !(LPC_GPDMA->CONFIG & 0x01) );


    // The size of the transfer is in multiples of 32bit copies (hence the /4)
    // and must be even multiples of FIFO_SIZE.
    uint8_t *buffer = CAPTUREBUFFER0;
    uint32_t blocksize = CAPTUREBUFFER_SIZE / DMA_LLI_NUM;
    transfersize = blocksize / 4;

    for (int i = 0; i < DMA_LLI_NUM; i++) {
        if (i == DMA_LLI_NUM / 2) {
            buffer = CAPTUREBUFFER1;
        }
        DMA_LTable[i].SrcAddr = VADC_DMA_READ_SRC;
        DMA_LTable[i].DstAddr = (uint32_t)buffer;
        DMA_LTable[i].NextLLI = (uint32_t)(&DMA_LTable[(i+1) % DMA_LLI_NUM]);
        DMA_LTable[i].Control = (transfersize << 0) |      // Transfersize (does not matter when flow control is handled by peripheral)
                               (0x2 << 12)  |          // Source Burst Size
                               (0x2 << 15)  |          // Destination Burst Size
                               //(0x0 << 15)  |          // Destination Burst Size
                               (0x2 << 18)  |          // Source width // 32 bit width
                               (0x2 << 21)  |          // Destination width   // 32 bits
                               (0x1 << 24)  |          // Source AHB master 0 / 1
                               (0x0 << 25)  |          // Dest AHB master 0 / 1
                               (0x0 << 26)  |          // Source increment(LAST Sample)
                               (0x1 << 27)  |          // Destination increment
                               (0x0UL << 31);          // Terminal count interrupt disabled
        buffer += blocksize;
    } //END for (int i = 0; i < DMA_LLI_NUM; i++)

    // Let the last LLI in the chain cause a terminal count interrupt to
    // notify when the capture buffer is completely filled
    DMA_LTable[DMA_LLI_NUM/2 - 1].Control |= (0x1UL << 31); // Terminal count interrupt enabled
    DMA_LTable[DMA_LLI_NUM - 1].Control |= (0x1UL << 31); // Terminal count interrupt enabled

    LPC_GPDMA->C0SRCADDR = DMA_LTable[0].SrcAddr;
    LPC_GPDMA->C0DESTADDR = DMA_LTable[0].DstAddr;
    LPC_GPDMA->C0CONTROL = DMA_LTable[0].Control;
    LPC_GPDMA->C0LLI     = (uint32_t)(&DMA_LTable[1]); // must be pointing to the second LLI as the first is used when initializing
    LPC_GPDMA->C0CONFIG  =  (0x1)        |          // Enable bit
                            (VADC_DMA_READ << 1) |  // SRCPERIPHERAL - set to 8 - VADC
                            (0x0 << 6)   |          // Destination peripheral - memory - no setting
                            (0x2 << 11)  |          // Flow control - peripheral to memory - DMA control
//                            (0x6 << 11)  |          // Flow control - peripheral to memory - peripheral control
                            (0x1 << 14)  |          // Int error mask
                            (0x1 << 15);            // ITC - term count error mask

    NVIC_EnableIRQ(DMA_IRQn);
}


