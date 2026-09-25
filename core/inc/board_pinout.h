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

/**
 * @brief Initialize all mapped GPIO pins and their peripheral clocks.
 */
void board_pinout_init(void);

#ifdef __cplusplus
}
#endif

#endif
