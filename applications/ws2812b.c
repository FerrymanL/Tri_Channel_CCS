#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "ws2812b.h"

#define LOG_TAG "ws2812b"
#define LOG_LVL LOG_LVL_WARNING
#include <ulog.h>

typedef struct {
    uint16_t di_pin;

    GPIO_TypeDef *port;
    uint16_t pin;
} ws2812b_dev_t;

static inline void ws2812b_delay_ns(uint32_t ns)
{
    volatile uint32_t cycles = (ns * 168) / 1000;

    while (cycles--) {
        __nop();
    }
}

__STATIC_FORCEINLINE void ws2812b_pin_high(ws2812b_dev_t *pdev)
{
    pdev->port->BSRR = pdev->pin;
}

__STATIC_FORCEINLINE void ws2812b_pin_low(ws2812b_dev_t *pdev)
{
    pdev->port->BSRR = (uint32_t)pdev->pin << 16;
}

static void ws2812b_reset(ws2812b_dev_t *pdev)
{
    ws2812b_pin_low(pdev);
    rt_hw_us_delay(60);
}

static void ws2812b_send_bit0(ws2812b_dev_t *pdev)
{
    ws2812b_pin_high(pdev);
    ws2812b_delay_ns(300);
    ws2812b_pin_low(pdev);
    ws2812b_delay_ns(1090);
}

static void ws2812b_send_bit1(ws2812b_dev_t *pdev)
{
    ws2812b_pin_high(pdev);
    ws2812b_delay_ns(1090);
    ws2812b_pin_low(pdev);
    ws2812b_delay_ns(320);
}

static void ws2812b_send_byte(ws2812b_dev_t *pdev, uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80)
            ws2812b_send_bit1(pdev);
        else
            ws2812b_send_bit0(pdev);

        data <<= 1;
    }
}

static void ws2812b_pin_parse(ws2812b_dev_t *pdev)
{
    uint16_t pin = pdev->di_pin;

    uint8_t port_index = pin / 16;
    uint8_t pin_index  = pin % 16;

    GPIO_TypeDef *ports[] = { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH};

    pdev->port = ports[port_index];
    pdev->pin  = 1 << pin_index;
}

static void ws2812b_gpio_init(ws2812b_dev_t *pdev)
{
    uint8_t pin_index = 0;

    uint16_t pin = pdev->pin;

    while(pin >>= 1)
        pin_index++;

    pdev->port->MODER &= ~(3 << (pin_index * 2));
    pdev->port->MODER |=  (1 << (pin_index * 2));

    pdev->port->OSPEEDR |= (3 << (pin_index * 2));

    pdev->port->BSRR = (uint32_t)pdev->pin << 16;
}

ws2812b_handle_t ws2812b_create(const ws2812b_config_t *config)
{
    ws2812b_dev_t *pdev = (ws2812b_dev_t *)rt_malloc(sizeof(ws2812b_dev_t));

    if (RT_NULL == pdev) {
        LOG_E("rt_malloc failed");
        return RT_NULL;
    }

    pdev->di_pin = config->di_pin;

    ws2812b_pin_parse(pdev);
    ws2812b_gpio_init(pdev);

    return (ws2812b_handle_t)pdev;
}

void ws2812b_led_ctrl(ws2812b_handle_t handle, ws2812b_color_t color, uint8_t state)
{
    if (RT_NULL == handle) {
        LOG_E("handle null");
        return;
    }

    ws2812b_dev_t *pdev = (ws2812b_dev_t *)handle;

    uint8_t rgb[3] = {0};

    if (state) {
        switch (color)
        {
            case WS2812B_COLOR_RED:
                rgb[1] = 0x00;
                break;

            case WS2812B_COLOR_GREEN:
                rgb[0] = 0x00;
                break;

            case WS2812B_COLOR_BLUE:
                rgb[2] = 0x00;
                break;

            default:
                LOG_E("invalid color");
                return;
        }
    }

    rt_base_t level = rt_hw_interrupt_disable();

    for (int i = 0; i < 3; i++)
        ws2812b_send_byte(pdev, rgb[i]);

    rt_hw_interrupt_enable(level);

    ws2812b_reset(pdev);
}

int32_t ws2812b_destroy(ws2812b_handle_t handle)
{
    if (RT_NULL == handle) {
        LOG_E("handle null");
        return -1;
    }

    rt_free(handle);

    return 0;
}
