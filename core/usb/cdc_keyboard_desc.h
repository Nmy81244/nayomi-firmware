#ifndef NAYOMI_CDC_KEYBOARD_DESC_H
#define NAYOMI_CDC_KEYBOARD_DESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdc_keyboard_class.h"
#include "usbd_core.h"

#define VCPKYBRD_BCD_NUM                     0x0110

/*
 * Temporary development IDs.
 * Replace these before any public/shipped Nayomi build with an owned VID/PID.
 */
#define USBD_VCPKYBRD_VENDOR_ID              0xCAFE
#define USBD_VCPKYBRD_PRODUCT_ID             0x4001

#define USBD_VCPKYBRD_CONFIG_DESC_SIZE      108
#define USBD_VCPKYBRD_HID_SIZ_REPORT_DESC    63
#define USBD_VCPKYBRD_SIZ_STRING_LANGID      4
#define USBD_VCPKYBRD_SIZ_STRING_SERIAL      0x1A

#define USBD_VCPKYBRD_DESC_MANUFACTURER_STRING    "Naomy"
#define USBD_VCPKYBRD_DESC_PRODUCT_STRING         "Nayomi"
#define USBD_VCPKYBRD_DESC_CONFIGURATION_STRING   "Nayomi Configuration"
#define USBD_VCPKYBRD_DESC_INTERFACE_STRING       "Nayomi Composite Interface"

/* USB 2.0 HS interrupt interval:
 * bInterval=1 => one 125 us microframe.
 */
#define VCPKYBRD_HID_BINTERVAL_TIME           0x01
#define VCPKYBRD_HS_HID_BINTERVAL_TIME        0x01

#define VCPKYBRD_CDC_INTERFACE                0x00
#define VCPKYBRD_CDC_DATA_INTERFACE           0x01
#define VCPKYBRD_KEYBOARD_INTERFACE           0x02

#define MCU_ID1                               (0x1FFFF7E8)
#define MCU_ID2                               (0x1FFFF7EC)
#define MCU_ID3                               (0x1FFFF7F0)

extern uint8_t g_usbd_vcpkybrd_hid_report[USBD_VCPKYBRD_HID_SIZ_REPORT_DESC];
extern uint8_t g_vcpkybrd_hid_usb_desc[9];
extern usbd_desc_handler cdc_keyboard_desc_handler;

#ifdef __cplusplus
}
#endif

#endif
