#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "hal_gpio.h"
#include <stm32f0xx_hal.h>

// Gyroscope configuration constants
uint8_t GYRO_ADDRESS = 0x69;
uint8_t GYRO_WHOAMI_REG = 0x0F;
uint8_t GYRO_WHOAMI_EXPECTED = 0xD4;

void SystemClock_Config(void);

// Light up exactly one LED based on GPIO position
static inline void activateLED(uint32_t pinPosition) {
    GPIOC->ODR = (GPIOC->ODR & ~((1 << 6) | (1 << 7) | (1 << 8) | (1 << 9))) | (1 << pinPosition);
}

// Check for I2C transfer status or error (NACK)
int waitForI2CFlag(int targetFlagBit) {
    while (!(I2C2->ISR & (1 << 4)) && !(I2C2->ISR & (1 << targetFlagBit))) {}
    if (I2C2->ISR & (1 << 4)) {
        // NACK received — turn on red LED to indicate error
        GPIOC->ODR |= (1 << 6);
        return 0;
    }
    return 1;
}

// Perform I2C read/write transaction with gyroscope
void gyroscopeTransaction(int deviceAddress, int byteCount, int *dataBuffer, int isRead, int registerAddress) {
    const uint32_t NUM_BYTES_POS = 16;
    const uint32_t SLAVE_ADDR_POS = 1;
    const uint32_t AUTO_INCREMENT = 0x80;

    // Clear I2C2 CR2 fields before new transaction
    I2C2->CR2 &= ~((0xFF << NUM_BYTES_POS) | 0x3FF | (1 << 10));

    if (isRead) {
        // Read: First phase just sends the register address
        I2C2->CR2 |= (1 << NUM_BYTES_POS) | (deviceAddress << SLAVE_ADDR_POS);
    } else {
        // Write: Send register address + data
        I2C2->CR2 |= ((byteCount + 1) << NUM_BYTES_POS) | (deviceAddress << SLAVE_ADDR_POS);
    }

    // Start transmission
    I2C2->CR2 |= (1 << 13);

    if (waitForI2CFlag(1)) {  // Wait for TXIS
        I2C2->TXDR = registerAddress | ((byteCount > 1) ? AUTO_INCREMENT : 0);

        if (isRead) {
            while (!(I2C2->ISR & (1 << 6))) {}  // Wait for transfer complete

            // Reconfigure for reading
            I2C2->CR2 &= ~((0xFF << NUM_BYTES_POS) | 0x3FF);
            I2C2->CR2 |= (byteCount << NUM_BYTES_POS) | (deviceAddress << SLAVE_ADDR_POS) | (1 << 10);
            I2C2->CR2 |= (1 << 13);  // Restart

            for (int i = 0; i < byteCount; i++) {
                while (!waitForI2CFlag(2)) {}
                dataBuffer[i] = I2C2->RXDR;
            }
        } else {
            for (int i = 0; i < byteCount; i++) {
                while (!waitForI2CFlag(1)) {}
                I2C2->TXDR = dataBuffer[i];
            }
        }
    }

    while (!(I2C2->ISR & (1 << 6)));  // Wait for STOP condition
    I2C2->CR2 |= (1 << 14);           // Send STOP
}

// Main lab function
int lab5_main(void) {
    HAL_Init();
    SystemClock_Config();

    // Enable peripheral clocks for GPIO and I2C
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

    // Configure PC6-PC9 as LED outputs (Red, Blue, Orange, Green)
    GPIOC->MODER |= (1 << 12) | (1 << 14) | (1 << 16) | (1 << 18);

    // Configure PB11 (SDA)
    GPIOB->MODER |= (1 << 23);
    GPIOB->MODER &= ~(1 << 22);
    GPIOB->OTYPER |= (1 << 11);
    GPIOB->AFR[1] |= (1 << 12);

    // Configure PB13 (SCL)
    GPIOB->MODER |= (1 << 27);
    GPIOB->MODER &= ~(1 << 26);
    GPIOB->OTYPER |= (1 << 13);
    GPIOB->AFR[1] |= (1 << 20);
    GPIOB->AFR[1] &= ~(1 << 21);
    GPIOB->AFR[1] |= (1 << 22);
    GPIOB->AFR[1] &= ~(1 << 23);

    // Configure PB14 (output, high)
    GPIOB->MODER &= ~(1 << 29);
    GPIOB->MODER |= (1 << 28);
    GPIOB->OTYPER &= ~(1 << 14);
    GPIOB->ODR |= (1 << 14);

    // Configure PC0 as push-pull output (mode select high)
    GPIOC->MODER |= (1 << 0);
    GPIOC->ODR |= (1 << 0);

    // Set up I2C2 timing
    I2C2->TIMINGR = 0;
    I2C2->TIMINGR |= 0x13;
    I2C2->TIMINGR |= (0xF << 8);
    I2C2->TIMINGR |= (0x2 << 16);
    I2C2->TIMINGR |= (0x4 << 20);
    I2C2->TIMINGR |= (1 << 28);

    // Enable I2C2
    I2C2->CR1 |= (1 << 0);

    // // Gyroscope WHO_AM_I read test
    // int dummy;
    // gyroscopeTransaction(GYRO_ADDRESS, 1, &dummy, 0, 0x0F);
    // gyroscopeTransaction(GYRO_ADDRESS, 1, &dummy, 0, GYRO_WHOAMI_REG);

    // int whoAmIValue = 0;
    // gyroscopeTransaction(GYRO_ADDRESS, 1, &whoAmIValue, 1, 0x0F);
    // gyroscopeTransaction(GYRO_ADDRESS, 1, &whoAmIValue, 1, GYRO_WHOAMI_REG);

    // if (whoAmIValue == GYRO_WHOAMI_EXPECTED) {
    //     HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
    //     HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    //     HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
    //     HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
    // } else {
    //     My_HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
    // }

    // Gyroscope CTRL_REG1 setup: enable output
    int ctrlRegValue = 11;
    gyroscopeTransaction(GYRO_ADDRESS, 1, &ctrlRegValue, 0, 0x20);

    int gyroRawData[4];

    while (1) {
        HAL_Delay(100);
        gyroscopeTransaction(GYRO_ADDRESS, 4, gyroRawData, 1, 0x28);

        int16_t xAxis = (gyroRawData[1] << 8) | gyroRawData[0];
        int16_t yAxis = (gyroRawData[3] << 8) | gyroRawData[2];

        if (yAxis >= 500) {
            if (xAxis >= 500)
                activateLED((xAxis > yAxis) ? 9 : 6);
            else if (xAxis <= -500)
                activateLED((abs(xAxis) > yAxis) ? 8 : 6);
        } else if (yAxis <= -500) {
            if (xAxis <= -500)
                activateLED((abs(xAxis) > abs(yAxis)) ? 8 : 7);
            else if (xAxis >= 500)
                activateLED((xAxis > abs(yAxis)) ? 9 : 7);
        }
    }
}
