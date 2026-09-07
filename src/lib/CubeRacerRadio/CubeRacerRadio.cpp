#ifdef CUBERACER_M4
#include "CubeRacerRadio.h"
#include "board_profile.h"
#include <Arduino.h>
#include "hwTimer.h"
#include <SPIEx.h>

namespace {
using namespace CubeRacer;
bool radioGranted=false,radioInitialized=false;
volatile RadioFault radioFault=RadioFault::NoGrant;
bool healthy() { return radioGranted && radioFault==RadioFault::None; }
struct Critical {
    uint32_t previous=__get_PRIMASK();
    Critical() { __disable_irq(); }
    ~Critical() { __set_PRIMASK(previous); }
};
void (*dioCallback)()=nullptr;
bool quiescent=true;
constexpr uint32_t DIO1=1U<<4;
#ifndef CUBERACER_M4_RADIO_DISABLED
bool mode(GPIO_TypeDef *gpio,unsigned pin,unsigned expected,unsigned af=0) {
    return ((gpio->MODER>>(pin*2))&3U)==expected &&
        (expected!=2 || ((gpio->AFR[pin/8]>>((pin%8)*4))&15U)==af);
}
#endif
}
bool cuberacerRadioGrant(uint32_t session) {
#ifdef CUBERACER_M4_RADIO_DISABLED
    (void)session;return false;
#else
    // Read back the M7 preconditions. Never repair shared configuration from M4.
    if(!(RCC->APB2ENR&RCC_APB2ENR_SPI4EN) || !(RCC->APB2ENR&RCC_APB2ENR_TIM1EN) ||
        (RCC->D2CCIP1R&RCC_D2CCIP1R_SPI45SEL) ||
        !(GPIOE->ODR&(1U<<10)) || !mode(GPIOE,10,1) ||
        !mode(GPIOA,4,1) || !mode(GPIOF,5,1) || !mode(GPIOE,6,1) ||
        !mode(GPIOE,5,0) || !mode(GPIOB,4,0) || !mode(GPIOE,2,2,5) ||
        !mode(GPIOE,13,2,5) || !mode(GPIOE,14,2,5) ||
        !mode(GPIOF,3,0) || ((GPIOF->PUPDR>>6)&3U)!=1 ||
        !mode(GPIOF,13,1) || (GPIOF->OTYPER&(1U<<13)) || (GPIOF->PUPDR&(3U<<26)) ||
        (SYSCFG->EXTICR[1]&15U)!=1 || !(EXTI->RTSR1&DIO1) || (EXTI->FTSR1&DIO1))return false;
    if(radioGranted || !session || HAL_RCC_GetPCLK2Freq()!=120000000U)return false;
    radioGranted=true;radioFault=RadioFault::None;quiescent=false;
    return true;
#endif
}
bool cuberacerRadioInit(void (*dio1)()) {
    if(!dio1 || !healthy())return false;
    if(radioInitialized)return true;
    Critical guard;
    digitalWrite(NSS_PIN,HIGH);
    SPIEx.setMOSI(MOSI_PIN);SPIEx.setMISO(MISO_PIN);SPIEx.setSCLK(SCK_PIN);
    SPIEx.begin();
    SPIEx.beginTransaction(SPISettings(15000000,MSBFIRST,SPI_MODE0));
    radioInitialized=true;
    if(SPIEx.getHandle()->Instance!=SPI4 || SPIEx.getHandle()->State!=HAL_SPI_STATE_READY) {
        radioFault=RadioFault::SpiError;return false;
    }
    dioCallback=dio1;
    EXTI_D2->PR1=DIO1; // W1C exactly PB4; routing/edge configuration is M7-owned.
    EXTI_D2->IMR1|=DIO1; // CPU2-only interrupt mask.
    NVIC_ClearPendingIRQ(EXTI4_IRQn);
    NVIC_SetPriority(EXTI4_IRQn,5);
    NVIC_EnableIRQ(EXTI4_IRQn);
    return true;
}
void cuberacerRadioEnd() {
    if(!radioGranted)return;
    Critical guard;
    hwTimer::stop(); // Quiesce TIM1 before relinquishing the radio grant.
    {
        NVIC_DisableIRQ(TIM1_UP_IRQn);NVIC_DisableIRQ(TIM1_CC_IRQn);
        TIM1->DIER=0;TIM1->CR1&=~TIM_CR1_CEN;TIM1->SR=0;
        NVIC_ClearPendingIRQ(TIM1_UP_IRQn);NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
    }
    // No register access before a successful init (including the disabled build).
    if(dioCallback) {
        NVIC_DisableIRQ(EXTI4_IRQn);EXTI_D2->IMR1&=~DIO1;EXTI_D2->PR1=DIO1;
        NVIC_ClearPendingIRQ(EXTI4_IRQn);dioCallback=nullptr;
    }
    if(radioInitialized)SPIEx.end();
    digitalWrite(NSS_PIN,HIGH);digitalWrite(RESET_PIN,LOW);
    digitalWrite(ANTENNA_PIN,LOW);digitalWrite(LED_PIN,LOW);
    radioGranted=radioInitialized=false;
    if(radioFault==RadioFault::None)radioFault=RadioFault::NoGrant;
    quiescent=!(TIM1->CR1&TIM_CR1_CEN) && !TIM1->DIER &&
        !(SPI4->CR1&SPI_CR1_SPE) && !SPI4->IER && !(EXTI_D2->IMR1&DIO1) &&
        (GPIOA->ODR&(1U<<4)) && !(GPIOF->ODR&((1U<<5)|(1U<<13))) && !(GPIOE->ODR&(1U<<6));
}
bool cuberacerRadioQuiescent() { return quiescent; }
extern "C" int cuberacerTimerClockAllowed(uintptr_t timer) {
    return radioGranted && timer==reinterpret_cast<uintptr_t>(TIM1) && (RCC->APB2ENR&RCC_APB2ENR_TIM1EN);
}
void cuberacerRadioTimerEnable() {
    if(!cuberacerRadioTimerReady())return;
    NVIC_ClearPendingIRQ(TIM1_UP_IRQn);NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
    NVIC_EnableIRQ(TIM1_UP_IRQn);NVIC_EnableIRQ(TIM1_CC_IRQn);
}
bool cuberacerRadioTimerReady() {
    return healthy() && cuberacerTimerClockAllowed(reinterpret_cast<uintptr_t>(TIM1));
}
bool cuberacerRadioTransfer(uint8_t *data,unsigned size) {
    Critical guard;
    if(!healthy() || !radioInitialized || !data || !size || size>259)return false;
    const auto status=SPIEx.transferChecked(data,size);
    if(status!=SPI_OK)radioFault=status==SPI_TIMEOUT?RadioFault::SpiTimeout:RadioFault::SpiError;
    return healthy();
}
bool cuberacerRadioButtonRead(uint8_t pin,bool &high) {
    if(pin!=BUTTON_PIN || !healthy())return false;
    high=digitalRead(pin)==HIGH;return true;
}
CubeRacer::RadioFault cuberacerRadioFault() { return radioFault; }
extern "C" void CUBERACER_DIO1_IRQHandler() {
    if(!dioCallback)return;
    if(EXTI_D2->PR1&DIO1) {
        EXTI_D2->PR1=DIO1;
        if(healthy() && dioCallback)dioCallback();
    }
}
#endif
