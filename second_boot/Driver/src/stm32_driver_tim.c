#include "stm32_driver_tim.h"

uint32_t u32Tick;

uint32_t GetCounterTick(void)
{
    return u32Tick;
}

void delay(uint16_t mDelay)
{
    uint32_t currTime = GetCounterTick();
    while (GetCounterTick() - currTime < mDelay);
}

void SystickConfig(uint32_t u32Reload)
{
    /* Cau hinh systick */
    SysTick->VAL = u32Reload;
    SysTick->LOAD = u32Reload;
    SysTick->CTRL = BIT2 | BIT1 | BIT0;
}

void TIM2_Init(void)
{
    // Enable clock
    RCC->APB1ENR.BITS.TIM2EN = 0x01;
    
    // Config timer
    TIM2->ARR.REG = 1000UL;     // 1s => Update Event
    TIM2->CNT.REG = 0;
    TIM2->PSC.REG = 71UL;    // 1us => Counter
    TIM2->DIER.BITS.UIE = 0x01;
    TIM2->CR1.BITS.CEN = 0x01;

    /* Cau hinh ngat NVIC */
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 0X01);
}

void TimerConfig(void)
{
    TIM2_Init();
}
