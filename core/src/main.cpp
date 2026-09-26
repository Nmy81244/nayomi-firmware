#include <at32f402_405.h>
#include <at32f402_405_clock.h>

#include <cstdio>
#include <cstring>

#include "board_pinout.h"
#include "nayomi_usb.h"

volatile uint16_t hall_raw = 0;
volatile uint32_t hall_millivolts = 0;

static void hall_adc_init(void)
{
    adc_base_config_type adc_base_struct;

    crm_periph_clock_enable(HALL_1_ADC_CRM_CLK, TRUE);
    adc_clock_div_set(ADC_DIV_8);

    adc_base_default_para_init(&adc_base_struct);

    adc_base_struct.sequence_mode           = FALSE;
    adc_base_struct.repeat_mode             = FALSE;
    adc_base_struct.data_align              = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;

    adc_base_config(HALL_1_ADC, &adc_base_struct);

    adc_ordinary_channel_set(
        HALL_1_ADC,
        HALL_1_ADC_CHANNEL,
        1,
        ADC_SAMPLETIME_239_5
    );

    adc_ordinary_conversion_trigger_set(
        HALL_1_ADC,
        ADC12_ORDINARY_TRIG_SOFTWARE,
        TRUE
    );

    adc_enable(HALL_1_ADC, TRUE);

    adc_calibration_init(HALL_1_ADC);

    while(adc_calibration_init_status_get(HALL_1_ADC) != RESET)
    {
    }

    adc_calibration_start(HALL_1_ADC);

    while(adc_calibration_status_get(HALL_1_ADC) != RESET)
    {
    }
}

static uint16_t hall_read(void)
{
    adc_ordinary_software_trigger_enable(HALL_1_ADC, TRUE);

    while(adc_flag_get(HALL_1_ADC, ADC_CCE_FLAG) == RESET)
    {
    }

    uint16_t value = adc_ordinary_conversion_data_get(HALL_1_ADC);
    adc_flag_clear(HALL_1_ADC, ADC_CCE_FLAG);

    return value;
}

static void cli_write(const char *text)
{
    const uint16_t length = static_cast<uint16_t>(std::strlen(text));

    while(nayomi_usb_is_configured() &&
          !nayomi_usb_cdc_write(reinterpret_cast<const uint8_t *>(text), length))
    {
    }
}

static void cli_dump_rx(const uint8_t *data, uint16_t length)
{
    char buffer[192];
    int offset = std::snprintf(buffer, sizeof(buffer), "RX[%u]:", static_cast<unsigned>(length));

    if(offset < 0)
        return;

    const uint16_t limit = length < 32U ? length : 32U;

    for(uint16_t i = 0; i < limit && offset < static_cast<int>(sizeof(buffer) - 4); ++i)
    {
        offset += std::snprintf(
            buffer + offset,
            sizeof(buffer) - static_cast<size_t>(offset),
            " %02X",
            static_cast<unsigned>(data[i])
        );
    }

    if(length > limit && offset < static_cast<int>(sizeof(buffer) - 5))
        offset += std::snprintf(buffer + offset, sizeof(buffer) - static_cast<size_t>(offset), " ...");

    if(offset > 0)
    {
        buffer[static_cast<size_t>(offset)] = '\0';
        cli_write(buffer);
        cli_write("\r\n");
    }
}

static void cli_write_hall(void)
{
    char buffer[96];

    const int length = std::snprintf(
        buffer,
        sizeof(buffer),
        "hall1: raw=%u mV=%lu\r\n",
        static_cast<unsigned>(hall_raw),
        static_cast<unsigned long>(hall_millivolts)
    );

    if(length > 0)
    {
        const uint16_t send_length =
            static_cast<uint16_t>(length < static_cast<int>(sizeof(buffer))
                                      ? length
                                      : sizeof(buffer) - 1);

        while(nayomi_usb_is_configured() &&
              !nayomi_usb_cdc_write(
                  reinterpret_cast<const uint8_t *>(buffer),
                  send_length))
        {
        }
    }
}

static void cli_key_test(void)
{
    static const uint8_t key_a[8] =
    {
        0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    static const uint8_t key_release[8] = {0};

    while(nayomi_usb_is_configured() &&
          !nayomi_usb_keyboard_send_report(key_a, sizeof(key_a)))
    {
    }

    nayomi_usb_delay_ms(10);

    while(nayomi_usb_is_configured() &&
          !nayomi_usb_keyboard_send_report(key_release, sizeof(key_release)))
    {
    }
}

static void cli_process_command(char *command)
{
    // Normalize leading/trailing spaces so commands copied from a terminal
    // or script are accepted without surprising "unknown command" errors.
    while(*command == ' ' || *command == '\t')
        ++command;

    char *end = command + std::strlen(command);
    while(end > command && (end[-1] == ' ' || end[-1] == '\t'))
        --end;
    *end = '\0';

    if(std::strcmp(command, "help") == 0 || std::strcmp(command, "?") == 0)
    {
        cli_write(
            "commands:\r\n"
            "  help     - show this help\r\n"
            "  info     - show device information\r\n"
            "  hall     - read Hall sensor 1\r\n"
            "  key a    - send a keyboard A test report\r\n"
            "  reset    - reset the MCU\r\n"
        );
        return;
    }

    if(std::strcmp(command, "info") == 0)
    {
        cli_write(
            "Nayomi\r\n"
            "MCU: AT32F405CCT7\r\n"
            "USB: High-Speed composite HID + CDC\r\n"
            "Keyboard HID interval: 125 us\r\n"
        );
        return;
    }

    if(std::strcmp(command, "hall") == 0)
    {
        cli_write_hall();
        return;
    }

    if(std::strcmp(command, "key a") == 0)
    {
        cli_key_test();
        cli_write("sent A\r\n");
        return;
    }

    if(std::strcmp(command, "reset") == 0)
    {
        cli_write("resetting...\r\n");
        nayomi_usb_delay_ms(10);
        NVIC_SystemReset();
    }

    cli_write("unknown command; try 'help'\r\n");
}

static void cli_reset_input(void)
{
    // Never let a partial command survive a USB disconnect/reconnect.
    static char line[96];
    (void)line;
}

/*
 * CLI input state is kept separately from the command handler so it can be
 * reset when the USB connection changes.
 */
static char cli_line[96];
static uint16_t cli_line_length = 0;
static bool cli_ignore_lf_after_cr = false;

static void cli_reset_input_state(void)
{
    cli_line_length = 0;
    cli_ignore_lf_after_cr = false;
    cli_line[0] = '\0';
}

static void cli_task(void)
{
    uint8_t rx[256];

    const uint16_t length = nayomi_usb_cdc_read(rx, sizeof(rx));

    if(length != 0)
        cli_dump_rx(rx, length);

    for(uint16_t i = 0; i < length; ++i)
    {
        const uint8_t byte = rx[i];
        const char ch = static_cast<char>(byte);

        // Treat CRLF as one Enter key. This prevents a normal terminal's
        // "\\r\\n" line ending from generating a second empty prompt.
        if(ch == '\n' && cli_ignore_lf_after_cr)
        {
            cli_ignore_lf_after_cr = false;
            continue;
        }

        if(ch == '\r' || ch == '\n')
        {
            cli_ignore_lf_after_cr = (ch == '\r');

            if(cli_line_length != 0)
            {
                cli_line[cli_line_length] = '\0';
                cli_process_command(cli_line);
                cli_reset_input_state();
            }

            cli_write("nayomi> ");
            continue;
        }

        cli_ignore_lf_after_cr = false;

        if(ch == '\b' || ch == 0x7F)
        {
            if(cli_line_length != 0)
                --cli_line_length;
            continue;
        }

        // A CLI command is ASCII text. Drop NUL and other control bytes
        // instead of allowing a stray USB/TTY byte to poison the command.
        if(byte < 0x20 || byte > 0x7E)
            continue;

        if(cli_line_length < sizeof(cli_line) - 1)
            cli_line[cli_line_length++] = ch;
    }
}

int main(void)
{
    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

    system_clock_config();
    board_pinout_init();
    hall_adc_init();
    nayomi_usb_init();

    LED_OFF();

    bool was_configured = false;

    cli_reset_input_state();

    while(true)
    {
        hall_raw = hall_read();
        hall_millivolts =
            (static_cast<uint32_t>(hall_raw) * 3300U) / 4095U;

        const bool configured = nayomi_usb_is_configured();

        if(configured && !was_configured)
        {
            cli_reset_input_state();

            cli_write(
                "\r\n"
                "Nayomi USB online. Type 'help'.\r\n"
                "nayomi> "
            );
        }

        was_configured = configured;

        cli_task();
    }
}
