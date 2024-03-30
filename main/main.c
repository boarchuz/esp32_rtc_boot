/**
 * Example demonstrating the first method of RTC Boot (ESP32 TRM 31.3.13).
 *
 * "When the CPU is powered up, the reset vector starts from 0x50000000, instead of 0x40000400. ROM
 * unpacking & SPI boot are not needed. The code in RTC memory has to do itself some initialization
 * for the C program environment."
 *
 * The latency from wakeup trigger to function entry is approximately 510us.
 * For comparison, the more common second method takes approximately 1050us.
 *
 * Using rtc_boot_init(), a small initialisation program is loaded at 0x50000000 which then calls
 * the provided user function (see my_rtc_boot_function).
 *
 * Conveniently, ESP-IDF allows reserving memory at 0x50000000 by enabling the ULP coprocessor. To
 * avoid overwriting any variables or functions in RTC slow memory, ensure ULP_COPROC_RESERVE_MEM
 * is adequate (>=44 bytes) in menuconfig before running this example.
 *
 * In this example, a GPIO is toggled a few times (connect an LED for visualisation) and then
 * execution switches to the ROM bootloader for an otherwise-normal wakeup from deep sleep.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/rtc_io.h"

#include "rtc_boot.h"

#include "sdkconfig.h"

static const char *TAG = "rtc_boot";

#define RTC_BOOT_GPIO_PIN 25

static RTC_IRAM_ATTR __attribute__((noreturn)) void my_rtc_boot_function(void)
{
    // Unless this function has a very short execution time, MWDT0 flash boot protection should be disabled.
    rtc_boot_fn_timg0_wdt_flashboot_mod_dis();

    // Since ROM memory is not yet initialised, g_ticks_per_us_pro must be set before esp_rom_delay_us can be used.
    extern uint32_t g_ticks_per_us_pro;
    g_ticks_per_us_pro = 40;

    for(int i = 0; i < 3; ++i)
    {
        rtc_boot_fn_rtcio_set(RTC_BOOT_GPIO_PIN, 0);
        esp_rom_delay_us(250 * 1000);
        rtc_boot_fn_rtcio_set(RTC_BOOT_GPIO_PIN, 1);
        esp_rom_delay_us(250 * 1000);
    }

    // Jump to ROM bootloader entry
    rtc_boot_fn_continue_to_rom_bootloader();
}

void app_main(void)
{
    if(esp_rom_get_reset_reason(0) != RESET_REASON_CORE_DEEP_SLEEP)
    {
        ESP_LOGI(TAG, "Starting");
        rtc_gpio_set_level(RTC_BOOT_GPIO_PIN, 1);
        rtc_gpio_init(RTC_BOOT_GPIO_PIN);
        rtc_gpio_set_direction(RTC_BOOT_GPIO_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
        ESP_ERROR_CHECK(rtc_boot_init(my_rtc_boot_function));
    }
    else
    {
        ESP_LOGI(TAG, "Woken");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    #if 1
        // Timer wakeup...
        esp_deep_sleep(3 * 1000 * 1000);
    #else
        // ..or GPIO0 low (Boot/Prog button on dev board)
        esp_sleep_enable_ext1_wakeup(1 << GPIO_NUM_0, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_deep_sleep_start();
    #endif
}
