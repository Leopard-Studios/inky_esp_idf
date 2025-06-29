#ifndef INKY_H
#define INKY_H
#include <inttypes.h>

#include <Adafruit_GFX.h>
#include "eeprom.h"
#include "esp_err.h"
#include "driver/spi_master.h"

#define NUM_DISPLAY_VAR 23
typedef enum {
    None,
    Red_pHAT_HT = 1, //High-Temp
    Yellow_wHAT,
    Black_wHAT,
    Black_pHAT,
    Yellow_pHAT,
    Red_wHAT,
    Red_wHAT_HT, //High-temp
    Red_wHAT_2,
    // None,
    Black_pHAT_SSD1608 = 10,
    Red_pHAT_SSD1608,
    Yellow_pHAT_SSD1608,
    // None,
    _7_Colour_UC8159 = 14,
    _7_Colour_640x400_UC8159,
    _7_Colour_640x400_UC8159_2,
    Black_wHAT_SSD1683,
    Red_wHAT_SSD1683,
    Yellow_wHAT_SSD1683,
    _7_Colour_800x480_AC073TC1A,
    Spectra_6_13_3_1600x1200_EL133UF1,
    Spectra_6_7_3_800x480_E673
} Display_Var_t ;

const char * const DisplayVars[] = {
    NULL,
    "Red pHAT (High-Temp)",
    "Yellow wHAT",
    "Black wHAT",
    "Black pHAT",
    "Yellow pHAT",
    "Red wHAT",
    "Red wHAT (High-Temp)",
    "Red wHAT",
    NULL,
    "Black pHAT (SSD1608)",
    "Red pHAT (SSD1608)",
    "Yellow pHAT (SSD1608)",
    NULL,
    "7-Colour (UC8159)",
    "7-Colour 640x400 (UC8159)",
    "7-Colour 640x400 (UC8159)",
    "Black wHAT (SSD1683)",
    "Red wHAT (SSD1683)",
    "Yellow wHAT (SSD1683)",
    "7-Colour 800x480 (AC073TC1A)",
    "Spectra 6 13.3 1600 x 1200 (EL133UF1)",
    "Spectra 6 7.3 800 x 480 (E673)"
};

typedef struct{
    uint16_t width;
    uint16_t height;
}resolution_t;

// enum class Colour_7_t : uint16_t {
//     BLACK = 0,
//     WHITE,
//     GREEN,
//     BLUE,
//     RED,
//     YELLOW,
//     ORANGE,
//     MAX
// };


// enum class Colour_6_Spectra_t : uint16_t {
//     BLACK = 0,
//     WHITE = 1,
//     YELLOW = 2,
//     RED = 3,
//     BLUE = 5,
//     GREEN = 6,
//     MAX
// } ;

class Inky : public virtual Adafruit_GFX{
    public:
        ~Inky();
        virtual esp_err_t setup() = 0;
        virtual void show() = 0;
        Display_Var_t DISPLAY_VAR = Display_Var_t::None;


        // 8bit variants
        void 
        drawRGBBitmap(int16_t x, int16_t y, 
                const uint8_t bitmap[], 
                int16_t w, int16_t h),
        drawRGBBitmap(int16_t x, int16_t y,
                uint8_t *bitmap, 
                int16_t w, int16_t h);
        // drawRGBBitmap(int16_t x, int16_t y, 
        //         const uint8_t bitmap[], const uint8_t mask[], 
        //         int16_t w, int16_t h),
        // drawRGBBitmap(int16_t x, int16_t y, 
        //         uint8_t *bitmap, uint8_t *mask,
        //         int16_t w, int16_t h);

    protected:
        gpio_num_t _cs_pin;
        gpio_num_t _dc_pin;
        gpio_num_t _reset_pin;
        gpio_num_t _busy_pin;
    
        // resolution_t _resolution;
        spi_host_device_t   _spi_bus;
        spi_device_handle_t _spi_dev = NULL;
        bool _gpio_setup = false;
        resolution_t _resolution;
        esp_err_t _init_hw();
        esp_err_t _busy_wait(uint32_t timeout);
        void _spi_transfer(uint8_t *data, uint32_t data_len);
        void _send_command
            (uint8_t command, uint8_t *data, uint32_t data_len);
        void _send_command(uint8_t command);
        void _send_data
            (uint8_t *data, uint32_t data_len);
    };  

Inky * Auto(        
    i2c_master_bus_handle_t i2c_bus,
    spi_host_device_t   spi_bus,
    gpio_num_t cs_pin,
    gpio_num_t dc_pin,
    gpio_num_t reset_pin, 
    gpio_num_t busy_pin
);

#endif
