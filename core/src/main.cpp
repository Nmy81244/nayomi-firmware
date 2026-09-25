#include <at32f402_405.h>

#include "board_pinout.h"
#include "ssd1306.h"

int main()
{
    board_pinout_init();

    ssd1306_init();

    i2c_status_type status = ssd1306_test_pattern();

    if (status != I2C_OK)
    {
        /*
         * I2C/OLED failure:
         * turn LED on permanently.
         */
        gpio_bits_set(LED_GPIO_PORT, LED_PIN);

        while (true)
        {
        }
    }

    /*
     * OLED succeeded.
     * Turn LED off and leave the test pattern displayed.
     */
    gpio_bits_reset(LED_GPIO_PORT, LED_PIN);

    while (true)
    {
    }
}