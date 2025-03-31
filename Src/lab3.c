#include <stm32f0xx_hal.h>
#include "main.h"
#include <assert.h>

int lab3_main(void) {
    My_HAL_RCC_GPIOC_CLK_ENABLE(); // Enable the GPIOC clock in the RCC

    // Set up a configuration struct to pass to the initialization function
    GPIO_InitTypeDef initStr = {GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
                                GPIO_MODE_OUTPUT_PP,
                                GPIO_SPEED_FREQ_LOW,
                                GPIO_NOPULL};

    My_HAL_GPIO_Init(GPIOC, &initStr); // Initialize pins PC6, PC7, PC8 & PC9

    My_HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); // Start PC8 high

    Timer2_Init();
    TIM3_PWM_Init();

    Configure_PC6_PC7_AlternateFunction(); // Configure PC6 & PC7 for PWM
    TIM3_Enable_PWM_PC6_PC7();  // Enable TIM3 PWM output on PC6 & PC7

    while (1) {
        // Main loop does nothing, the timer interrupt handles LED toggling
    }
}

void Timer2_Init(void) {
    // Enable Timer 2 Peripheral in RCC
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Configure Timer to Trigger UEV at 4 Hz
    // Processor frequency: 8 MHz
    // Desired timer frequency: 4 Hz
    // Timer clock = 8 MHz / (PSC + 1)
    // ARR counts required for 4 Hz: (Timer Clock) / 4
    TIM2->PSC = 1999;  // Prescaler: 2000 - 1 (divides 8 MHz by 2000 -> 4 kHz)
    TIM2->ARR = 999;   // Auto-reload: 1000 - 1 (4 kHz / 1000 = 4 Hz)

    // Enable UEV Interrupt
    TIM2->DIER |= TIM_DIER_UIE;

    // Enable Timer
    TIM2->CR1 |= TIM_CR1_CEN;

    // Enable Timer Interrupt in NVIC
    NVIC_EnableIRQ(TIM2_IRQn);
}

// Timer Interrupt Handler
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) { // Check if update interrupt flag is set
        TIM2->SR &= ~TIM_SR_UIF; // Clear the update interrupt flag

        // Toggle LEDs after delay
        My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8); // Green LED
        My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9); // Orange LED
    }
}

void TIM3_PWM_Init(void) {
    // Enable TIM3 clock in RCC
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Set the timer update period for 800 Hz PWM
    // Formula: PWM frequency = timer clock / ((PSC + 1) * (ARR + 1))
    // Assume APB1 Timer Clock = 16 MHz
    TIM3->PSC = 99;  // Prescaler to slow down clock (16MHz / (99+1) = 160kHz)
    TIM3->ARR = 199; // Auto-reload value to get 800Hz (160kHz / 200 = 800Hz)

    // Configure CCMR1 to set channels 1 & 2 in PWM mode
    TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S); // Set as output
    TIM3->CCMR1 |= (7 << TIM_CCMR1_OC1M_Pos); // PWM Mode 2 for CH1
    TIM3->CCMR1 |= (6 << TIM_CCMR1_OC2M_Pos); // PWM Mode 1 for CH2

    // Enable output compare preload for both channels
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;

    // Enable output channels in CCER
    TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;

    // Set CCRx registers to 20% of ARR
    TIM3->CCR1 = (uint16_t)(TIM3->ARR * 0.2); // 20% duty cycle for CH1
    TIM3->CCR2 = (uint16_t)(TIM3->ARR * 0.2); // 20% duty cycle for CH2

    // Start the timer
    TIM3->CR1 |= TIM_CR1_CEN;
}

void Configure_PC6_PC7_AlternateFunction(void) {
    // Enable GPIOC in RCC
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // Set PC6 and PC7 to Alternate Function mode (MODER6 = 10, MODER7 = 10)
    GPIOC->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7); // Clear mode bits for PC6 and PC7
    GPIOC->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1); // Set to alternate function mode

    // Set the Alternate Function (AF0) for PC6 and PC7 (Timer 3)
    // Maybe should be AFR[1]?
    GPIOC->AFR[0] &= ~((0xF << (6 * 4)) | (0xF << (7 * 4)));  // Clear current AF settings
    GPIOC->AFR[0] |= ((0 << (6 * 4)) | (0 << (7 * 4))); // Set AF0 for PC6 and PC7 (AF0 corresponds to Timer 3)
}

void TIM3_Enable_PWM_PC6_PC7(void) {
    // Enable TIM3 peripheral clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Configure TIM3 for 800Hz PWM
    TIM3->PSC = 48 - 1;  // Set prescaler (assuming 48MHz clock)
    TIM3->ARR = 60000 / 800 - 1;  // Auto-reload for 800 Hz

    // Set PWM Mode
    TIM3->CCMR1 &= ~((7 << TIM_CCMR1_OC1M_Pos) | (7 << TIM_CCMR1_OC2M_Pos));
    TIM3->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos) | (6 << TIM_CCMR1_OC2M_Pos); // PWM Mode 1 for both

    // Enable output compare preload for both channels
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;

    // Enable output in CCER
    TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
    
    // Set initial duty cycle
    TIM3->CCR1 = TIM3->ARR * 0.5;
    TIM3->CCR2 = TIM3->ARR * 0.5;

    // Enable counter
    TIM3->CR1 |= TIM_CR1_CEN;
}
