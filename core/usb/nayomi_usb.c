#include "nayomi_usb.h"

#include "usb_conf.h"
#include "usb_core.h"
#include "usbd_int.h"
#include "cdc_keyboard_class.h"
#include "cdc_keyboard_desc.h"

otg_core_type nayomi_otg_core_instance;

static uint32_t fac_us;

static void usb_clock48m_select(void)
{
    crm_pllu_output_set(TRUE);

    while(crm_flag_get(CRM_PLLU_STABLE_FLAG) != SET)
    {
    }

    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_PLLU);
}

static void usb_gpio_config(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(OTG_PIN_GPIO_CLOCK, TRUE);
    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode           = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;

#ifndef USB_VBUS_IGNORE
    gpio_init_struct.gpio_pins = OTG_PIN_VBUS;
    gpio_init_struct.gpio_pull = GPIO_PULL_DOWN;

    gpio_pin_mux_config(
        OTG_PIN_GPIO,
        OTG_PIN_VBUS_SOURCE,
        OTG_PIN_MUX
    );

    gpio_init(OTG_PIN_GPIO, &gpio_init_struct);
#endif
}

static void usb_delay_init(void)
{
    systick_clock_source_config(SYSTICK_CLOCK_SOURCE_AHBCLK_NODIV);
    fac_us = system_core_clock / 1000000U;
}

void nayomi_usb_delay_us(uint32_t us)
{
    uint32_t temp;

    if(fac_us == 0)
        usb_delay_init();

    SysTick->LOAD = (uint32_t)(us * fac_us);
    SysTick->VAL = 0U;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    do
    {
        temp = SysTick->CTRL;
    }
    while((temp & SysTick_CTRL_ENABLE_Msk) &&
          !(temp & SysTick_CTRL_COUNTFLAG_Msk));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0U;
}

void nayomi_usb_delay_ms(uint32_t ms)
{
    while(ms != 0)
    {
        const uint32_t step = (ms > 50U) ? 50U : ms;
        nayomi_usb_delay_us(step * 1000U);
        ms -= step;
    }
}

void nayomi_usb_init(void)
{
    usb_delay_init();
    usb_gpio_config();

    crm_periph_clock_enable(OTG_CLOCK, TRUE);
    usb_clock48m_select();

    nvic_irq_enable(OTG_IRQ, 0, 0);

    usbd_init(
        &nayomi_otg_core_instance,
        USB_SPEED_CORE_ID,
        USB_ID,
        &cdc_keyboard_class_handler,
        &cdc_keyboard_desc_handler
    );
}

int nayomi_usb_is_configured(void)
{
    return usbd_connect_state_get(&nayomi_otg_core_instance.dev) ==
           USB_CONN_STATE_CONFIGURED;
}

uint16_t nayomi_usb_cdc_read(uint8_t *buffer, uint16_t buffer_size)
{
    if(buffer == 0 || buffer_size == 0)
        return 0;

    uint16_t available = usb_vcpkybrd_vcp_get_rxdata(
        &nayomi_otg_core_instance.dev,
        buffer
    );

    if(available > buffer_size)
        available = buffer_size;

    return available;
}

int nayomi_usb_cdc_write(const uint8_t *buffer, uint16_t length)
{
    if(!nayomi_usb_is_configured() || buffer == 0 || length == 0)
        return 0;

    return usb_vcpkybrd_vcp_send_data(
        &nayomi_otg_core_instance.dev,
        (uint8_t *)buffer,
        length
    ) == SUCCESS;
}

int nayomi_usb_keyboard_send_report(const uint8_t *report, uint16_t length)
{
    if(!nayomi_usb_is_configured() || report == 0 || length == 0)
        return 0;

    extern vcp_keyboard_type vcp_keyboard_struct;

    if(vcp_keyboard_struct.g_keyboard_tx_completed == 0)
        return 0;

    return usb_vcpkybrd_class_send_report(
        &nayomi_otg_core_instance.dev,
        (uint8_t *)report,
        length
    ) == USB_OK;
}

