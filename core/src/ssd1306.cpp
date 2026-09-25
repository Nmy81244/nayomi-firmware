#include "ssd1306.h"
#include "board_pinout.h"

static i2c_handle_type oled_i2c;

static constexpr uint32_t I2C_TIMEOUT = 0xFFFFFFF;

/*
 * Artery's I2C application library expects the slave address
 * in the shifted form:
 *
 * SSD1306 datasheet/module address = 0x3C
 * Artery transfer address          = 0x78
 */

extern "C" void i2c_lowlevel_init(i2c_handle_type *hi2c)
{
    gpio_init_type gpio_init_struct;

    if (hi2c->i2cx != OLED_I2C_PORT)
        return;

    /* Enable clocks */
    crm_periph_clock_enable(OLED_I2C_CRM_CLK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

    /* PB6 -> I2C1_SCL */
    gpio_pin_mux_config(
        OLED_SCL_PORT,
        OLED_SCL_SOURCE,
        OLED_I2C_MUX
    );

    /* PB7 -> I2C1_SDA */
    gpio_pin_mux_config(
        OLED_SDA_PORT,
        OLED_SDA_SOURCE,
        OLED_I2C_MUX
    );

    /*
     * I2C pins:
     * MUX + open drain.
     */
    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_mode           = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;

    gpio_init_struct.gpio_pins = OLED_SCL_PIN;
    gpio_init(OLED_SCL_PORT, &gpio_init_struct);

    gpio_init_struct.gpio_pins = OLED_SDA_PIN;
    gpio_init(OLED_SDA_PORT, &gpio_init_struct);

    /*
     * Start conservatively at the 10 kHz timing value
     * from Artery's official I2C example.
     *
     * Once the display works, we can move this to 100 kHz.
     */
    i2c_init(
        hi2c->i2cx,
        0x0F,
        0x4170FEFE
    );

    /*
     * This is the MCU's own address.
     * It doesn't matter for our master-only OLED use,
     * but the Artery middleware initializes it this way.
     */
    i2c_own_address1_set(
        hi2c->i2cx,
        I2C_ADDRESS_MODE_7BIT,
        0x00
    );
}

static i2c_status_type oled_write(
    const uint8_t *data,
    uint16_t size
)
{
    return i2c_master_transmit(
        &oled_i2c,
        OLED_I2C_ADDRESS,
        const_cast<uint8_t *>(data),
        size,
        I2C_TIMEOUT
    );
}

static i2c_status_type ssd1306_command(
    uint8_t command
)
{
    const uint8_t packet[] = {
        0x00,
        command
    };

    return oled_write(packet, sizeof(packet));
}

static i2c_status_type ssd1306_command2(
    uint8_t command,
    uint8_t value
)
{
    const uint8_t packet[] = {
        0x00,
        command,
        value
    };

    return oled_write(packet, sizeof(packet));
}

static i2c_status_type ssd1306_set_page(
    uint8_t page
)
{
    const uint8_t packet[] = {
        0x00,
        static_cast<uint8_t>(0xB0 | page),
        0x00,
        0x10
    };

    return oled_write(packet, sizeof(packet));
}

static i2c_status_type ssd1306_write_page(
    const uint8_t *data
)
{
    uint8_t packet[129];

    packet[0] = 0x40;

    for (uint8_t i = 0; i < 128; ++i)
        packet[i + 1] = data[i];

    return oled_write(packet, sizeof(packet));
}

void ssd1306_init(void)
{
    oled_i2c.i2cx = OLED_I2C_PORT;

    /* Configure I2C1 + PB6/PB7 */
    i2c_config(&oled_i2c);

    /*
     * SSD1306 initialization for 128x32.
     */
    ssd1306_command(0xAE);           // Display OFF
    ssd1306_command2(0xD5, 0x80);    // Display clock
    ssd1306_command2(0xA8, 0x1F);    // Multiplex = 32
    ssd1306_command2(0xD3, 0x00);    // Display offset
    ssd1306_command(0x40);           // Start line = 0

    ssd1306_command2(0x8D, 0x14);    // Charge pump ON

    ssd1306_command2(0x20, 0x00);    // Horizontal addressing

    ssd1306_command(0xA1);           // Segment remap
    ssd1306_command(0xC8);           // COM scan direction

    ssd1306_command2(0xDA, 0x02);    // COM pins for 128x32

    ssd1306_command2(0x81, 0x8F);    // Contrast
    ssd1306_command2(0xD9, 0xF1);    // Pre-charge
    ssd1306_command2(0xDB, 0x40);    // VCOMH

    ssd1306_command(0xA4);           // Display follows RAM
    ssd1306_command(0xA6);           // Normal display

    ssd1306_command(0xAF);           // Display ON
}

i2c_status_type ssd1306_test_pattern(void)
{
    uint8_t line[128];

    /*
     * Four pages on a 128x32 display.
     *
     * Alternating pages of 0xAA / 0x55 creates a very
     * obvious pattern if the display is actually alive.
     */
    for (uint8_t page = 0; page < 4; ++page)
    {
        if (ssd1306_set_page(page) != I2C_OK)
            return I2C_ERR_STEP_1;

        for (uint8_t x = 0; x < 128; ++x)
        {
            if (page & 1)
                line[x] = 0x55;
            else
                line[x] = 0xAA;
        }

        i2c_status_type status = ssd1306_write_page(line);

        if (status != I2C_OK)
            return status;
    }

    return I2C_OK;
}