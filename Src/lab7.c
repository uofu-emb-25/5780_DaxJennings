
#include <stdio.h>
#include <stdlib.h>
#include "stm32f0xx.h"
#include "motor.h"
#include "SEGGER_RTT.h"

#include <stdarg.h>
#include <stdio.h>

/* -------------------------------------------------------------------------------------------------------------
 *  Global Variable Declarations
 *  -------------------------------------------------------------------------------------------------------------
 */
volatile uint32_t debouncer;

/* -------------------------------------------------------------------------------------------------------------
 *  Miscellaneous Core Functions
 *  -------------------------------------------------------------------------------------------------------------
 */

 void USART_Transmitt(char data) {
    // Wait until the Transmit Data Register Empty (TXE) flag is set
    while (!(USART3->ISR & USART_ISR_TXE)){}
    
    // Write the character to the USART transmit register
    USART3->TDR = data;
}
 void USART_Print(const char *str) {
    while (*str) {
        USART_Transmitt(*str++);
    }
}
void USART_Printf(const char *fmt, ...) {
    char buf[100];  // Adjust size if needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    USART_Print(buf);
}

void LED_init(void) {
    // Initialize PC8 and PC9 for LED's
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;                                          // Enable peripheral clock to GPIOC
    GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;                  // Set PC8 & PC9 to outputs
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);                    // Set to push-pull output type
    GPIOC->OSPEEDR &= ~((GPIO_OSPEEDR_OSPEEDR8_0 | GPIO_OSPEEDR_OSPEEDR8_1) |
                        (GPIO_OSPEEDR_OSPEEDR9_0 | GPIO_OSPEEDR_OSPEEDR9_1));   // Set to low speed
    GPIOC->PUPDR &= ~((GPIO_PUPDR_PUPDR8_0 | GPIO_PUPDR_PUPDR8_1) |
                      (GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR9_1));             // Set to no pull-up/down
    GPIOC->ODR &= ~(GPIO_ODR_8 | GPIO_ODR_9);                                   // Shut off LED's
}

void  button_init(void) {
    // Initialize PA0 for button input
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;                                          // Enable peripheral clock to GPIOA
    GPIOA->MODER &= ~(GPIO_MODER_MODER0_0 | GPIO_MODER_MODER0_1);               // Set PA0 to input
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEEDR0_0 | GPIO_OSPEEDR_OSPEEDR0_1);     // Set to low speed
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1;                                        // Set to pull-down
}

/* Called by SysTick Interrupt
 * Performs button debouncing, changes wave type on button rising edge
 * Updates frequency output from ADC value
 */
void Lab7_Systick_Callback(void) {
    // Remember that this function is called by the SysTick interrupt
    // You can't call any functions in here that use delay

    debouncer = (debouncer << 1);
    if(GPIOA->IDR & (1 << 0)) {
        debouncer |= 0x1;
    }

    if(debouncer == 0x7FFFFFFF) {
        // Begin critical section
        __disable_irq();
        switch(target_rpm) {
            case 80:
                target_rpm = 50;
                break;
            case 50:
                target_rpm = 81;
                break;
            case 0:
                target_rpm = 80;
                break;
            default:
                target_rpm = 0;
                break;
        }
        __enable_irq();
        // End critical section
    }
}

/* -------------------------------------------------------------------------------------------------------------
 * Main Program Code
 *
 * Starts initialization of peripherals
 * Blinks green LED (PC9) in loop as heartbeat
 * -------------------------------------------------------------------------------------------------------------
 */
volatile uint32_t encoder_count = 0;

int lab7_main(void) {
    // Code from Lab4 to initialize UART
    HAL_Init(); // Reset of all peripherals, init the Flash and Systick
    // Enable USART3 clock
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;  // Enable GPIOC clock
    // Set up a configuration struct to pass to the initialization function
    GPIO_InitTypeDef initStr = {
        GPIO_PIN_4 | GPIO_PIN_5, // Pins for TX and RX
        GPIO_MODE_OUTPUT_PP,
        GPIO_SPEED_FREQ_LOW,
        GPIO_NOPULL,
        0x1
    };
    My_HAL_GPIO_Init(GPIOC, &initStr); // Initialize pins
    // Set baud rate for 115200 bits/second (assuming PCLK1 = HCLK)
    USART3->BRR = HAL_RCC_GetPCLK1Freq() / 115200;
    // Enable USART3 transmitter, receiver, and USART itself
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;
    // Debug: Check if USART3 is enabled
    if (! ((USART3->CR1 & USART_CR1_TE) | (USART3->CR1 & USART_CR1_RE) | (USART3->CR1 & USART_CR1_UE))) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET); // Turn on blue LED if USART3 is not enabled
        while (1); // Halt execution if USART3 is not enabled
    }
    NVIC_SetPriority(USART3_4_IRQn, 1);
    NVIC_EnableIRQ(USART3_4_IRQn);


    debouncer = 0;                          // Initialize global variables
    HAL_Init();                             // Initialize HAL internals
    LED_init();                             // Initialize LED's
    button_init();                          // Initialize button
    motor_init();                           // Initialize motor code

    while (1) {
        GPIOC->ODR ^= GPIO_ODR_9;           // Toggle green LED (heartbeat)
        encoder_count = TIM2->CNT;

        // Print values over UART
        //USART_Printf("DC=%lu, TRPM=%lu, MS=%lu\r\n", duty_cycle, target_rpm, motor_speed);
        USART_Printf("%lu,%lu,%lu\r\n", duty_cycle, target_rpm, motor_speed);

        HAL_Delay(128);                      // Delay 1/8 second
    }
}

// ----------------------------------------------------------------------------
