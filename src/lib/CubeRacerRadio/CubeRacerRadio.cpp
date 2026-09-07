#ifdef CUBERACER_M4
#include "CubeRacerRadio.h"
#include "board_profile.h"
#include <Arduino.h>
#include "hwTimer.h"
#include <SPIEx.h>

namespace
{
using namespace CubeRacer;
bool radioInitialized = false;
volatile RadioFault radioFault = RadioFault::None;

bool healthy()
{
    return radioInitialized && cuberacerRadioFault() == RadioFault::None;
}

struct Critical
{
    uint32_t previous = __get_PRIMASK();

    Critical() { __disable_irq(); }

    ~Critical() { __set_PRIMASK(previous); }
};

void (*dioCallback)() = nullptr;
bool quiescent = true;
constexpr uint32_t DIO1 = 1U << 4;
}

bool cuberacerRadioInit(void (*dio1)())
{
    if (!dio1 || cuberacerRadioFault() != RadioFault::None)
        return false;
    if (radioInitialized)
        return true;
    Critical guard;
    quiescent = false;
    digitalWrite(NSS_PIN, HIGH);
    SPIEx.setMOSI(MOSI_PIN);
    SPIEx.setMISO(MISO_PIN);
    SPIEx.setSCLK(SCK_PIN);
    SPIEx.begin();
    SPIEx.beginTransaction(SPISettings(15000000, MSBFIRST, SPI_MODE0));
    radioInitialized = true;
    if (SPIEx.getHandle()->Instance != SPI4 || SPIEx.getHandle()->State != HAL_SPI_STATE_READY)
    {
        radioFault = RadioFault::SpiError;
        return false;
    }
    dioCallback = dio1;
    EXTI_D2->PR1 = DIO1;   // W1C exactly PA4; routing/edge configuration is M7-owned.
    EXTI_D2->IMR1 |= DIO1; // CPU2-only interrupt mask.
    NVIC_ClearPendingIRQ(EXTI4_IRQn);
    NVIC_SetPriority(EXTI4_IRQn, 5);
    NVIC_EnableIRQ(EXTI4_IRQn);
    return true;
}

void cuberacerRadioEnd()
{
    if (!radioInitialized)
        return;
    Critical guard;
    hwTimer::stop(); // Stop local radio timing after an RF fault.
    {
        NVIC_DisableIRQ(TIM1_UP_IRQn);
        NVIC_DisableIRQ(TIM1_CC_IRQn);
        TIM1->DIER = 0;
        TIM1->CR1 &= ~TIM_CR1_CEN;
        TIM1->SR = 0;
        NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
        NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
    }
    // No register access before a successful init (including the disabled build).
    if (dioCallback)
    {
        NVIC_DisableIRQ(EXTI4_IRQn);
        EXTI_D2->IMR1 &= ~DIO1;
        EXTI_D2->PR1 = DIO1;
        NVIC_ClearPendingIRQ(EXTI4_IRQn);
        dioCallback = nullptr;
    }
    if (radioInitialized)
        SPIEx.end();
    digitalWrite(NSS_PIN, HIGH);
    digitalWrite(RESET_PIN, LOW);
    digitalWrite(ANTENNA_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    radioInitialized = false;
    quiescent = !(TIM1->CR1 & TIM_CR1_CEN) && !TIM1->DIER &&
                !(SPI4->CR1 & SPI_CR1_SPE) && !SPI4->IER && !(EXTI_D2->IMR1 & DIO1) &&
                (GPIOB->ODR & (1U << 4)) && !(GPIOF->ODR & ((1U << 5) | (1U << 13))) && !(GPIOE->ODR & (1U << 6));
}

bool cuberacerRadioQuiescent()
{
    return quiescent;
}

void cuberacerRadioTimerEnable()
{
    if (!cuberacerRadioTimerReady())
        return;
    NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
    NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    NVIC_EnableIRQ(TIM1_CC_IRQn);
}

bool cuberacerRadioTimerReady()
{
    return healthy();
}

bool cuberacerRadioButtonRead(uint8_t pin, bool &high)
{
    if (pin != BUTTON_PIN || !healthy())
        return false;
    high = digitalRead(pin) == HIGH;
    return true;
}

CubeRacer::RadioFault cuberacerRadioFault()
{
    if (radioFault != RadioFault::None)
        return radioFault;
    const auto status = SPIEx.getLastError();
    return status == SPI_OK ? RadioFault::None : status == SPI_TIMEOUT ? RadioFault::SpiTimeout
                                                                       : RadioFault::SpiError;
}

extern "C" void CUBERACER_DIO1_IRQHandler()
{
    if (!dioCallback)
        return;
    if (EXTI_D2->PR1 & DIO1)
    {
        EXTI_D2->PR1 = DIO1;
        if (healthy() && dioCallback)
            dioCallback();
    }
}
#endif
