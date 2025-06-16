#ifndef INKY_EEPROM_H
#define INKY_EEPROM_H

#include <inttypes.h>
#include <string>

#include "driver/i2c_master.h"

#define EEPROM_ADDRESS 0x50

// std::string DISPLAY_VARIANT[] = {
//     "",
//     "Red pHAT (High-Temp)",
//     "Yellow wHAT",
//     "Black wHAT",
//     "Black pHAT",
//     "Yellow pHAT",
//     "Red wHAT",
//     "Red wHAT (High-Temp)",
//     "Red wHAT",
//     "",
//     "Black pHAT (SSD1608)",
//     "Red pHAT (SSD1608)",
//     "Yellow pHAT (SSD1608)",
//     "",
//     "7-Colour (UC8159)",
//     "7-Colour 640x400 (UC8159)",
//     "7-Colour 640x400 (UC8159)",
//     "Black wHAT (SSD1683)",
//     "Red wHAT (SSD1683)",
//     "Yellow wHAT (SSD1683)",
//     "7-Colour 800x480 (AC073TC1A)",
//     "Spectra 6 13.3 1600 x 1200 (EL133UF1)",
//     "Spectra 6 7.3 800 x 480 (E673)"
// };

// std::string valid_colors[] = {
//     "", "black", "red", "yellow", "", "7colour", "spectra6"};

//EPD EEPROM structure.
typedef struct {

    //< Little endian 
    // H uint16_t width
    // H uint16_t height
    // B uint8_t color
    // B uint8_t pcb_variant
    // B uint8_t display_variant
    // 22 char string str(datetime.datetime.now()).encode("ASCII"))
    // public:
        uint16_t Width;
        uint16_t Height;
        uint8_t Colour;
        uint8_t PCB_Var;
        uint8_t Display_Var;
        std::string Eeprom_Write_Time;

}EPD_t;

void epd_from_bytes(
    uint8_t * buf,
    EPD_t * epd_result
);

// read EDP from eeprom ( given GPIO pins )
esp_err_t read_eeprom( 
    gpio_num_t scl, 
    gpio_num_t sda, 
    EPD_t * epd_result
);

// read EDP from eeprom ( given i2c bus )
esp_err_t read_eeprom(
    i2c_master_bus_handle_t i2c_bus_handle,
    EPD_t * epd_result
);

void print_eeprom(EPD_t * epd_result);

#endif