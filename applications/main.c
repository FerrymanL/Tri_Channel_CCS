#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#include <rtdevice.h>
#include <string.h>
#include <drivers/misc.h>
#include "msh.h"
#include "ad5541.h"
#include "ws2812b.h"

// #define LED0_PIN    GET_PIN(C, 11)

#define UART3_RX_BUF_SIZE    256
#define UART3_FRAME_BUF_SIZE 512

#define REF_VOLT           5.0f
#define MIN_OUTPUT_VOLT    0.0f
#define MAX_OUTPUT_VOLT    5.0f * 1000.0f

#define CHANNEL_1_RS    120.0f
#define CHANNEL_2_RS    120.0f
#define CHANNEL_3_RS    120.0f

#define MIN_OUTPUT_CURR(rs)    (((((MIN_OUTPUT_VOLT / 1000.0f) * 2.0f) - REF_VOLT) / rs) * 1000.0f)
#define MAX_OUTPUT_CURR(rs)    (((((MAX_OUTPUT_VOLT / 1000.0f) * 2.0f) - REF_VOLT) / rs) * 1000.0f)

#define CURRENT_TRANSFORM_FORMULA(curr, rs)    ((REF_VOLT - (curr * rs)) / 2.0f)

#define CHANNEL_1_CURR_K    1.0f
#define CHANNEL_1_CURR_B    0.0f

#define CHANNEL_2_CURR_K    1.0f
#define CHANNEL_2_CURR_B    0.0f

#define CHANNEL_3_CURR_K    1.0f
#define CHANNEL_3_CURR_B    0.0f

rt_sem_t uart3_rx_sem = NULL;
static char frame_buf[UART3_FRAME_BUF_SIZE] = {0};

static ad5541_handle_t ad5541_handle_tb[3] = {0};
static ad5541_config_t ad5541_config_tb[] = {
    {
        .dev_name = "ad5541_1",
        .bus_name = "spi3",
        .cs_pin   = GET_PIN(A, 15),
    },
    {
        .dev_name = "ad5541_2",
        .bus_name = "spi1",
        .cs_pin   = GET_PIN(A, 4),
    },
    {
        .dev_name = "ad5541_3",
        .bus_name = "spi2",
        .cs_pin   = GET_PIN(B, 12),
    },
};

static ws2812b_handle_t ws2812b_handle = NULL;
static ws2812b_config_t ws2812b_config = {
    .di_pin = GET_PIN(C, 11),
};

static rt_timer_t connect_timer = NULL;

static void connection_timeout_callback(void *parameter)
{
    ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_RED);
    rt_timer_stop(connect_timer);
}

static int host_connection_state(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: Connection=true/false\r\n");
        return -1;
    }

    if (!rt_strcmp(argv[1], "True")) {
        ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_GREEN);
        rt_timer_stop(connect_timer);
        rt_timer_start(connect_timer);
    } else if (!rt_strcmp(argv[1], "False")) {
        ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_RED);
        rt_timer_stop(connect_timer);
    } else {
        rt_kprintf("invalid para\r\n");
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(host_connection_state, Connection, set host connection state);

int main(void)
{
    for (uint32_t  i = 0; i < RT_ARRAY_SIZE(ad5541_handle_tb); i++) {
        ad5541_handle_tb[i] = ad5541_create(&ad5541_config_tb[i]);
        if (ad5541_handle_tb[i] == RT_NULL) {
            rt_kprintf("ad5541_%d create failed!\n", i + 1);
        }
        rt_thread_mdelay(500);
        ad5541_set_voltage(ad5541_handle_tb[i], 2.5f);
        rt_thread_mdelay(500);
        ad5541_set_voltage(ad5541_handle_tb[i], 2.5f);
        rt_thread_mdelay(500);
        ad5541_set_voltage(ad5541_handle_tb[i], 2.5f);
        rt_thread_mdelay(10);
    }

    ws2812b_handle = ws2812b_create(&ws2812b_config);
    ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_RED);

    connect_timer = rt_timer_create("conn_timer", connection_timeout_callback, RT_NULL, 3000, RT_TIMER_FLAG_ONE_SHOT);

    while (1) {
        ws2812b_led_ctrl(ws2812b_handle,  RT_TRUE);
        rt_thread_mdelay(1000);
        ws2812b_led_ctrl(ws2812b_handle, RT_FALSE);
        rt_thread_mdelay(1000);
    }
}

static int channel_output_curr_range_get(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: Range\n");
        return -1;
    }

    rt_kprintf("Channel 1 Current Range: [%.6f ~ %.6f] mA\n", MIN_OUTPUT_CURR(CHANNEL_1_RS), MAX_OUTPUT_CURR(CHANNEL_1_RS));
    rt_kprintf("Channel 2 Current Range: [%.6f ~ %.6f] mA\n", MIN_OUTPUT_CURR(CHANNEL_2_RS), MAX_OUTPUT_CURR(CHANNEL_2_RS));
    rt_kprintf("Channel 3 Current Range: [%.6f ~ %.6f] mA\n", MIN_OUTPUT_CURR(CHANNEL_3_RS), MAX_OUTPUT_CURR(CHANNEL_3_RS));

    return 0;
}
MSH_CMD_EXPORT_ALIAS(channel_output_curr_range_get, Range, get output current range of all channels);

static int ws2812b_set_current_color(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: Led=red/green/blue\r\n");
        return -1;
    }

    if (!rt_strcmp(argv[1], "red")) {
        ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_RED);
    } else if (!rt_strcmp(argv[1], "green")) {
        ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_GREEN);
    } else if (!rt_strcmp(argv[1], "blue")) {
        ws2812b_set_color(ws2812b_handle, WS2812B_COLOR_BLUE);
    } else {
        rt_kprintf("invalid para\r\n");
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ws2812b_set_current_color, Led, set led current color);

static int ad5541_1_volt_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: V1=<volt>\n");
        return -1;
    }

    float volt = (float)atof(argv[1]);
    if (volt < MIN_OUTPUT_VOLT || volt > MAX_OUTPUT_VOLT) {
        rt_kprintf("Voltage Out of Range [%.2f ~ %.2f] mV\r\n", MIN_OUTPUT_VOLT, MAX_OUTPUT_VOLT);
        return -1;
    }

    ad5541_set_voltage(ad5541_handle_tb[0], volt / 1000.0f);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ad5541_1_volt_set, V1, set channel 1 output voltage);

static int ad5541_2_volt_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: V2=<volt>\n");
        return -1;
    }

    float volt = (float)atof(argv[1]);
    if (volt < MIN_OUTPUT_VOLT || volt > MAX_OUTPUT_VOLT) {
        rt_kprintf("Voltage Out of Range [%.6f ~ %.6f] mV\r\n", MIN_OUTPUT_VOLT, MAX_OUTPUT_VOLT);
        return -1;
    }

    ad5541_set_voltage(ad5541_handle_tb[1], volt / 1000.0f);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ad5541_2_volt_set, V2, set channel 2 output voltage);

static int ad5541_3_volt_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: V3=<volt>\n");
        return -1;
    }

    float volt = (float)atof(argv[1]);
    if (volt < MIN_OUTPUT_VOLT || volt > MAX_OUTPUT_VOLT) {
        rt_kprintf("Voltage Out of Range [%.6f ~ %.6f] mV\r\n", MIN_OUTPUT_VOLT, MAX_OUTPUT_VOLT);
        return -1;
    }

    ad5541_set_voltage(ad5541_handle_tb[2], volt / 1000.0f);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ad5541_3_volt_set, V3, set channel 3 output voltage);

static int ad5541_1_curr_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: I1=<current>\n");
        return -1;
    }

    float curr = (float)atof(argv[1]);
    if (curr < MIN_OUTPUT_CURR(CHANNEL_1_RS) || curr > MAX_OUTPUT_CURR(CHANNEL_1_RS)) {
        rt_kprintf("Current Out of Range: [%.6f ~ %.6f] mA\n", MIN_OUTPUT_CURR(CHANNEL_1_RS), MAX_OUTPUT_CURR(CHANNEL_1_RS));
        return -1;
    }

    float curr_cal = (curr / 1000.0f) * CHANNEL_1_CURR_K + CHANNEL_1_CURR_B;
    float volt = CURRENT_TRANSFORM_FORMULA(curr_cal, CHANNEL_1_RS);

    ad5541_set_voltage(ad5541_handle_tb[0], volt);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ad5541_1_curr_set, I1, set channel 1 output current);

static int ad5541_2_curr_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: I2=<current>\n");
        return -1;
    }

    float curr = (float)atof(argv[1]);
    if (curr < MIN_OUTPUT_CURR(CHANNEL_2_RS) || curr > MAX_OUTPUT_CURR(CHANNEL_2_RS)) {
        rt_kprintf("Current Out of Range: [%.2f ~ %.2f] mA\n", MIN_OUTPUT_CURR(CHANNEL_2_RS), MAX_OUTPUT_CURR(CHANNEL_2_RS));
        return -1;
    }

    float curr_cal = (curr / 1000.0f) * CHANNEL_2_CURR_K + CHANNEL_2_CURR_B;
    float volt = CURRENT_TRANSFORM_FORMULA(curr_cal, CHANNEL_2_RS);

    ad5541_set_voltage(ad5541_handle_tb[1], volt);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ad5541_2_curr_set, I2, set channel 2 output current);

static int ad5541_3_curr_set(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: I3=<current>\n");
        return -1;
    }

    float curr = (float)atof(argv[1]);
    if (curr < MIN_OUTPUT_CURR(CHANNEL_3_RS) || curr > MAX_OUTPUT_CURR(CHANNEL_3_RS)) {
        rt_kprintf("Current Out of Range: [%.2f ~ %.2f] mA\n", MIN_OUTPUT_CURR(CHANNEL_3_RS), MAX_OUTPUT_CURR(CHANNEL_3_RS));
        return -1;
    }

    float curr_cal = (curr / 1000.0f) * CHANNEL_3_CURR_K + CHANNEL_3_CURR_B;
    float volt = CURRENT_TRANSFORM_FORMULA(curr_cal, CHANNEL_3_RS);

    ad5541_set_voltage(ad5541_handle_tb[2], volt);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ad5541_3_curr_set, I3, set channel 3 output current);

static rt_err_t uart3_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(uart3_rx_sem);

    return RT_EOK;
}

int32_t uart3_frame_data(const char *data, uint32_t length, const rt_device_t uart)
{
    static uint32_t frame_index = 0;

    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        if (byte == '\r' || byte == '\n') {
            if (frame_index < UART3_FRAME_BUF_SIZE) {
                frame_buf[frame_index++] = byte;
            }

            while ((i + 1) < length && (data[i + 1] == '\r' || data[i + 1] == '\n')) {
                i++;
                if (frame_index < UART3_FRAME_BUF_SIZE) {
                    frame_buf[frame_index++] = data[i];
                }
            }

            rt_device_write(uart, 0, frame_buf, frame_index);
            msh_exec(frame_buf, frame_index);
            rt_device_write(uart, 0, "exec_done\r\n", 11);
            frame_index = 0;
        } else {
            if (frame_index < UART3_FRAME_BUF_SIZE) {
                frame_buf[frame_index++] = byte;
            }
        }
    }

    return 0;
}

static void uart3_msh_exec_entry(void *parameter)
{
    uart3_rx_sem = rt_sem_create("uart3_rx", 1, RT_IPC_FLAG_PRIO);
    if (NULL == uart3_rx_sem) {
        rt_kprintf("uart3_rx sem create failed!\n");
        return;
    }

    rt_device_t uart3 = rt_device_find("uart3");
    if (!uart3) {
        rt_kprintf("uart3 not found!\n");
        return;
    }
    rt_device_open(uart3, RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_INT_TX);

    rt_device_set_rx_indicate(uart3, uart3_rx_ind);

    uint32_t length = 0;
    char buffer[UART3_RX_BUF_SIZE] = {0};

    while (1) {
        rt_sem_take(uart3_rx_sem, RT_WAITING_FOREVER);

        do {
            length = rt_device_read(uart3, 0, buffer, UART3_RX_BUF_SIZE);
            if (length > 0) {
                uart3_frame_data(buffer, length, uart3);
            }
        } while(length > 0);
    }
}

int uart3_msh_exec(void)
{
    rt_thread_t tid = rt_thread_create("uart3_msh", uart3_msh_exec_entry, RT_NULL, 4096, 20, 10);
    if (tid)
        rt_thread_startup(tid);

    return 0;
}
INIT_APP_EXPORT(uart3_msh_exec);
