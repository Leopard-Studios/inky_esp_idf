#ifndef INKY_DUMMY_H
#define INKY_DUMMY_H

#include "inky.h"
#include "eeprom.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include <inttypes.h>

class Inky_DUMMY: public Inky{

    public:
    ~Inky_DUMMY(void);
    Inky_DUMMY(
        resolution_t resolution, 
        gpio_num_t cs_pin,
        gpio_num_t dc_pin,
        gpio_num_t reset_pin, 
        gpio_num_t busy_pin,
        spi_host_device_t   spi_bus
    );
    esp_err_t setup();
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

    Display_Var_t DISPLAY_VAR = Display_Var_t::DUMMY;
    protected:
        uint8_t** _buf = nullptr;
        size_t _buf_len = 0;

};
#endif

