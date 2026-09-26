#include "usb_core.h"
#include "usbd_int.h"

static otg_core_type *nayomi_otg_core(void);

extern otg_core_type *nayomi_otg_core_instance;

void OTGHS_IRQHandler(void)
{
    if(nayomi_otg_core_instance != 0)
        usbd_irq_handler(nayomi_otg_core_instance);
}
