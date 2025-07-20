#include "main.h"

int main(void)
{
    setupHardware();
    SystickConfig(71999);
    TraceInit();
    TestLed();
    
    while (1)
    {
        GPIOC->ODR.BITS.ODR13 = !GPIOC->ODR.BITS.ODR13;
        DEBUG(LOG_WARN, __FUNCTION__, "Welcome application");
        delay(1000);
    }
}

void SystickConfig(uint32_t u32Reload)
{
    /* Cau hinh systick */
    SysTick->VAL = u32Reload;
    SysTick->LOAD = u32Reload;
    SysTick->CTRL = BIT2 | BIT1 | BIT0;
}
