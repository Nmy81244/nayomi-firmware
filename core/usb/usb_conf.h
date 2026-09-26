#ifndef NAYOMI_USB_CONF_H
#define NAYOMI_USB_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "at32f402_405.h"
#include "at32f402_405_usb.h"

/* F405 USB HS device configuration, following Artery's official
 * composite CDC + keyboard example.
 */
#define USE_OTG_DEVICE_MODE

#ifdef USB_OTG_HS

#define USB_ID                           USB_OTG2_ID
#define OTG_CLOCK                        CRM_OTGHS_PERIPH_CLOCK
#define OTG_IRQ                          OTGHS_IRQn
#define OTG_IRQ_HANDLER                  OTGHS_IRQHandler
#define OTG_WKUP_IRQ                     OTGHS_WKUP_IRQn
#define OTG_WKUP_HANDLER                 OTGHS_WKUP_IRQHandler
#define OTG_WKUP_EXINT_LINE              EXINT_LINE_20

#define OTG_PIN_GPIO                     GPIOB
#define OTG_PIN_GPIO_CLOCK               CRM_GPIOB_PERIPH_CLOCK
#define OTG_PIN_VBUS                     GPIO_PINS_13
#define OTG_PIN_VBUS_SOURCE              GPIO_PINS_SOURCE13
#define OTG_PIN_ID                       GPIO_PINS_12
#define OTG_PIN_ID_SOURCE                GPIO_PINS_SOURCE12

#define OTG_PIN_SOF_GPIO                GPIOA
#define OTG_PIN_SOF_GPIO_CLOCK          CRM_GPIOA_PERIPH_CLOCK
#define OTG_PIN_SOF                     GPIO_PINS_4
#define OTG_PIN_SOF_SOURCE              GPIO_PINS_SOURCE4

#define OTG_PIN_MUX                     GPIO_MUX_10
#define USB_SPEED_CORE_ID               USB_HIGH_SPEED_CORE_ID

#endif

#define USBD_RX_SIZE                     256
#define USBD_EP0_TX_SIZE                 64
#define USBD_EP1_TX_SIZE                 256
#define USBD_EP2_TX_SIZE                 20
#define USBD_EP3_TX_SIZE                 20
#define USBD_EP4_TX_SIZE                 20
#define USBD_EP5_TX_SIZE                 20
#define USBD_EP6_TX_SIZE                 20
#define USBD_EP7_TX_SIZE                 20

#define USBD2_RX_SIZE                    256
#define USBD2_EP0_TX_SIZE                64
#define USBD2_EP1_TX_SIZE                256
#define USBD2_EP2_TX_SIZE                20
#define USBD2_EP3_TX_SIZE                20
#define USBD2_EP4_TX_SIZE                20
#define USBD2_EP5_TX_SIZE                20
#define USBD2_EP6_TX_SIZE                20
#define USBD2_EP7_TX_SIZE                20

#define USB_EPT_MAX_NUM                  8

/* The WeAct board is used as a USB device. We do not require firmware
 * to sample VBUS through the OTG VBUS pin.
 */
#define USB_VBUS_IGNORE

#define usb_delay_ms(ms) nayomi_usb_delay_ms(ms)
#define usb_delay_us(us) nayomi_usb_delay_us(us)

void nayomi_usb_delay_ms(uint32_t ms);
void nayomi_usb_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif
