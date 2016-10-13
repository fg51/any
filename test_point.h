
#define TESTPOINT_INIT() \
    do { \
        scu_pinmux(  \
                0x6, \
                11,  \
                  PUP_DISABLE   \
                | PDN_DISABLE   \
                | SLEWRATE_SLOW \
                | FILTER_ENABLE, \
                FUNC0); \
        LPC_GPIO_PORT->DIR[3] |= (1UL << 7); \
        LPC_GPIO_PORT->SET[3] |= (1UL << 7); \
    } while(0)

#define TESTPOINT_ON() 	(LPC_GPIO_PORT->SET[3] |= (1UL << 7))
#define TESTPOINT_OFF()	(LPC_GPIO_PORT->CLR[3] = (1UL << 7))
#define TESTPOINT_TOGGLE()	(LPC_GPIO_PORT->NOT[3] = (1UL << 7))
#define TESTPOINT_SPIKE()	TESTPOINT_TOGGLE();TESTPOINT_TOGGLE()


#define TARTGET_BIT (1UL << 7)


void testpoint_init(void)
{
    do {
        scu_pinmux(
                0x6,
                11,
                  PUP_DISABLE
                | PDN_DISABLE
                | SLEWRATE_SLOW
                | FILTER_ENABLE,
                FUNC0);
        LPC_GPIO_PORT->DIR[3] |= (1UL << 7);
        LPC_GPIO_PORT->SET[3] |= (1UL << 7);
    } while(0)
}

inline void testpoint_on(void)
{
    LPC_GPIO_PORT->SET[3] |= TARTGET_BIT;
}

inline testpoint_off(void)
{
    LPC_GPIO_PORT->CLR[3] = TARTGET_BIT;
}

inline testpoint_toggle(void) {
    LPC_GPIO_PORT->NOT[3] = TARGET_BIT;
}

inline testpoint_spike(void)
{
    testpoint_toggle();
    testpoint_toggle();
}
