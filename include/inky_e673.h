#ifndef INKY_E673_h
#define INKY_E673_h

#include "inky.h"
#include "eeprom.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include <inttypes.h>

#define _RESOLUTION_7_3_INCH {800, 480}

#define EL673_PSR   0x00
#define EL673_PWR   0x01
#define EL673_POF   0x02
#define EL673_POFS  0x03
#define EL673_PON   0x04
#define EL673_BTST1 0x05
#define EL673_BTST2 0x06
#define EL673_DSLP  0x07
#define EL673_BTST3 0x08
#define EL673_DTM1  0x10
#define EL673_DSP   0x11
#define EL673_DRF   0x12
#define EL673_PLL   0x30
#define EL673_CDI   0x50
#define EL673_TCON  0x60
#define EL673_TRES  0x61
#define EL673_REV   0x70
#define EL673_VDCS  0x82
#define EL673_CMDH  0xAA
#define EL673_PWS   0xE3

class Inky_E673: public Inky{

    public:
    ~Inky_E673(void);
    Inky_E673(
        resolution_t resolution, 
        gpio_num_t cs_pin,
        gpio_num_t dc_pin,
        gpio_num_t reset_pin, 
        gpio_num_t busy_pin,
        spi_host_device_t   spi_bus
    );

    esp_err_t setup();
    void setBorder(int16_t colour);
    void drawPixel(int16_t x, int16_t y, uint16_t colour);
    void show();
    void clear(uint16_t colour);
    void refresh();
    enum Colour {
        BLACK = 0,
        WHITE = 1,
        YELLOW = 2,
        RED = 3,
        BLUE = 5,
        GREEN = 6,
        MAX
    };

    Display_Var_t DISPLAY_VAR = Display_Var_t::Spectra_6_7_3_800x480_E673;

    private:
        uint8_t **_buf;
        uint32_t _buf_len;

        void _reset();

        void _magic_pimoroni();
        void _magic_waveshare();
        // esp_err_t _busy_wait(uint32_t timeout);
        // void _send_command(uint8_t command, uint8_t *data, uint32_t data_len);
        // void _send_command(uint8_t command);
        // void _spi_transfer(uint8_t *data, uint32_t data_len);
};

#endif