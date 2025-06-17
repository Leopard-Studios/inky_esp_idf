#ifndef INKY_UC8159_H
#define INKY_UC8159_H

#include "inky.h"
#include "eeprom.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include <inttypes.h>

//commands
#define UC8159_PSR       0x00
#define UC8159_PWR       0x01
#define UC8159_POF       0x02
#define UC8159_PFS       0x03
#define UC8159_PON       0x04
#define UC8159_BTST      0x06
#define UC8159_DSLP      0x07
#define UC8159_DTM1      0x10
#define UC8159_DSP       0x11
#define UC8159_DRF       0x12
#define UC8159_IPC       0x13
#define UC8159_PLL       0x30
#define UC8159_TSC       0x40
#define UC8159_TSE       0x41
#define UC8159_TSW       0x42
#define UC8159_TSR       0x43
#define UC8159_CDI       0x50
#define UC8159_LPD       0x51
#define UC8159_TCON      0x60
#define UC8159_TRES      0x61
#define UC8159_DAM       0x65
#define UC8159_REV       0x70
#define UC8159_FLG       0x71
#define UC8159_AMV       0x80
#define UC8159_VV        0x81
#define UC8159_VDCS      0x82
#define UC8159_PWS       0xE3
#define UC8159_TSSET     0xE5

#define _RESOLUTION_5_7_INCH    {600, 448}  // Inky Impression 5.7"
#define _RESOLUTION_4_INCH      {640, 400}    //# Inky Impression 4"

class Inky_UC8159: public Inky{
    
    public:
    ~Inky_UC8159(void);
    Inky_UC8159(
        resolution_t resolution, 
        gpio_num_t cs_pin,
        gpio_num_t dc_pin,
        gpio_num_t reset_pin, 
        gpio_num_t busy_pin,
        spi_host_device_t   spi_bus
    );
    esp_err_t setup();
    void setBorder(Colour_7_t colour);
    void drawPixel(int16_t x, int16_t y, uint16_t colour);
    void show();
    
    private:
    resolution_t _resolution;
    esp_err_t _init_hw();
    void _reset();
    esp_err_t _busy_wait(uint32_t timeout);
    void _send_command(uint8_t command, uint8_t *data, uint32_t data_len);
    void _send_command(uint8_t command);
    void _spi_transfer(uint8_t *data, uint32_t data_len);

    bool _gpio_setup = false;
    bool _h_flip;
    bool _v_flip;
    gpio_num_t _cs_pin;
    gpio_num_t _dc_pin;
    gpio_num_t _reset_pin;
    gpio_num_t _busy_pin;
    spi_host_device_t   _spi_bus;
    spi_device_handle_t _spi_dev = NULL;
    EPD_t _eeprom_data;
    Colour_7_t _border_colour = WHITE;
    uint8_t _resolution_setting;
    uint8_t **_buf;
    uint32_t _buf_len;
};
#endif