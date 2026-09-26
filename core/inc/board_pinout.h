#ifndef BOARD_PINOUT_H
#define BOARD_PINOUT_H

#include <at32f402_405.h>

#ifdef __cplusplus
extern "C" {
#endif

/* On-board status LED */
#define LED_PIN                     GPIO_PINS_13
#define LED_GPIO_PORT               GPIOC
#define LED_CRM_CLK                 CRM_GPIOC_PERIPH_CLOCK

#define LED_ON() \
    gpio_bits_reset(LED_GPIO_PORT, LED_PIN)

#define LED_OFF() \
    gpio_bits_set(LED_GPIO_PORT, LED_PIN)

/* OLED: SSD1306, I2C1 */
#define OLED_I2C_PORT               I2C1
#define OLED_I2C_CRM_CLK            CRM_I2C1_PERIPH_CLOCK

#define OLED_GPIO_PORT              GPIOB
#define OLED_GPIO_CRM_CLK           CRM_GPIOB_PERIPH_CLOCK

#define OLED_SCL_PIN                GPIO_PINS_6
#define OLED_SCL_SOURCE             GPIO_PINS_SOURCE6

#define OLED_SDA_PIN                GPIO_PINS_7
#define OLED_SDA_SOURCE             GPIO_PINS_SOURCE7

#define OLED_I2C_MUX                GPIO_MUX_4

/* SSD1306 7-bit address 0x3C, shifted for Artery's I2C API */
#define OLED_I2C_ADDRESS            0x78

#define HALL_1_PIN               GPIO_PINS_0
#define HALL_1_GPIO_PORT         GPIOA
#define HALL_1_GPIO_CRM_CLK      CRM_GPIOA_PERIPH_CLOCK

#define HALL_1_ADC               ADC1
#define HALL_1_ADC_CHANNEL       ADC_CHANNEL_0
#define HALL_1_ADC_CRM_CLK       CRM_ADC1_PERIPH_CLOCK

void board_pinout_init(void);

#ifdef __cplusplus
}
#endif

#endif