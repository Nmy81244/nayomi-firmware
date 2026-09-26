#include "board_pinout.h"

void board_pinout_init(void)
{
    crm_periph_clock_enable(LED_CRM_CLK, TRUE);
    crm_periph_clock_enable(HALL_1_GPIO_CRM_CLK, TRUE);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    /* LED */
    gpio_init_struct.gpio_pins           = LED_PIN;
    gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init(LED_GPIO_PORT, &gpio_init_struct);

    /* HALL #1 output -> PA0 -> ADC */
    gpio_init_struct.gpio_pins = HALL_1_PIN;
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(HALL_1_GPIO_PORT, &gpio_init_struct);
}