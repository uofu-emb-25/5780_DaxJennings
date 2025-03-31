#include <stm32f0xx_hal.h>
#include "main.h"
#include <assert.h>
#include <stdio.h>

int lab6_main(void) {
    // 6.1.1. Initialize the LED pins to output.
    SystemClock_Config(); //Configure the system clock
    My_HAL_RCC_GPIOC_CLK_ENABLE(); // Enable the GPIOC clock in the RCC
    // Set up a configuration struct to pass to the initialization function
    GPIO_InitTypeDef initStr = {GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
        GPIO_MODE_OUTPUT_PP,
        GPIO_SPEED_FREQ_LOW,
        GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &initStr); // Initialize LED pins PC6, PC7, PC8 & PC9

    // 6.1.2. Select a GPIO pin to use as the ADC input. (PC0 : ADC_IN10)
    // • Remember that the “ADC_INx” additional/analog function indicates which ADC input channel the pin connects to.
    // • Configure the pin to analog mode, no pull-up/down resistors.
    // Analog mode
    GPIOC->MODER |= (1 << 0); // (Set bit 0)
    GPIOC->MODER |= (1 << 1); // (Set bit 1)
    // No pull up/down resistors
    GPIOC->PUPDR &= ~(1 << 0); // (Clear bit 0)
    GPIOC->PUPDR &= ~(1 << 1); // (Clear bit 1)
    // • Connect the output (center pin) of a potentiometer to the input pin. The other two pins of the potentiometer should be connected to 3V and GND.

    // 6.1.3. Enable the ADC1 in the RCC peripheral.
    RCC->APB2ENR |= (1 << 9); // (Set bit 9: ADC interface clock enable)

    // 6.1.4. Configure the ADC to 8-bit resolution, continuous conversion mode, hardware triggers disabled (software trigger only).
    ADC1->CFGR1 &= ~ADC_CFGR1_RES; // Clear resolution bits
    ADC1->CFGR1 |= ADC_CFGR1_RES_1; // Set resolution to 8-bit
    ADC1->CFGR1 |= ADC_CFGR1_CONT; // Enable continuous conversion mode
    ADC1->CFGR1 &= ~ADC_CFGR1_EXTEN; // Disable hardware trigger (software trigger only)

    // 6.1.5. Select/enable the input pin’s channel for ADC conversion.
    ADC1->CHSELR = ADC_CHSELR_CHSEL10; // Select ADC_IN10 (PC0) channel
    ADC1->CR |= ADC_CR_ADCAL; // Start self-calibration
    while (ADC1->CR & ADC_CR_ADCAL); // Wait for calibration to complete

    // 6.1.6. Perform a self-calibration, enable, and start the ADC.
    // • The lab manual describes the basic procedure without mentioning the actual flags and conditions for advancement.
    // • Use sections 13.4.1 (Calibration) and 13.4.2 (ADC on-off control) in the peripheral reference manual.
    ADC1->CR |= ADC_CR_ADEN; // Enable ADC
    while (!(ADC1->ISR & ADC_ISR_ADRDY)); // Wait until ADC is ready
    ADC1->CR |= ADC_CR_ADSTART; // Start ADC conversion

    // 6.1.7. In the main application loop, read the ADC data register and turn on/off LEDs depending on the value.
    // • Use four increasing threshold values, each LED should have a minimum ADC value/- voltage to turn on.
    // • As the voltage on the input pin increases, the LEDs should light one-by-one.
    // • If the pin voltage decreases below the threshold for a LED, it should turn off.
    while (1) {
        while (!(ADC1->ISR & ADC_ISR_EOC)); // Wait for ADC conversion to complete
        uint8_t adc_value = (uint8_t)(ADC1->DR & 0xFF); // Read 8-bit ADC value

        // LED control based on threshold values
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, (adc_value > 64) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, (adc_value > 128) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, (adc_value > 192) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, (adc_value > 240) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

/*
    // (LAB 5 STUFF)
    HAL_Init(); // Reset of all peripherals, init the Flash and Systick
    SystemClock_Config(); //Configure the system clock

    My_HAL_RCC_GPIOC_CLK_ENABLE(); // Enable the GPIOC clock in the RCC
    // Set up a configuration struct to pass to the initialization function
    GPIO_InitTypeDef initStr = {GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
        GPIO_MODE_OUTPUT_PP,
        GPIO_SPEED_FREQ_LOW,
        GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &initStr); // Initialize pins PC6, PC7, PC8 & PC9
    //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
    //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);


    //5.2---------------------------------------------------------------------------------------
    // 5.2.1 Enable GPIOB and GPIOC in the RCC.
        RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
        RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    // 5.2.2 Set PB11 to alternate function mode, open-drain output type, and select I2C2_SDA as its alternate function
        // Set PB11 to alternate function mode
        //try something else GPIOB->MODER &= ~(GPIO_MODER_MODER11); // (Clear bit 0)
        GPIOB->MODER &= ~(1 << 22); // (Clear bit 22)
        //try something else GPIOB->MODER |= (GPIO_MODER_MODER11_1); // (Set bit 1)
        GPIOB->MODER |= (1 << 23); // (Set bit 23)
        // Open-drain output type
        GPIOB->OTYPER |= (1 << 11); // Sets the 11 bit in the GPIOB_OTYPER register
        // Select I2C2_SDA as alternate function

        GPIOB->AFR[1] &= ~(0xF << ((11 - 8) * 4));  // Use AFR[1] for PB11
        GPIOB->AFR[1] |= (1 << ((11 - 8) * 4)); // AF1 for PB11

    // 5.2.3 Set PB13 to alternate function mode, open-drain output type, and select I2C2_SCL as its alternate function.
        // Set PB13 to alternate function mode
        //try something else GPIOB->MODER &= ~(GPIO_MODER_MODER13); // (Clear bit 0)
        GPIOB->MODER &= ~(1 << 26); // (Clear bit 26)
        //try something else GPIOB->MODER |= (GPIO_MODER_MODER13_1); // (Set bit 1)
        GPIOB->MODER |= (1 << 27); // (Set bit 27)
        // Open-drain output type
        GPIOB->OTYPER |= (1 << 13); // (Set bit 13)
        // Select I2C2_SCL as alternate function

        GPIOB->AFR[1] &= ~(0xF << ((13 - 8) * 4)); // Use AFR[1] for PB13
        GPIOB->AFR[1] |= (5 << ((13 - 8) * 4)); // AF5 for PB13

    // 5.2.4 Set PB14 to output mode, push-pull output type, and initialize/set the pin high.
        // Set PB14 to output mode
        GPIOB->MODER |= (1 << 28); // (Set bit 28)
        GPIOB->MODER &= ~(1 << 29); // (Clear bit 29)
        // Push-pull output type
        GPIOB->OTYPER &= ~(1 << 14); // (Clear bit 14)
        // Set pin high
        My_HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // Start PB14 high
    // 5.2.5 Set PC0 to output mode, push-pull output type, and initialize/set the pin high.
        // Set PC0 to output mode
        GPIOC->MODER |= (1 << 0); // (Set bit 0)
        GPIOC->MODER &= ~(1 << 1); // (Clear bit 1)
        // Push-pull output type
        GPIOC->OTYPER &= ~(1 << 0); // (Clear bit 0)
        // Set pin high
        My_HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // Start PC0 high
    //------------------------------------------------------------------------------------------


    //5.3---------------------------------------------------------------------------------------
    // 5.3.1 Enable the I2C2 peripheral in the RCC.
        RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
    // 5.3.2 Set the parameters in the TIMINGR register to use 100 kHz standard-mode I2C.
        // PRESC to 1
        I2C2->TIMINGR |= (1 << 28); // (Set bit 28)
        // SCLL to 0x13
        I2C2->TIMINGR |= (1 << 0) | (1 << 1) | (1 << 4); // (Set bits 0, 1, and 4)
        // SCLH to 0xF
        I2C2->TIMINGR |= (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11); // (Set bits 8-11)
        // SDADEL to 0x2
        I2C2->TIMINGR |= (1 << 17); // (Set bit 17)
        // SCLDEL to 0x4
        I2C2->TIMINGR |= (1 << 22); // (Set bit 22)
    // 5.3.3 Enable the I2C peripheral using the PE bit in the CR1 register.
        I2C2->CR1 |= (1 << 0); // (Set bit 0)
    //------------------------------------------------------------------------------------------


    //5.4---------------------------------------------------------------------------------------
        // Masks for waiting loops
        uint32_t TXIS_mask = (1<<1);
        uint32_t NACKF_mask = (1<<4);
        uint32_t TC_mask = (1<<6);
        uint32_t RXNE_mask = (1<<2);
    //5.4.1 Set the L3GD20 slave address = 0x69
        //Try something else... I2C2->CR2 |= (1 << 1) | (1 << 4) | (1 << 6) | (1 << 7); // (Set bits 1, 4, 6, and 7)
        I2C2->CR2 &= ~((0x7F << 16) | (0x3FF << 0)); // Clear register
        I2C2->CR2 |= (0x69 << 1); // Set SADD to 0x69

        // Number of bytes to transmit = 1.
        I2C2->CR2 |= (1 << 16); // (Set bit 16)
        // RD_WRN bit to indicate a write operation
        I2C2->CR2 &= ~(1 << 10); // (Clear bit 10)
        // Set the START bit
        I2C2->CR2 |= (1 << 13); // (Set bit 13)
    //5.4.2 Wait until either of the TXIS or NACKF flags are set.
        while(!((I2C2->ISR & TXIS_mask) | (I2C2->ISR & NACKF_mask))) {}
    //5.4.3 Write the address of the “WHO_AM_I” register into the I2C transmit register. (TXDR)
        I2C2->TXDR = 0xD3;
    //5.4.4 Wait until the TC (Transfer Complete) flag is set.
        while(!(I2C2->ISR & TC_mask)) {}
    //5.4.5 Reload the CR2 register with the same parameters as before, but set the RD_WRN bit to indicate a read operation.
        // Don’t forget to set the START bit again to perform a I2C restart condition.
        // Set the L3GD20 slave address = 0x6B
        //I2C2->CR2 |= (1 << 1) | (1 << 4) | (1 << 6) | (1 << 7); // (Set bits 1, 4, 6, and 7)

        I2C2->CR2 |= (0x69 << 1); // Set SADD to 0x69

        // Number of bytes to transmit = 1.
        I2C2->CR2 |= (1 << 16); // (Set bit 16)
        // RD_WRN bit to indicate a read operation
        I2C2->CR2 |= (1 << 10); // (Set bit 10)
        // Set the START bit
        I2C2->CR2 |= (1 << 13); // (Set bit 13)
    //5.4.6 Wait until either of the RXNE or NACKF flags are set.
        while(!((I2C2->ISR & RXNE_mask) | (I2C2->ISR & NACKF_mask))) {}
        // Continue if the RXNE flag is set.
        while(!(I2C2->ISR & RXNE_mask)) {}
    //5.4.7 Wait until the TC (Transfer Complete) flag is set.
        while(!(I2C2->ISR & TC_mask)) {}
    //5.4.8 Check the contents of the RXDR register to see if it matches 0xD3. (expected value of the “WHO_AM_I” register)
        if(I2C2->RXDR == 0xD3){
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
        }
        else{
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
        }
    //5.4.9 Set the STOP bit in the CR2 register to release the I2C bus.
        I2C2->CR2 |= (1 << 14); // (Set bit 14)
    //------------------------------------------------------------------------------------------
*/
}