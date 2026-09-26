#include <at32f402_405.h>

#include "board_pinout.h"

volatile uint16_t hall_raw = 0;
volatile uint32_t hall_millivolts = 0;

static void hall_adc_init(void)
{
    adc_base_config_type adc_base_struct;

    /* ADC peripheral clock */
    crm_periph_clock_enable(HALL_1_ADC_CRM_CLK, TRUE);

    /*
     * HCLK / 8
     * At 216 MHz HCLK this gives 27 MHz ADC clock,
     * which is within the ADC clock limit.
     */
    adc_clock_div_set(ADC_DIV_8);

    /* Basic ADC configuration */
    adc_base_default_para_init(&adc_base_struct);

    adc_base_struct.sequence_mode           = FALSE;
    adc_base_struct.repeat_mode             = FALSE;
    adc_base_struct.data_align              = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;

    adc_base_config(HALL_1_ADC, &adc_base_struct);

    /* PA0 = ADC channel 0 */
    adc_ordinary_channel_set(
        HALL_1_ADC,
        HALL_1_ADC_CHANNEL,
        1,
        ADC_SAMPLETIME_239_5
    );

    /* Software-triggered conversion */
    adc_ordinary_conversion_trigger_set(
        HALL_1_ADC,
        ADC12_ORDINARY_TRIG_SOFTWARE,
        TRUE
    );

    /* Enable ADC */
    adc_enable(HALL_1_ADC, TRUE);

    /* ADC calibration */
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
    /* Start one conversion */
    adc_ordinary_software_trigger_enable(
        HALL_1_ADC,
        TRUE
    );

    /* Wait for conversion complete */
    while(adc_flag_get(HALL_1_ADC, ADC_CCE_FLAG) == RESET)
    {
    }

    /* Read result */
    uint16_t value =
        adc_ordinary_conversion_data_get(HALL_1_ADC);

    /* Clear completion flag */
    adc_flag_clear(HALL_1_ADC, ADC_CCE_FLAG);

    return value;
}

int main(void)
{
    board_pinout_init();

    hall_adc_init();

    while(true)
    {
        hall_raw = hall_read();

        /*
         * Assuming the ADC reference/input range is 0–3.3 V:
         *
         * 0     = 0 V
         * 4095  = 3.3 V
         */
        hall_millivolts =
            ((uint32_t)hall_raw * 3300U) / 4095U;
    }
}