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
    WS2812B_COLOR_RED,
    WS2812B_COLOR_GREEN,
    WS2812B_COLOR_BLUE
} ws2812b_color_t;

typedef void *ws2812b_handle_t;

ws2812b_handle_t ws2812b_create(const ws2812b_config_t *config);

void ws2812b_led_ctrl(ws2812b_handle_t handle, ws2812b_color_t color, uint8_t state);

int32_t ws2812b_destroy(ws2812b_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
