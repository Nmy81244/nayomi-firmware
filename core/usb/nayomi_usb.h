#ifndef NAYOMI_USB_H
#define NAYOMI_USB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void nayomi_usb_init(void);
int nayomi_usb_is_configured(void);

uint16_t nayomi_usb_cdc_read(uint8_t *buffer, uint16_t buffer_size);
int nayomi_usb_cdc_write(const uint8_t *buffer, uint16_t length);

int nayomi_usb_keyboard_send_report(const uint8_t *report, uint16_t length);

void nayomi_usb_delay_ms(uint32_t ms);
void nayomi_usb_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif
