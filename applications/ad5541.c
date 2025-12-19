#include <rtthread.h>
#include <rtdevice.h>
#include "ad5541.h"
#include "drv_spi.h"

#define LOG_TAG "ad5541"
#define LOG_LVL LOG_LVL_WARNING
#include "ulog.h"

typedef struct {
    char *dev_name;
    char *bus_name;
    uint16_t cs_pin;
    struct rt_spi_device *spi_device;
} ad5541_dev_t;

static inline int32_t is_vaild_config(const ad5541_config_t *config)
{
    if (RT_NULL == config) {
        LOG_E("invalid null pointer: config");
        return -1;
    }

    if (RT_NULL == config->bus_name) {
        LOG_E("invalid null pointer: bus_name");
        return -1;
    }

    if (RT_NULL == config->dev_name) {
        LOG_E("invalid null pointer: dev_name");
        return -1;
    }

    return 0;
}

int32_t ad5541_set_voltage(ad5541_handle_t handle, float voltage)
{
    if (RT_NULL == handle) {
        LOG_E("invalid null pointer: handle");
        return -1;
    }

    if (voltage < 0.0f || voltage > 5.0f) {
        LOG_E("invalid voltage value: %f", voltage);
        return -2;
    }

    ad5541_dev_t *pdev = (ad5541_dev_t *)handle;

    uint16_t dac_code = (uint16_t)((voltage / 5.0f) * 65535.0f);
    dac_code = (dac_code << 8) | (dac_code >> 8);

    if (2 != rt_spi_send(pdev->spi_device, &dac_code, 2)) {
        LOG_E("ad5541 set voltage failed");
        return -1;
    }

    return 0;
}

ad5541_handle_t ad5541_create(const ad5541_config_t *config)
{
    if (0 != is_vaild_config(config)) {
        LOG_E("invalid config param: config");
        return RT_NULL;
    }

    ad5541_dev_t *pdev = (ad5541_dev_t *)rt_malloc(sizeof(ad5541_dev_t));
    if (RT_NULL == pdev) {
        LOG_E("rt_malloc is failed");
        return RT_NULL;
    }
    pdev->bus_name = config->bus_name;
    pdev->cs_pin = config->cs_pin;
    pdev->dev_name = config->dev_name;
    pdev->spi_device = (struct rt_spi_device *)rt_device_find(config->bus_name);
    if (pdev->spi_device == RT_NULL) {
        LOG_E("ad5541 not find spi bus name: %s", config->bus_name);
        rt_free(pdev);
        return RT_NULL;
    }

    if (0 != rt_hw_spi_device_attach(config->bus_name, config->dev_name, config->cs_pin)) {
        LOG_E("ad5541 spi device attach fail");
        rt_free(pdev);
        return RT_NULL;
    }

    pdev->spi_device = (struct rt_spi_device *)rt_device_find(config->dev_name);
    if (pdev->spi_device == RT_NULL) {
        LOG_E("ad5541 create device name %s fail", config->dev_name);
        rt_free(pdev);
        return RT_NULL;
    }

    struct rt_spi_configuration spi_config = {
        .data_width = 8,
        .mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB,
        .max_hz = 1 * 1000 * 1000
    };
    if (0 != rt_spi_configure(pdev->spi_device, &spi_config)) {
        LOG_E("spi configure failed");
        rt_free(pdev);
        return RT_NULL;
    }

    ad5541_set_voltage((ad5541_handle_t)pdev, 2.5f);

    return (ad5541_handle_t)pdev;
}

int32_t ad5541_destroy(ad5541_handle_t handle)
{
    if (RT_NULL == handle) {
        LOG_E("ad5541_destroy input handle is RT_NULL");
        return -1;
    }

    rt_free(handle);
    return 0;
}
