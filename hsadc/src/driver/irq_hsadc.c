#ifdef __cplusplus
extern "C" {
#endif

#include "driver/common_driver.h"

#if 0
#include "shared_data.h"
#else
extern volatile uint32_t lastSample[6];
#endif


/**
 * @brief    Handle interrupt for HSADC peripheral
 * @return    Nothing
 * @note    The HSADC IRQ handler is called SPIFI_ADCHS_IRQHandler() on the
 * M0 cores. For the M4 core, it's called ADCHS_IRQHandler().
 */
void ADCHS_IRQHandler(void)
{
    static bool isOn;
    //int prn_delay = 0;

    /* Toggle LED on each sample interrupt */
    isOn = !isOn;
    Board_LED_Set(0, isOn);

    //{{{ group1
    /* Get threshold interrupt status on group 1 and toggle on any crossing */
    const uint32_t kSTS1 = Chip_HSADC_GetIntStatus(LPC_ADCHS, 1) 
        & Chip_HSADC_GetEnabledInts(LPC_ADCHS, 1);
    if (kSTS1 & (HSADC_INT1_THCMP_DCROSS(0) | HSADC_INT1_THCMP_DCROSS(1) |
               HSADC_INT1_THCMP_UCROSS(2) | HSADC_INT1_THCMP_UCROSS(3) |
               HSADC_INT1_THCMP_DCROSS(4) | HSADC_INT1_THCMP_UCROSS(5))) {
        Board_LED_Set(1, true);
    } else {
        Board_LED_Set(1, false);
    }

    /* Clear threshold interrupts statuses */
    Chip_HSADC_ClearIntStatus(LPC_ADCHS, 1, kSTS);
    //}}} group1

    /* Pull data from FIFO */
    uint32_t data = Chip_HSADC_GetFIFO(LPC_ADCHS);
    while (!(data & HSADC_FIFO_EMPTY)) {
        /* Pull sample data and channel from FIFO data */
        const uint32_t kSAMPLE = HSADC_FIFO_SAMPLE(data);
        const uint32_t kCH = HSADC_FIFO_CHAN_ID(data);

        /* We don't really have anythng to do with the data,
           so just save it */
        //drvHSADC::lastSample[ch] = sample;
        lastSample[kCH] = kSAMPLE;

        /* Next sample */
        data = Chip_HSADC_GetFIFO(LPC_ADCHS);
    }

    //{{{ group0
    /* Get ADC interrupt status on group 0 */
    uint32_t kSTS2 = Chip_HSADC_GetIntStatus(LPC_ADCHS, 0)
        & Chip_HSADC_GetEnabledInts(LPC_ADCHS, 0);

    /* Set LED 2 (if it exists) on an error */
    if (kSTS2 & (HSADC_INT0_FIFO_OVERFLOW | HSADC_INT0_DSCR_ERROR)) {
        Board_LED_Set(2, true);
    } else {
        Board_LED_Set(2, false);
    }

    /* Clear group 0 interrupt statuses */
    Chip_HSADC_ClearIntStatus(LPC_ADCHS, 0, kSTS2);
    ///}}} group0
}


#ifdef __cplusplus
} //extern "C" {
#endif
