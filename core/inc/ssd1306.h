#ifndef SSD1306_H
#define SSD1306_H

#include <at32f402_405.h>
#include "i2c_application.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT   32

void ssd1306_init(void);
i2c_status_type ssd1306_test_pattern(void);

#endif