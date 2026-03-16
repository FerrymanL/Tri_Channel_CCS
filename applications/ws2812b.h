#ifndef __WS2812B_H__
#define __WS2812B_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t di_pin;
} ws2812b_config_t;

typedef enum {
    WS2812B_COLOR_OFF = 0,
    WS2812B_COLOR_RED,
    WS2812B_COLOR_GREEN,
    WS2812B_COLOR_BLUE,
    WS2812B_COLOR_YELLOW,
    WS2812B_COLOR_CYAN,
    WS2812B_COLOR_PURPLE,
    WS2812B_COLOR_WHITE,

    WS2812B_COLOR_MAX
} ws2812b_color_t;

static const uint8_t ws2812b_color_table[WS2812B_COLOR_MAX][3] = {
    /*  G     R     B  */
    {0x00, 0x00, 0x00},   // OFF
    {0x00, 0xFF, 0x00},   // RED
    {0xFF, 0x00, 0x00},   // GREEN
    {0x00, 0x00, 0xFF},   // BLUE
    {0xFF, 0xFF, 0x00},   // YELLOW
    {0xFF, 0x00, 0xFF},   // CYAN
    {0x00, 0xFF, 0xFF},   // PURPLE
    {0xFF, 0xFF, 0xFF},   // WHITE
};

typedef void *ws2812b_handle_t;

ws2812b_handle_t ws2812b_create(const ws2812b_config_t *config);

void ws2812b_set_color(ws2812b_handle_t handle, ws2812b_color_t color);

void ws2812b_led_ctrl(ws2812b_handle_t handle, rt_bool_t state);

int32_t ws2812b_destroy(ws2812b_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
