#include "nayomi_input.h"

#include <at32f402_405.h>

#include "board_pinout.h"
#include "nayomi_usb.h"

extern volatile uint16_t hall_raw;
extern volatile uint32_t hall_millivolts;

namespace
{
constexpr uint16_t KEY_RELEASED_RAW = 1700;
constexpr uint16_t KEY_PRESSED_RAW  = 25;

constexpr uint16_t ACTUATION_TRAVEL = 500; // 50% travel
constexpr uint16_t RT_PRESS_TRAVEL  = 50;  // 5% travel
constexpr uint16_t RT_RELEASE_TRAVEL = 50; // 5% travel

struct KeyState
{
    bool pressed = false;
    uint16_t peak_travel = 0;
    uint16_t release_reference = 0;
};

KeyState key1;
uint8_t keyboard_report[8] = {0};

uint16_t clamp_travel(int32_t travel)
{
    if(travel < 0)
        return 0;
    if(travel > 1000)
        return 1000;
    return static_cast<uint16_t>(travel);
}

/*
 * Travel is defined semantically as:
 *   0    = released endpoint
 *   1000 = pressed endpoint
 *
 * The raw endpoints may be in either order, so the input algorithm never
 * assumes that pressing the key makes the ADC value increase or decrease.
 */
uint16_t raw_to_travel(uint16_t raw)
{
    const int32_t numerator =
        static_cast<int32_t>(raw) -
        static_cast<int32_t>(KEY_RELEASED_RAW);

    const int32_t denominator =
        static_cast<int32_t>(KEY_PRESSED_RAW) -
        static_cast<int32_t>(KEY_RELEASED_RAW);

    if(denominator == 0)
        return 0;

    return clamp_travel((numerator * 1000) / denominator);
}

uint16_t hall_sample(void)
{
    adc_ordinary_software_trigger_enable(HALL_1_ADC, TRUE);

    while(adc_flag_get(HALL_1_ADC, ADC_CCE_FLAG) == RESET)
    {
    }

    const uint16_t value = adc_ordinary_conversion_data_get(HALL_1_ADC);
    adc_flag_clear(HALL_1_ADC, ADC_CCE_FLAG);
    return value;
}

void update_key(uint16_t travel)
{
    if(!key1.pressed)
    {
        uint16_t required_travel = ACTUATION_TRAVEL;

        const uint16_t rapid_required = static_cast<uint16_t>(
            key1.release_reference + RT_PRESS_TRAVEL
        );

        if(rapid_required > required_travel)
            required_travel = rapid_required;

        if(travel >= required_travel)
        {
            key1.pressed = true;
            key1.peak_travel = travel;
        }
    }
    else
    {
        if(travel > key1.peak_travel)
            key1.peak_travel = travel;

        if(key1.peak_travel > travel &&
           key1.peak_travel - travel >= RT_RELEASE_TRAVEL)
        {
            key1.pressed = false;
            key1.release_reference = travel;
        }
    }

    keyboard_report[0] = 0;
    keyboard_report[1] = 0;
    keyboard_report[2] = key1.pressed ? 0x04 : 0x00; // HID A
    keyboard_report[3] = 0;
    keyboard_report[4] = 0;
    keyboard_report[5] = 0;
    keyboard_report[6] = 0;
    keyboard_report[7] = 0;
}
}

extern "C" void nayomi_input_reset(void)
{
    key1 = {};
    keyboard_report[0] = 0;
    keyboard_report[1] = 0;
    keyboard_report[2] = 0;
    keyboard_report[3] = 0;
    keyboard_report[4] = 0;
    keyboard_report[5] = 0;
    keyboard_report[6] = 0;
    keyboard_report[7] = 0;
}

extern "C" void nayomi_input_sof(void)
{
    if(!nayomi_usb_is_configured())
        return;

    hall_raw = hall_sample();
    hall_millivolts =
        (static_cast<uint32_t>(hall_raw) * 3300U) / 4095U;

    const uint16_t travel = raw_to_travel(hall_raw);
    update_key(travel);

    /*
     * The HS HID endpoint is polled every 125 us. Send the current keyboard
     * state on each microframe whenever the previous transfer has completed.
     */
    nayomi_usb_keyboard_send_report(keyboard_report, sizeof(keyboard_report));
}
