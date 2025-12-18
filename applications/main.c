#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#include <rtdevice.h>
#include <string.h>
#include "ad5541.h"

#define LED0_PIN           GET_PIN(C, 11)

static ad5541_handle_t ad5541_1_handle[3] = {0};

int main(void)
{
    ad5541_config_t ad5541_1_config = {
        .dev_name = "ad5541_1",
        .bus_name = "spi1",
        .cs_pin   = GET_PIN(A, 4),
    };

    ad5541_1_handle[0] = ad5541_create(&ad5541_1_config);
    if (ad5541_1_handle[0] == RT_NULL) {
        rt_kprintf("ad5541_1 create failed!\n");
    }

    ad5541_1_config.dev_name = "ad5541_2";
    ad5541_1_config.bus_name = "spi2";
    ad5541_1_config.cs_pin   = GET_PIN(B, 12);

    ad5541_1_handle[1] = ad5541_create(&ad5541_1_config);
    if (ad5541_1_handle[1] == RT_NULL) {
        rt_kprintf("ad5541_2 create failed!\n");
    }

    ad5541_1_config.dev_name = "ad5541_3";
    ad5541_1_config.bus_name = "spi3";
    ad5541_1_config.cs_pin   = GET_PIN(A, 15);

    ad5541_1_handle[2] = ad5541_create(&ad5541_1_config);
    if (ad5541_1_handle[2] == RT_NULL) {
        rt_kprintf("ad5541_3 create failed!\n");
    }

    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    while (1)
    {
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }
}

static void ad5541_volt_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: ad5541_volt_set <1|2|3> <volt>\n");
        return;
    }

    uint8_t chip_index = atoi(argv[1]);
    if(chip_index < 1 || chip_index > 3) {
        rt_kprintf("Usage: ad5541_volt_set <1|2|3> <volt>\n");
        return;
    }

    float volt = (float)atof(argv[2]);
    ad5541_set_voltage(ad5541_1_handle[chip_index - 1], volt);
}
MSH_CMD_EXPORT(ad5541_volt_set, set AD5541 output voltage);


static void ad5541_curr_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: ad5541_curr_set <1|2|3> <current>\n");
        return;
    }

    uint8_t chip_index = atoi(argv[1]);
    if(chip_index < 1 || chip_index > 3) {
        rt_kprintf("Usage: ad5541_curr_set <1|2|3> <current>\n");
        return;
    }

    float volt = (5.0f - (atof(argv[2]) / 1000.0f) * 120.0f) / 2.0f;
    ad5541_set_voltage(ad5541_1_handle[chip_index - 1], volt);
}
MSH_CMD_EXPORT(ad5541_curr_set, set AD5541 output current);
