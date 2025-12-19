#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#include <rtdevice.h>
#include <string.h>
#include <drivers/misc.h>
#include "msh.h"
#include "ad5541.h"

#define LED0_PIN    GET_PIN(C, 11)

#define UART3_RX_BUF_SIZE    256
#define UART3_FRAME_BUF_SIZE 512

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
        .bus_name = "spi1",
        .cs_pin   = GET_PIN(A, 4),
    },
    {
        .dev_name = "ad5541_2",
        .bus_name = "spi2",
        .cs_pin   = GET_PIN(B, 12),
    },
    {
        .dev_name = "ad5541_3",
        .bus_name = "spi3",
        .cs_pin   = GET_PIN(A, 15),
    },
};

int main(void)
{
    for (uint32_t  i = 0; i < RT_ARRAY_SIZE(ad5541_handle_tb); i++) {
        ad5541_handle_tb[i] = ad5541_create(&ad5541_config_tb[i]);
        if (ad5541_handle_tb[i] == RT_NULL) {
            rt_kprintf("ad5541_%d create failed!\n", i + 1);
        }
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

static void ad5541_1_volt_set(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: V1=<volt>\n");
        return;
    }

    float volt = (float)atof(argv[1]);
    ad5541_set_voltage(ad5541_handle_tb[0], volt);
}
MSH_CMD_EXPORT_ALIAS(ad5541_1_volt_set, V1, set channel 1 output voltage);

static void ad5541_2_volt_set(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: V2=<volt>\n");
        return;
    }

    float volt = (float)atof(argv[1]);
    ad5541_set_voltage(ad5541_handle_tb[1], volt);
}
MSH_CMD_EXPORT_ALIAS(ad5541_2_volt_set, V2, set channel 2 output voltage);

static void ad5541_3_volt_set(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: V3=<volt>\n");
        return;
    }

    float volt = (float)atof(argv[1]);
    ad5541_set_voltage(ad5541_handle_tb[2], volt);
}
MSH_CMD_EXPORT_ALIAS(ad5541_3_volt_set, V3, set channel 3 output voltage);

static void ad5541_1_curr_set(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: I1=<current>\n");
        return;
    }

    float curr = (atof(argv[1]) / 1000.0f) * CHANNEL_1_CURR_K + CHANNEL_1_CURR_B;

    float volt = (5.0f - curr * 120.0f) / 2.0f;
    ad5541_set_voltage(ad5541_handle_tb[0], volt);
}
MSH_CMD_EXPORT_ALIAS(ad5541_1_curr_set, I1, set channel 1 output current);

static void ad5541_2_curr_set(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: I2=<current>\n");
        return;
    }

    float curr = (atof(argv[1]) / 1000.0f) * CHANNEL_2_CURR_K + CHANNEL_2_CURR_B;

    float volt = (5.0f - curr * 120.0f) / 2.0f;
    ad5541_set_voltage(ad5541_handle_tb[1], volt);
}
MSH_CMD_EXPORT_ALIAS(ad5541_2_curr_set, I2, set channel 2 output current);

static void ad5541_3_curr_set(int argc, char **argv)
{
    if (argc < 1) {
        rt_kprintf("Usage: I3=<current>\n");
        return;
    }

    float curr = (atof(argv[1]) / 1000.0f) * CHANNEL_3_CURR_K + CHANNEL_3_CURR_B;

    float volt = (5.0f - curr * 120.0f) / 2.0f;
    ad5541_set_voltage(ad5541_handle_tb[2], volt);
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
