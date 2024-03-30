#ifndef RTC_BOOT_H
#define RTC_BOOT_H

#include <string.h>
#include <stdint.h>

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_channel.h"
#include "soc/soc.h"
#include "soc/soc_caps.h"
#include "soc/timer_group_struct.h"
#include "hal/mwdt_ll.h"
#include "hal/rtc_io_ll.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_err.h"

#include "sdkconfig.h"

static esp_err_t rtc_boot_init(void *user_fn)
{
    if(
        !(
            (user_fn >= (void*)SOC_RTC_IRAM_LOW && user_fn < (void*)SOC_RTC_IRAM_HIGH) ||
            (user_fn >= (void*)SOC_RTC_DATA_LOW && user_fn < (void*)SOC_RTC_DATA_HIGH)
        )
    )
    {
        ESP_LOGE("RTCBOOT", "user_fn (%p) not in RTC memory", user_fn);
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t rtc_boot_entry_program[] = {
        0x06, 0x03, 0x00,           // j start
        0x00,

        0x20, 0x00, 0x04, 0x00, // processor_state
        #define RTC_BOOT_MEM_FN_WORD 2
        0x00, 0x00, 0x00, 0x00, // user_fn (set to provided user_fn)
        0x20, 0x3f, 0xfe, 0x3f, // _stack

    // start:
        0x0c, 0x00,                 // movi.n a0 0
        0x0c, 0x11,                 // movi.n a1 1
        0x21, 0xFC, 0xFF,           // l32r a2 processor_state
        0x10, 0x49, 0x13,           // wsr a1 WindowStart
        0x00, 0x48, 0x13,           // wsr a0 WindowBase
        0x20, 0xe6, 0x13,           // wsr a2 PS
        0x10, 0x20, 0x00,           // rsync
        0x11, 0xFA, 0xFF,           // l32r a1 _stack
        0x31, 0xF8, 0xFF,           // l32r a3 user_fn
        0xD0, 0x03, 0x00,           // callx4 a3
    };
    #ifndef CONFIG_ULP_COPROC_RESERVE_MEM
        ESP_LOGE("RTCBOOT", "No reserved RTC Slow Memory. Enable ULP and set reserved memory to at least %u bytes. Continuing anyway...", sizeof(rtc_boot_entry_program));
    #else
        if(CONFIG_ULP_COPROC_RESERVE_MEM < sizeof(rtc_boot_entry_program))
        {
            ESP_LOGE("RTCBOOT", "Insufficient reserved RTC Slow Memory. Increase reserved memory to at least %u bytes.", sizeof(rtc_boot_entry_program));
        }
    #endif
    #define RTC_BOOT_MEM_ADDR 0x50000000
    memcpy((void*)RTC_BOOT_MEM_ADDR, rtc_boot_entry_program, sizeof(rtc_boot_entry_program));
    ((uint32_t*)RTC_BOOT_MEM_ADDR)[RTC_BOOT_MEM_FN_WORD] = (uint32_t)user_fn;
    REG_CLR_BIT(RTC_CNTL_RESET_STATE_REG, RTC_CNTL_PROCPU_STAT_VECTOR_SEL);
    return ESP_OK;
}

FORCE_INLINE_ATTR __attribute__((unused, noreturn)) void rtc_boot_fn_continue_to_rom_bootloader(void)
{
    __asm__ __volatile__ ("j.l 0x40000400, a2" "\n");
    __builtin_unreachable();
}

FORCE_INLINE_ATTR __attribute__((unused)) void rtc_boot_fn_timg0_wdt_flashboot_mod_dis(void)
{
    mwdt_ll_write_protect_disable(&TIMERG0);
    mwdt_ll_set_flashboot_en(&TIMERG0, 0);
    mwdt_ll_write_protect_enable(&TIMERG0);
}

FORCE_INLINE_ATTR __attribute__((unused)) void rtc_boot_fn_rtcio_set(int gpio_num, int level)
{
    const int gpio_to_rtcio[SOC_GPIO_PIN_COUNT] = {
        RTCIO_GPIO0_CHANNEL,    //GPIO0
        -1,//GPIO1
        RTCIO_GPIO2_CHANNEL,    //GPIO2
        -1,//GPIO3
        RTCIO_GPIO4_CHANNEL,    //GPIO4
        -1,//GPIO5
        -1,//GPIO6
        -1,//GPIO7
        -1,//GPIO8
        -1,//GPIO9
        -1,//GPIO10
        -1,//GPIO11
        RTCIO_GPIO12_CHANNEL,   //GPIO12
        RTCIO_GPIO13_CHANNEL,   //GPIO13
        RTCIO_GPIO14_CHANNEL,   //GPIO14
        RTCIO_GPIO15_CHANNEL,   //GPIO15
        -1,//GPIO16
        -1,//GPIO17
        -1,//GPIO18
        -1,//GPIO19
        -1,//GPIO20
        -1,//GPIO21
        -1,//GPIO22
        -1,//GPIO23
        -1,//GPIO24
        RTCIO_GPIO25_CHANNEL,   //GPIO25
        RTCIO_GPIO26_CHANNEL,   //GPIO26
        RTCIO_GPIO27_CHANNEL,   //GPIO27
        -1,//GPIO28
        -1,//GPIO29
        -1,//GPIO30
        -1,//GPIO31
        RTCIO_GPIO32_CHANNEL,   //GPIO32
        RTCIO_GPIO33_CHANNEL,   //GPIO33
        RTCIO_GPIO34_CHANNEL,   //GPIO34
        RTCIO_GPIO35_CHANNEL,   //GPIO35
        RTCIO_GPIO36_CHANNEL,   //GPIO36
        RTCIO_GPIO37_CHANNEL,   //GPIO37
        RTCIO_GPIO38_CHANNEL,   //GPIO38
        RTCIO_GPIO39_CHANNEL,   //GPIO39
    };
    rtcio_ll_set_level(gpio_to_rtcio[gpio_num], level);
}

#endif
