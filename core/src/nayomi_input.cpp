#include "nayomi_input.h"

#include <at32f402_405.h>

#include "board_pinout.h"
#include "nayomi_usb.h"

extern volatile uint16_t hall_raw;
extern volatile uint32_t hall_millivolts;

namespace
{
/*
 * Calibration is deliberately runtime-only for now.
 *
 * Each key starts with both endpoints at its first observed Hall reading.
 * New extremes pull the corresponding endpoint outward. The pull is stronger
 * when the new sample is farther outside the current calibration envelope.
 *
 * No assumption is made about which direction means "pressed".
 */
constexpr uint16_t DIRECTION_LOCK_DISTANCE = 128;

constexpr uint16_t ACTUATION_TRAVEL = 500; // 50% of learned travel
constexpr uint16_t RT_PRESS_TRAVEL  = 50;  // 5%
constexpr uint16_t RT_RELEASE_TRAVEL = 50; // 5%

struct KeyState
{
    uint16_t low = 0;
    uint16_t high = 0;
    uint16_t boot_raw = 0;

    bool initialized = false;
    bool direction_locked = false;
    bool pressed_toward_low = false;

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
 * Move an endpoint toward a newly observed extreme.
 *
 * The endpoint never jumps all the way to one noisy sample. A sample that is
 * only slightly outside the envelope moves the endpoint slowly; a sample far
 * outside the envelope pulls it more strongly.
 */
uint16_t pull_endpoint(uint16_t endpoint, uint16_t extreme, bool toward_high)
{
    const uint16_t distance = toward_high
        ? static_cast<uint16_t>(extreme - endpoint)
        : static_cast<uint16_t>(endpoint - extreme);

    if(distance == 0)
        return endpoint;

    uint16_t step = static_cast<uint16_t>(1U + (distance / 16U));

    if(step > 32U)
        step = 32U;

    if(step > distance)
        step = distance;

    if(toward_high)
        return static_cast<uint16_t>(endpoint + step);

    return static_cast<uint16_t>(endpoint - step);
}

void update_calibration(uint16_t raw)
{
    if(!key1.initialized)
    {
        key1.low = raw;
        key1.high = raw;
        key1.boot_raw = raw;
        key1.initialized = true;
        return;
    }

    if(raw < key1.low)
        key1.low = pull_endpoint(key1.low, raw, false);
    else if(raw > key1.high)
        key1.high = pull_endpoint(key1.high, raw, true);

    if(key1.direction_locked)
        return;

    const int32_t delta =
        static_cast<int32_t>(raw) -
        static_cast<int32_t>(key1.boot_raw);

    if(delta <= -static_cast<int32_t>(DIRECTION_LOCK_DISTANCE))
    {
        key1.direction_locked = true;
        key1.pressed_toward_low = true;
    }
    else if(delta >= static_cast<int32_t>(DIRECTION_LOCK_DISTANCE))
    {
        key1.direction_locked = true;
        key1.pressed_toward_low = false;
    }
}

uint16_t raw_to_travel(uint16_t raw)
{
    const uint16_t span = static_cast<uint16_t>(key1.high - key1.low);

    if(!key1.initialized ||
       !key1.direction_locked ||
       span < DIRECTION_LOCK_DISTANCE)
    {
        return 0;
    }

    if(key1.pressed_toward_low)
    {
        const int32_t travel =
            (static_cast<int32_t>(key1.high) - raw) * 1000 /
            static_cast<int32_t>(span);

        return clamp_travel(travel);
    }

    const int32_t travel =
        (static_cast<int32_t>(raw) - key1.low) * 1000 /
        static_cast<int32_t>(span);

    return clamp_travel(travel);
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
    if(!key1.direction_locked)
    {
        key1.pressed = false;
        key1.peak_travel = 0;
        key1.release_reference = 0;
    }
    else if(!key1.pressed)
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

    update_calibration(hall_raw);

    const uint16_t travel = raw_to_travel(hall_raw);
    update_key(travel);

    /* Send the current keyboard state when the HID endpoint is ready. */
    nayomi_usb_keyboard_send_report(keyboard_report, sizeof(keyboard_report));
}
