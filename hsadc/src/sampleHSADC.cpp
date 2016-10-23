/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "driver/common_driver.h"

#if defined (__MULTICORE_MASTER_SLAVE_M0APP) | defined (__MULTICORE_MASTER_SLAVE_M0SUB)
#include "cr_start_m0.h"
#endif

// TODO: insert other include files here
#include "driver/hsadc.h"
#if 0
using drvHSADC::setup_HSADC;
using drvHSADC::lastSample;
#endif

//#include "driver/timer.h"
//using drvTimer::setup_timer;
//using drvTimer::foo;
#include "driver/foo_timer.h"
using foobar::bar;


// TODO: insert other definitions and declarations here

#define USE_INTERRUPT_MODE

void setup_board(void);
void setup_multicore(void);
void set_threshold(void);


int main(void) {

    setup_board();

#if 0
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
#else
    // set_threshold();
#endif


    setup_multicore();
#if 0
    /* Enable HSADC interrupts in NVIC */
    NVIC_EnableIRQ(ADCHS_IRQn);

    /* Update descriptor tables - needed after updating any descriptors */
    Chip_HSADC_UpdateDescTable(LPC_ADCHS, 0);

#endif
    bar();


    // TODO: insert code here

//#if 0
//    // Force the counter to be placed into memory
//    volatile static int i = 0 ;
//    // Enter an infinite loop, just incrementing a counter
//    while(1) {
//        i++ ;
//    }
//#endif

    uint32_t stored_last_0 = 0;

	/* Sleep while waiting for conversions */
	while (1) {
		__WFI();
		if (lastSample[0] >> 6 != stored_last_0 >> 6) {
			//DEBUGOUT("ADC VALUE[0]: %08x\r\n", lastSample[0]);
			stored_last_0 = lastSample[0];
		}
	}

    return 0 ;
}


void setup_board(void)
{
#ifdef __USE_LPCOPEN

    //NOTE: Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();

    /* Set up and initialize all required blocks and
     * functions related to the board hardware
     */
    Board_Init();

    // Set the LED to the state of "On"
    //Board_LED_Set(0, true);

    Board_LED_Set(0, false);

#if 1
    setup_HSADC();
#endif

    /* Enable HSADC interrupts in NVIC */
    NVIC_EnableIRQ(ADCHS_IRQn);

    /* Update descriptor tables - needed after updating any descriptors */
    Chip_HSADC_UpdateDescTable(LPC_ADCHS, 0);

    //foo();
    bar();

    Board_LED_Set(0, true);
#endif
}


void setup_multicore(void)
{
    // Start M0APP slave processor
#if defined (__MULTICORE_MASTER_SLAVE_M0APP)
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);
#endif

    // Start M0SUB slave processor
#if defined (__MULTICORE_MASTER_SLAVE_M0SUB)
    cr_start_m0(SLAVE_M0SUB,&__core_m0sub_START__);
#endif
}


void set_threshold(void)
{
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
}
