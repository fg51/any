#include "driver/hsadc.h"


#include "driver/common_driver.h"

#include "board.h"
#include <cr_section_macros.h>


#if 0
namespace drvHSADC {
#endif
/* Last saved ADC sample for each input */
volatile uint32_t lastSample[6] = {};
#if 0
} // namespace drvHSADC
#endif


static const CHIP_CGU_CLKIN_T adcBaseClkSources[] = {
    CLKIN_IRC,      // Usually 12MHz
    CLKIN_CLKIN,    // External clock in rate
    CLKIN_CRYSTAL,  // Usually 12MHz
    CLKIN_AUDIOPLL, // Unknown, will be 0 if not configured
    CLKIN_MAINPLL   // Usually 204MHz, may be too fast to use with a divider 
};


static void setupClock(uint32_t rate);
static void cal_best_divider(
        CHIP_CGU_IDIV_T *outBestDivider,
        CHIP_CGU_CLKIN_T *outMappedCGUDuv);

static uint32_t cal_maxCGUDiv(CHIP_CGU_IDIV_T bestDivider);
static uint32_t getClockRate(int clkIndex, uint32_t maxCGU);
static void cal_best_rate(
          const uint32_t kRate
        , const uint32_t kMaxCGUDiv
        , uint32_t *bestRate
        , CHIP_CGU_CLKIN_T *savedClkIn
        , uint32_t *savedMaxCGU
        );




#if 0
namespace drvHSADC {
#endif

#define USE_INTERRUPT_MODE

#define HSADC_CLOCK_RATE_HZ (80 * 1E+6)



void setup_HSADC(void)
{
#if 1
    setupClock(HSADC_CLOCK_RATE_HZ);
    Chip_HSADC_Init(LPC_ADCHS); //NOTE: Initialize HSADC
#endif

#if 0
    //NOTE: Show the actual HSADC clock rate
    freqHSADC = Chip_HSADC_GetBaseClockRate(LPC_ADCHS);
    DEBUGOUT("HSADC sampling rate = %dKHz\r\n", freqHSADC / 1000);
#endif

#if 1
    /* Setup FIFO trip points for interrupt/DMA to 8 samples, no packing */
    Chip_HSADC_SetupFIFO(LPC_ADCHS, 8, false);

    /*
     * Software trigger only, 0x90 recovery clocks,
     * add channel IF to FIFO entry
     */
    Chip_HSADC_ConfigureTrigger(
              LPC_ADCHS, HSADC_CONFIG_TRIGGER_SW
            , HSADC_CONFIG_TRIGGER_RISEEXT
            , HSADC_CONFIG_TRIGGER_NOEXTSYNC
            , HSADC_CHANNEL_ID_EN_ADD
            , 0x90
            );
#endif

#if 1
    /* Select both positive and negative DC biasing for input 3 */
    //Chip_HSADC_SetACDCBias(LPC_ADCHS, 3, HSADC_CHANNEL_DCBIAS, HSADC_CHANNEL_DCBIAS);
    Chip_HSADC_SetACDCBias(
              LPC_ADCHS
            , 0
            , HSADC_CHANNEL_DCBIAS
            , HSADC_CHANNEL_NODCBIAS
            );
#endif

#if 1
    /* Set low A threshold to 10% and high A threshold to 90% */
    Chip_HSADC_SetThrLowValue(LPC_ADCHS, 0, ((HSADC_MAX_SAMPLEVAL * 1) / 10));
    Chip_HSADC_SetThrHighValue(LPC_ADCHS, 0, ((HSADC_MAX_SAMPLEVAL * 9) / 10));

    /* Set low B threshold to 40% and high B threshold to 60% */
    Chip_HSADC_SetThrLowValue(LPC_ADCHS, 1, ((HSADC_MAX_SAMPLEVAL * 4) / 10));
    Chip_HSADC_SetThrHighValue(LPC_ADCHS, 1, ((HSADC_MAX_SAMPLEVAL * 6) / 10));
#endif

#if 1
    /* Setup data format for 2's complement and update clock settings. This function
       should be called whenever a clock change is made to the HSADC */
    Chip_HSADC_SetPowerSpeed(LPC_ADCHS, true);

    /* Enable HSADC power */
    Chip_HSADC_EnablePower(LPC_ADCHS);
#endif

   /* Setup HSADC table 0 descriptors */
   /* Descriptor entries are mapped as follows */

#if 1
   /*
    * 0 : mapped to input 0,
    * branch to next descriptor after sample,
    * match time is 0x90 clocks for the initial sample
    * (must be greater than or equal to recovery clocks for auto power-up),
    * test against threshold A
    */
   Chip_HSADC_SetupDescEntry(
            LPC_ADCHS, 0, 0,
            (  HSADC_DESC_CH(0)
             | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(0x95)
             | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
             )
            );
#endif

#if 1
    /*
     * 1 : mapped to input 0,
     * branch to next descriptor after sample,
     * match time is 1, test against threshold A
     */
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 1
            , ( HSADC_DESC_CH(0)
              | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(1)
              | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
              ));
#endif

#if 1

    /*
     * 2-3 : mapped to input 1,
     * branch to next descriptor after sample,
     * match time is 1 test against threshold A
     */
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 2,
            (  HSADC_DESC_CH(1)
             | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(1)
             | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
             ));
#endif

#if 1
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 3, 
            (HSADC_DESC_CH(1)
             | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(1)
             | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
             ));
#endif

#if 1

    /*
     * 4-5 : mapped to input 2,
     * branch to next descriptor after sample,
     * match time is 1 test against threshold A
     */
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 4,
            (  HSADC_DESC_CH(2)
             | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(1)
             | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
             ));
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 5,
            (  HSADC_DESC_CH(2)
             | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(1)
             | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
             ));
#endif

#if 1
    /* 6 : mapped to input 3,
     * branch to next descriptor after sample,
     * match time is 1 test against threshold A
     */
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 6,
            (  HSADC_DESC_CH(3) 
             | HSADC_DESC_BRANCH_NEXT | HSADC_DESC_MATCH(1)
             | HSADC_DESC_THRESH_A | HSADC_DESC_RESET_TIMER
             )
            );
#endif

#if 1
    /*
     * 7 : mapped to input 4, 
     * branch to next descriptor after sample,
     * match time is 1, test against threshold B,
     * halt after conversion with interrupt,
     * power down after conversion
     */
    Chip_HSADC_SetupDescEntry(LPC_ADCHS, 0, 7,
            ( HSADC_DESC_CH(4)
            | HSADC_DESC_HALT | HSADC_DESC_INT | HSADC_DESC_POWERDOWN
            | HSADC_DESC_MATCH(1) | HSADC_DESC_THRESH_B | HSADC_DESC_RESET_TIMER
            ));
#endif

#if 1
    /*
     * Setup HSADC interrupts on group 0 - FIFO trip (full),
     * FIFO overrun error, and descriptor statuses
     */
    Chip_HSADC_EnableInts(
          LPC_ADCHS
        , 0
        , ( HSADC_INT0_FIFO_FULL
          | HSADC_INT0_DSCR_DONE
          | HSADC_INT0_FIFO_OVERFLOW
          | HSADC_INT0_DSCR_ERROR)
        );
#endif

#if 1
    /* Setup HSADC threshold interrupts on group 1 */
    Chip_HSADC_EnableInts(
          LPC_ADCHS
        , 1
        , ( HSADC_INT1_THCMP_DCROSS(0)
          | HSADC_INT1_THCMP_DCROSS(1) /* Inputs 0 and 1 below threshold below range */
          | HSADC_INT1_THCMP_UCROSS(2)
          | HSADC_INT1_THCMP_UCROSS(3) /* Inputs 2 and 3 above threshold range */
          | HSADC_INT1_THCMP_DCROSS(4) /* Inputs 4 downward threshold crossing detect */
          | HSADC_INT1_THCMP_UCROSS(5) /* Inputs 5 upward threshold crossing detect */
          )
        );

#endif

#if 1
    /* Enable HSADC interrupts in NVIC */
    NVIC_EnableIRQ(ADCHS_IRQn);

    /* Update descriptor tables - needed after updating any descriptors */
    Chip_HSADC_UpdateDescTable(LPC_ADCHS, 0);

#endif
}

#if 0
} // namespace drvHSADC
#endif



/*
 * Clock setup function for generating approximate HSADC clock. Trim this
 * example function as needed to get the size down in a production system.
 */
static void setupClock(const uint32_t kRate)
{
    /*
     * The HSADC clock (sample) rate is derived from the HSADC base clock
     * divided by the HSADC clock divider (only 1 or 2). The HSADC base
     * clock can be selected from a number of internal clock sources such
     * as the main PLL1, audio PLL, external crystal rate, IRC, RTC, or a
     * CGU divider. Unless a PLL is setup for the exact rate desired, a
     * rate close to the target rate may be the closest approximation.
     */

    CHIP_CGU_IDIV_T bestDivider;
    CHIP_CGU_CLKIN_T mappedCGUDuv;
    cal_best_divider(&bestDivider, &mappedCGUDuv);
    const uint32_t kMaxCGUDiv = cal_maxCGUDiv(bestDivider);

    uint32_t bestRate = 0xFFFFFFFF;
    CHIP_CGU_CLKIN_T savedClkIn;
    uint32_t savedMaxCGU = 1;
    cal_best_rate(kRate, kMaxCGUDiv, &bestRate, &savedClkIn, &savedMaxCGU);

    //NOTE: Now to setup clocks
    if (kMaxCGUDiv == 1) {
        //NOTE: CCU divider and base clock only
        //NOTE: Select best clock as HSADC base clock
        Chip_Clock_SetBaseClock(CLK_BASE_ADCHS, savedClkIn, true, false);
    } else {
        //NOTE: CCU divider with base clock routed via a CGU divider
        Chip_Clock_SetDivider(bestDivider, savedClkIn, savedMaxCGU);
        Chip_Clock_SetBaseClock(CLK_BASE_ADCHS, mappedCGUDuv, true, false);
    }

    //NOTE: Enable ADC clock
    Chip_Clock_EnableOpts(CLK_ADCHS, true, true, 1);
}


/*
 * Determine if there are any free dividers in the CGU. Assumes an
 * unused divider is attached to CLOCKINPUT_PD. Divider A can only
 * divide 1-4, B/C/D can divide 1-16, E can divider 1-256.
 */
static void cal_best_divider(
        CHIP_CGU_IDIV_T *outBestDivider,
        CHIP_CGU_CLKIN_T *outMappedCGUDuv)
{
    CHIP_CGU_IDIV_T  freeDivider = CLK_IDIV_A;  //NOTE: Dividers only
    CHIP_CGU_CLKIN_T divider     = CLKIN_IDIVA; //NOTE: CGU clock input sources
    CHIP_CGU_IDIV_T  bestDivider = CLK_IDIV_LAST;

    CHIP_CGU_CLKIN_T mappedCGUDuv;

    while (freeDivider < CLK_IDIV_LAST) {
        //NOTE:  A CGUI divider that is pulled on input down is free
        CHIP_CGU_CLKIN_T clkIn = Chip_Clock_GetDividerSource(freeDivider);
        if (clkIn == CLKINPUT_PD) {
            /* Save available divider and mapped CGU clock divider source */
            bestDivider  = freeDivider;
            mappedCGUDuv = divider;
        }

        /* Try next divider */
        freeDivider = static_cast<CHIP_CGU_IDIV_T>(freeDivider + 1);
        divider = static_cast<CHIP_CGU_CLKIN_T>(divider + 1);
    }

    *outBestDivider  = bestDivider;
    *outMappedCGUDuv = mappedCGUDuv;
}


static uint32_t cal_maxCGUDiv(CHIP_CGU_IDIV_T bestDivider)
{
    //NOTE: Determine maximum divider value per CGU divider
    if (bestDivider != CLK_IDIV_LAST) {
        if (bestDivider == CLK_IDIV_A) {
            return 4;
        }
        if (bestDivider >= CLK_IDIV_B) {
            if (bestDivider <= CLK_IDIV_D) {
                return 16;
            }
        }
        return 256;
    }

    //NOTE: No CGU divider available
    return 1;
}


static uint32_t getClockRate(const int clkIndex, const uint32_t maxCGU)
{
    const uint32_t clkRate = Chip_Clock_GetClockInputHz(adcBaseClkSources[clkIndex]);
    const uint32_t clkRate1 = clkRate / maxCGU;
    return clkRate1;
}


/*
 * Using the best available maximum CGU and CCU dividers, attempt to
 * find a base clock rate that will get as close as possible to the
 * target rate without going over the rate.
 *
 * @param [inn] kMaxCGUDiv
 * @param [out] bestRate
 * @param [out] savedClkIn
 * @param [out] savedMaxCGU
 */
static void cal_best_rate(
          const uint32_t kRate
        , const uint32_t kMaxCGUDiv
        , uint32_t *outBestRate
        , CHIP_CGU_CLKIN_T *outSavedClkIn
        , uint32_t *outSavedMaxCGU
        )

{
    uint32_t bestRate = *outBestRate;
    CHIP_CGU_CLKIN_T savedClkIn = *outSavedClkIn;
    uint32_t savedMaxCGU = *outSavedMaxCGU;

    const int kMaxClkLen = sizeof(adcBaseClkSources) / sizeof(CHIP_CGU_CLKIN_T);
    for (int clkIndex = 0; clkIndex < kMaxClkLen; clkIndex++) {
        for (uint32_t maxCGU = 1; maxCGU <= kMaxCGUDiv; maxCGU++) {
            const uint32_t kTestRate = getClockRate(clkIndex, maxCGU);
            if (kRate >= kTestRate) {
                if ((kRate - kTestRate) < (kRate - bestRate)) {
                    bestRate = kTestRate;
                    savedClkIn = adcBaseClkSources[clkIndex];
                    savedMaxCGU = maxCGU;
                } //if ((rate - testRate) < (rate - *bestRate)) {
            } //if (rate >= testRate) {
        } //for (maxCGU = 1; maxCGU <= kMaxCGUDiv; maxCGU++) {
    }

    *outBestRate = bestRate;
    *outSavedClkIn = savedClkIn;
    *outSavedMaxCGU = savedMaxCGU;
}



#if 1
/**
 * @brief    Handle interrupt for HSADC peripheral
 * @return    Nothing
 * @note    The HSADC IRQ handler is called SPIFI_ADCHS_IRQHandler() on the
 * M0 cores. For the M4 core, it's called ADCHS_IRQHandler().
 */
void ADCHS_IRQHandler(void)
{
    uint32_t sts, data, sample, ch;
    static bool on;
    //int prn_delay = 0;

    /* Toggle LED on each sample interrupt */
    on = !on;
    Board_LED_Set(0, on);

    /* Get threshold interrupt status on group 1 and toggle on any crossing */
    sts = Chip_HSADC_GetIntStatus(LPC_ADCHS, 1) & Chip_HSADC_GetEnabledInts(LPC_ADCHS, 1);
    if (sts & (HSADC_INT1_THCMP_DCROSS(0) | HSADC_INT1_THCMP_DCROSS(1) |
               HSADC_INT1_THCMP_UCROSS(2) | HSADC_INT1_THCMP_UCROSS(3) |
               HSADC_INT1_THCMP_DCROSS(4) | HSADC_INT1_THCMP_UCROSS(5))) {
        Board_LED_Set(1, true);
    }
    else {
        Board_LED_Set(1, false);
    }

    /* Clear threshold interrupts statuses */
    Chip_HSADC_ClearIntStatus(LPC_ADCHS, 1, sts);

    /* Pull data from FIFO */
    data = Chip_HSADC_GetFIFO(LPC_ADCHS);
    while (!(data & HSADC_FIFO_EMPTY)) {
        /* Pull sample data and channel from FIFO data */
        sample = HSADC_FIFO_SAMPLE(data);
        ch = HSADC_FIFO_CHAN_ID(data);

        /* We don't really have anythng to do with the data,
           so just save it */
        //drvHSADC::lastSample[ch] = sample;
        lastSample[ch] = sample;

        /* Next sample */
        data = Chip_HSADC_GetFIFO(LPC_ADCHS);
    }

    /* Get ADC interrupt status on group 0 */
    sts = Chip_HSADC_GetIntStatus(LPC_ADCHS, 0) & Chip_HSADC_GetEnabledInts(LPC_ADCHS, 0);

    /* Set LED 2 (if it exists) on an error */
#if 1
    if (sts & (HSADC_INT0_FIFO_OVERFLOW | HSADC_INT0_DSCR_ERROR)) {
        Board_LED_Set(2, true);
    }
    else {
        Board_LED_Set(2, false);
    }
#else
    Board_LED_Set(
        2,
        (sts & (HSADC_INT0_FIFO_OVERFLOW | HSADC_INT0_DSCR_ERROR))?
            true : false
        );
#endif

    /* Clear group 0 interrupt statuses */
    Chip_HSADC_ClearIntStatus(LPC_ADCHS, 0, sts);
}
#endif



