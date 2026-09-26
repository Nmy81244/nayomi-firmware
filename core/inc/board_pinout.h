#ifndef BOARD_PINOUT_H
#define BOARD_PINOUT_H

#include <at32f402_405.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WeAct Studio AT32F405CCT7 onboard status LED.
 * The board LED is active-low.
 */
#define LED_PIN                     GPIO_PINS_13
#define LED_GPIO_PORT               GPIOC
#define LED_CRM_CLK                 CRM_GPIOC_PERIPH_CLOCK

#define LED_ON()     gpio_bits_reset(LED_GPIO_PORT, LED_PIN)

#define LED_OFF()     gpio_bits_set(LED_GPIO_PORT, LED_PIN)

/* First MT9105ET Hall sensor: OUT -> PA0 -> ADC1 channel 0. */
#define HALL_1_PIN                  GPIO_PINS_0
#define HALL_1_GPIO_PORT            GPIOA
#define HALL_1_GPIO_CRM_CLK         CRM_GPIOA_PERIPH_CLOCK

#define HALL_1_ADC                  ADC1
#define HALL_1_ADC_CHANNEL          ADC_CHANNEL_0
#define HALL_1_ADC_CRM_CLK          CRM_ADC1_PERIPH_CLOCK

void board_pinout_init(void);

#ifdef __cplusplus
}
#endif

#endif
