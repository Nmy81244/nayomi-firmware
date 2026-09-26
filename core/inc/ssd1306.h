#ifndef SSD1306_H
#define SSD1306_H

#include <at32f402_405.h>
#include "i2c_application.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT    32
#define SSD1306_PAGES      4

/*
 * The physical SSD1306 address is 0x3C (7-bit).
 *
 * Artery's i2c_master_transmit() uses the address in the
 * shifted form, therefore:
 *
 *   0x3C << 1 = 0x78
 */
#define SSD1306_I2C_ADDRESS 0x78

/**
 * @brief Initialize the I2C peripheral and SSD1306.
 *
 * @return I2C_OK on success, otherwise an Artery I2C error code.
 */
i2c_status_type ssd1306_init(void);

/**
 * @brief Clear the framebuffer and display.
 */
i2c_status_type ssd1306_clear(void);

/**
 * @brief Fill the framebuffer with a byte pattern.
 *
 * @param value Byte written to every framebuffer byte.
 */
i2c_status_type ssd1306_fill(uint8_t value);

/**
 * @brief Set or clear one pixel in the framebuffer.
 *
 * @param x X coordinate, 0..127.
 * @param y Y coordinate, 0..31.
 * @param on Non-zero = pixel on, zero = pixel off.
 */
void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t on);

/**
 * @brief Send the complete framebuffer to the OLED.
 */
i2c_status_type ssd1306_flush(void);

/**
 * @brief Display an obvious hardware test pattern.
 */
i2c_status_type ssd1306_test_pattern(void);

#ifdef __cplusplus
}
#endif

#endif