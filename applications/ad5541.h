#ifndef __AD5541_H__
#define __AD5541_H__

#include <stdint.h>
#include "rtconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *dev_name;
    char *bus_name;
    uint16_t cs_pin;
} ad5541_config_t;

typedef void *ad5541_handle_t;


ad5541_handle_t ad5541_create(const ad5541_config_t *config);

int32_t ad5541_set_voltage(ad5541_handle_t handle, float voltage);

int32_t ad5541_destroy(ad5541_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* __AD5541_H__ */
