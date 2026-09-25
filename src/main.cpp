#include "at32f402_405.h"

int main()
{
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_pins           = GPIO_PINS_13;
    gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init(GPIOC, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_0;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init(GPIOA, &gpio_init_struct);

    while (true)
    {
        if (gpio_input_data_bit_read(GPIOA, GPIO_PINS_0) == RESET)
            gpio_bits_set(GPIOC, GPIO_PINS_13);
        else
            gpio_bits_reset(GPIOC, GPIO_PINS_13);
    }
}