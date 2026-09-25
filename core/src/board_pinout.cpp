#include "board_pinout.h"

void board_pinout_init(void)
{
    crm_periph_clock_enable(BUTTON_CRM_CLK, TRUE);
    crm_periph_clock_enable(LED_CRM_CLK, TRUE);

    /* OLED I2C peripheral + GPIO clocks */
    crm_periph_clock_enable(OLED_I2C_CRM_CLK, TRUE);
    crm_periph_clock_enable(OLED_GPIO_CRM_CLK, TRUE);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    /* LED */
    gpio_init_struct.gpio_pins           = LED_PIN;
    gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init(LED_GPIO_PORT, &gpio_init_struct);

    /* Button */
    gpio_init_struct.gpio_pins = BUTTON_PIN;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init(BUTTON_GPIO_PORT, &gpio_init_struct);

    /* PB6 -> I2C1_SCL */
    gpio_pin_mux_config(
        OLED_GPIO_PORT,
        OLED_SCL_SOURCE,
        OLED_I2C_MUX
    );

    /* PB7 -> I2C1_SDA */
    gpio_pin_mux_config(
        OLED_GPIO_PORT,
        OLED_SDA_SOURCE,
        OLED_I2C_MUX
    );

    /* I2C pins */
    gpio_init_struct.gpio_mode           = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;

    gpio_init_struct.gpio_pins = OLED_SCL_PIN;
    gpio_init(OLED_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.gpio_pins = OLED_SDA_PIN;
    gpio_init(OLED_GPIO_PORT, &gpio_init_struct);

    /* I2C1 */
    i2c_init(
        OLED_I2C_PORT,
        0x0F,
        0x4170FEFE
    );

    i2c_enable(OLED_I2C_PORT, TRUE);
}