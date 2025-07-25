#include "stm32_interrupt.h"
#include "stm32_hal_usb.h"
#include "debug.h"

extern void ButtonProcess(void);
void prvGetRegistersFromStack(uint32_t* pulFaultStackAddress);

extern uint32_t u32Tick;
static uint32_t u32Counter;

void SysTick_Handler(void)
{
    ++u32Tick;
}

void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4                         \n"
        "ite eq                             \n"
        "mrseq r0, msp                      \n"
        "mrsne r0, psp                      \n"
        "ldr r1, [r0, #24]                  \n"
        "ldr r2, =prvGetRegistersFromStack  \n"
        "bx r2                              \n"
    );
}

void MemManage_Handler(void)
{
    __asm volatile (
        "tst lr, #4                         \n"
        "ite eq                             \n"
        "mrseq r0, msp                      \n"
        "mrsne r0, psp                      \n"
        "ldr r1, [r0, #24]                  \n"
        "ldr r2, =prvGetRegistersFromStack  \n"
        "bx r2                              \n"
    );
}

void BusFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4                         \n"
        "ite eq                             \n"
        "mrseq r0, msp                      \n"
        "mrsne r0, psp                      \n"
        "ldr r1, [r0, #24]                  \n"
        "ldr r2, =prvGetRegistersFromStack  \n"
        "bx r2                              \n"
    );
}

void UsageFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4                         \n"
        "ite eq                             \n"
        "mrseq r0, msp                      \n"
        "mrsne r0, psp                      \n"
        "ldr r1, [r0, #24]                  \n"
        "ldr r2, =prvGetRegistersFromStack  \n"
        "bx r2                              \n"
    );
}

void prvGetRegistersFromStack(uint32_t* pulFaultStackAddress)
{
    volatile uint32_t r0  = 0;
    volatile uint32_t r1  = 0;
    volatile uint32_t r2  = 0;
    volatile uint32_t r3  = 0;
    volatile uint32_t r12 = 0;
    volatile uint32_t lr  = 0;
    volatile uint32_t pc  = 0;
    volatile uint32_t psr = 0;

    r0  = pulFaultStackAddress[0];
    r1  = pulFaultStackAddress[1];
    r2  = pulFaultStackAddress[2];
    r3  = pulFaultStackAddress[3];
    r12 = pulFaultStackAddress[4];
    lr  = pulFaultStackAddress[5];
    pc  = pulFaultStackAddress[6];
    psr = pulFaultStackAddress[7];

    debug_print("r0  : %08X\r\n", r0);
    debug_print("r1  : %08X\r\n", r1);
    debug_print("r2  : %08X\r\n", r2);
    debug_print("r3  : %08X\r\n", r3);
    debug_print("r12 : %08X\r\n", r12);
    debug_print("lr  : %08X\r\n", lr);
    debug_print("pc  : %08X\r\n", pc);
    debug_print("psr : %08X\r\n", psr);

    // Soft reset
    NVIC_SystemReset();
}

void EXTIConfig(void)
{
    /* Cau hinh input GPIOA pin 1 */
    RCC->APB2ENR.BITS.IOPAEN = 0x01;
    RCC->APB2ENR.BITS.AFIOEN = 0x01;
    GPIOA->CRL.BITS.MODE1 = 0x00;
    GPIOA->CRL.BITS.CNF1 = 0x02;
    GPIOA->ODR.BITS.ODR1 = 1;

    /* Cau hinh ngat EXTI1 */
    AFIO->EXTICR1.BITS.EXTI1 = 0x00;
    EXTI->IMR.BITS.MR1 = 0x01;
    EXTI->FTSR.BITS.TR1 = 0x01;
    
    /* Cau hinh ngat NVIC */
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_SetPriority(EXTI1_IRQn, 0x01);
    
    /* Bat ngat toan cuc */
    __ASM("CPSIE I");
}

void EXTI1_IRQHandler(void)
{
    if (EXTI->PR.BITS.PR1 == 1)
    {
        EXTI->PR.BITS.PR1 = 1;
        NVIC_ClearPendingIRQ(EXTI1_IRQn);
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM2->DIER.BITS.UIE && TIM2->SR.BITS.UIF)
    {
        ++u32Counter;

        if (u32Counter == 200)
        {
            u32Counter = 0;
            GPIOC->ODR.BITS.ODR13 = !GPIOC->ODR.BITS.ODR13;
        }

        TIM2->SR.BITS.UIF = 0;
        NVIC_ClearPendingIRQ(TIM2_IRQn);
    }
}

#if 0
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    if (USB->ISTR.BITS.RESET != RESET)
    {
        USB_ResetCallBack();
    }
    
    if (USB->ISTR.BITS.CTR != RESET)
    {
        USB_TransactionCallBack();
    }
    
    if (USB->ISTR.BITS.ERR != RESET)
    {
        USB->ISTR.BITS.ERR = 0;
    }
    
    if (USB->ISTR.BITS.SOF != RESET)
    {
        USB->ISTR.BITS.SOF = 0;
    }
    
    if (USB->ISTR.BITS.ESOF != RESET)
    {
        USB->ISTR.BITS.ESOF = 0;
    }
    
    if (USB->ISTR.BITS.SUSP != RESET)
    {
        USB->ISTR.BITS.SUSP = 0;
    }
}
#endif

