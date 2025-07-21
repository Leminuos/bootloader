#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32f103.h"
#include "stm32_driver_gpio.h"
#include "stm32_driver_spi.h"
#include "stm32_driver_tim.h"
#include "stm32_hal_util.h"
#include "spiflash.h"
#include "debug.h"
#include "bootloader.h"

extern void setupHardware(void);
extern void TraceInit(void);
extern void delay(uint16_t mDelay);
void SystickConfig(uint32_t u32Reload);

#endif /* __MAIN_H__ */
