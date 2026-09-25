#include <at32f402_405.h>

#include "board_pinout.h"

int main()
{
    board_pinout_init();

    while (true)
    {
        if (gpio_input_data_bit_read(BUTTON_GPIO_PORT, BUTTON_PIN) == RESET)
            gpio_bits_set(LED_GPIO_PORT, LED_PIN);
        else
            gpio_bits_reset(LED_GPIO_PORT, LED_PIN);
    }
}