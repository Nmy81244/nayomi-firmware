#ifndef BOARD_PINOUT_H
#define BOARD_PINOUT_H

#include <at32f402_405.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pin Mappings */
#define LED_PIN                     GPIO_PINS_13
#define LED_GPIO_PORT               GPIOC
#define LED_CRM_CLK                 CRM_GPIOC_PERIPH_CLOCK

#define BUTTON_PIN                  GPIO_PINS_0
#define BUTTON_GPIO_PORT            GPIOA
#define BUTTON_CRM_CLK              CRM_GPIOA_PERIPH_CLOCK

#define OLED_I2C_PORT               I2C1
#define OLED_I2C_CRM_CLK            CRM_I2C1_PERIPH_CLOCK

#define OLED_GPIO_PORT              GPIOB
#define OLED_GPIO_CRM_CLK           CRM_GPIOB_PERIPH_CLOCK

#define OLED_SCL_PIN                GPIO_PINS_6
#define OLED_SCL_SOURCE             GPIO_PINS_SOURCE6

#define OLED_SDA_PIN                GPIO_PINS_7
#define OLED_SDA_SOURCE             GPIO_PINS_SOURCE7

#define OLED_I2C_MUX                GPIO_MUX_4

#define OLED_I2C_ADDRESS            0x78

/**
 * @brief Initialize all mapped GPIO pins and their peripheral clocks.
 */
void board_pinout_init(void);

#ifdef __cplusplus
}
#endif

#endif
