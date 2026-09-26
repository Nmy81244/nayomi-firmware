#include "usb_core.h"
#include "usbd_int.h"

extern otg_core_type nayomi_otg_core_instance;

void OTGHS_IRQHandler(void)
{
    usbd_irq_handler(&nayomi_otg_core_instance);
}
