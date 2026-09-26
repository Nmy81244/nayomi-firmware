#include "ssd1306.h"
#include "board_pinout.h"

namespace
{
    constexpr uint32_t I2C_TIMEOUT = 100000;

    /*
     * 128 x 32 / 8 = 512 bytes.
     *
     * SSD1306 memory is arranged as four pages of 128 bytes.
     */
    uint8_t framebuffer[SSD1306_WIDTH * SSD1306_PAGES] = {};

    i2c_handle_type oled_i2c;

    /*
     * SSD1306 command helpers
     */

    i2c_status_type oled_write(
        const uint8_t *data,
        uint16_t size
    )
    {
        return i2c_master_transmit(
            &oled_i2c,
            SSD1306_I2C_ADDRESS,
            const_cast<uint8_t *>(data),
            size,
            I2C_TIMEOUT
        );
    }

    i2c_status_type ssd1306_command(uint8_t command)
    {
        const uint8_t packet[] =
        {
            0x00,       // Co = 0, D/C# = 0: command
            command
        };

        return oled_write(packet, sizeof(packet));
    }

    i2c_status_type ssd1306_command2(
        uint8_t command,
        uint8_t value
    )
    {
        const uint8_t packet[] =
        {
            0x00,
            command,
            value
        };

        return oled_write(packet, sizeof(packet));
    }

    i2c_status_type ssd1306_set_page(uint8_t page)
    {
        if (page >= SSD1306_PAGES)
            return I2C_ERR_STEP_1;

        /*
         * Page addressing mode:
         *
         * 0xB0..0xB3 = page
         * 0x00        = lower column nibble
         * 0x10        = upper column nibble
         */
        const uint8_t packet[] =
        {
            0x00,
            static_cast<uint8_t>(0xB0 | page),
            0x00,
            0x10
        };

        return oled_write(packet, sizeof(packet));
    }

    i2c_status_type ssd1306_write_page(
        uint8_t page,
        const uint8_t *data
    )
    {
        i2c_status_type status = ssd1306_set_page(page);

        if (status != I2C_OK)
            return status;

        /*
         * 0x40 = Co=0, D/C#=1:
         * following bytes are display RAM data.
         *
         * 128 bytes of pixel data.
         */
        uint8_t packet[129];

        packet[0] = 0x40;

        for (uint16_t x = 0; x < SSD1306_WIDTH; ++x)
            packet[x + 1] = data[x];

        return oled_write(packet, sizeof(packet));
    }
}

/*
 * Artery I2C application library calls this weak function
 * from i2c_config().
 *
 * The board layer handles the GPIO clocks/mux configuration.
 * This function only initializes the I2C peripheral itself.
 */
extern "C" void i2c_lowlevel_init(i2c_handle_type *hi2c)
{
    if (hi2c == nullptr)
        return;

    if (hi2c->i2cx != OLED_I2C_PORT)
        return;

    /*
     * Official Artery F405 examples:
     *
     *   digital filter = 0x0F
     *   100 kHz clock  = 0x90F03030
     *
     * This assumes the official 216 MHz system clock
     * configuration is active.
     */
    i2c_init(
        hi2c->i2cx,
        0x0F,
        0x90F03030
    );

    /*
     * Own address is irrelevant for our master-only OLED use,
     * but the Artery middleware expects normal I2C configuration.
     */
    i2c_own_address1_set(
        hi2c->i2cx,
        I2C_ADDRESS_MODE_7BIT,
        0x00
    );
}

extern "C" i2c_status_type ssd1306_init(void)
{
    oled_i2c.i2cx = OLED_I2C_PORT;

    /*
     * This performs the peripheral reset, low-level initialization,
     * and I2C peripheral enable through Artery's middleware.
     */
    i2c_config(&oled_i2c);

    i2c_status_type status;

    /*
     * SSD1306 initialization sequence for 128x32.
     */

    status = ssd1306_command(0xAE);       // Display OFF
    if (status != I2C_OK)
        return status;

    status = ssd1306_command2(0xD5, 0x80); // Display clock divide/oscillator
    if (status != I2C_OK)
        return status;

    status = ssd1306_command2(0xA8, 0x1F); // Multiplex ratio = 31 -> 32 rows
    if (status != I2C_OK)
        return status;

    status = ssd1306_command2(0xD3, 0x00); // Display offset = 0
    if (status != I2C_OK)
        return status;

    status = ssd1306_command(0x40);       // Display start line = 0
    if (status != I2C_OK)
        return status;

    /*
     * Internal charge pump.
     */
    status = ssd1306_command2(0x8D, 0x14);
    if (status != I2C_OK)
        return status;

    /*
     * Page addressing mode.
     *
     * This is important because our framebuffer is written
     * one SSD1306 page at a time.
     */
    status = ssd1306_command2(0x20, 0x02);
    if (status != I2C_OK)
        return status;

    /*
     * Segment remap.
     *
     * Makes column 127 appear at the first physical segment.
     */
    status = ssd1306_command(0xA1);
    if (status != I2C_OK)
        return status;

    /*
     * COM output scan direction.
     */
    status = ssd1306_command(0xC8);
    if (status != I2C_OK)
        return status;

    /*
     * 128x32 uses the alternative COM pin configuration.
     */
    status = ssd1306_command2(0xDA, 0x02);
    if (status != I2C_OK)
        return status;

    /*
     * Contrast.
     */
    status = ssd1306_command2(0x81, 0x8F);
    if (status != I2C_OK)
        return status;

    /*
     * Pre-charge period.
     */
    status = ssd1306_command2(0xD9, 0xF1);
    if (status != I2C_OK)
        return status;

    /*
     * VCOMH deselect level.
     */
    status = ssd1306_command2(0xDB, 0x40);
    if (status != I2C_OK)
        return status;

    /*
     * Resume displaying RAM contents.
     */
    status = ssd1306_command(0xA4);
    if (status != I2C_OK)
        return status;

    /*
     * Normal display mode.
     */
    status = ssd1306_command(0xA6);
    if (status != I2C_OK)
        return status;

    /*
     * Clear the framebuffer before turning the display on.
     */
    for (uint16_t i = 0; i < sizeof(framebuffer); ++i)
        framebuffer[i] = 0;

    status = ssd1306_command(0xAF);       // Display ON
    if (status != I2C_OK)
        return status;

    return I2C_OK;
}

extern "C" void ssd1306_set_pixel(
    uint8_t x,
    uint8_t y,
    uint8_t on
)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
        return;

    const uint16_t index =
        static_cast<uint16_t>(y / 8) * SSD1306_WIDTH + x;

    const uint8_t mask =
        static_cast<uint8_t>(1U << (y & 7));

    if (on != 0)
        framebuffer[index] |= mask;
    else
        framebuffer[index] &= static_cast<uint8_t>(~mask);
}

extern "C" i2c_status_type ssd1306_flush(void)
{
    for (uint8_t page = 0; page < SSD1306_PAGES; ++page)
    {
        const uint8_t *page_data =
            &framebuffer[page * SSD1306_WIDTH];

        i2c_status_type status =
            ssd1306_write_page(page, page_data);

        if (status != I2C_OK)
            return status;
    }

    return I2C_OK;
}

extern "C" i2c_status_type ssd1306_clear(void)
{
    for (uint16_t i = 0; i < sizeof(framebuffer); ++i)
        framebuffer[i] = 0;

    return ssd1306_flush();
}

extern "C" i2c_status_type ssd1306_fill(uint8_t value)
{
    for (uint16_t i = 0; i < sizeof(framebuffer); ++i)
        framebuffer[i] = value;

    return ssd1306_flush();
}

i2c_status_type ssd1306_test_pattern(void)
{
    /*
     * Force the entire display ON.
     *
     * 0xA5 = Entire Display ON
     * This bypasses display RAM completely.
     */
    return ssd1306_command(0xA5);
}