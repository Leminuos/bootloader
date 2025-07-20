#include "main.h"

int main(void)
{
    setupHardware();
    SystickConfig(71999);
    TraceInit();
    BootloaderInit();

    // Initialize GPIO for bootloader mode
    RCC->APB2ENR.BITS.IOPCEN = 0x01;
    GPIOC->CRH.BITS.MODE13 = 0x03; // Output mode, max speed 50 MHz
    GPIOC->CRH.BITS.CNF13 = 0x00;  // General purpose output push-pull
    GPIOC->ODR.BITS.ODR13 = 0;     // Set PC13 low
    delay(20);
    GPIOC->ODR.BITS.ODR13 = 1;     // Set PC13 high
    
    while (1)
    {
        if (LoadFirmware(0x10000))
        {
            // Jump to application
            JumpToApplication();
        }
        else
        {
            while (1)
            {

            }
        }
    }
}

