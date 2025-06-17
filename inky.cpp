#include <stdio.h>
#include "inky.h"
#include "esp_log.h"
#include "inky_uc8159.h"
#define TAG "Inky"
Inky * Auto(        
    i2c_master_bus_handle_t i2c_bus,
    spi_host_device_t   spi_bus,
    gpio_num_t cs_pin,
    gpio_num_t dc_pin,
    gpio_num_t reset_pin, 
    gpio_num_t busy_pin
){
    EPD_t eeprom_data;
    esp_err_t status = read_eeprom(i2c_bus, &eeprom_data);
    if(status != ESP_OK){
        ESP_LOGE(TAG,"Auto(): Failed to read eeprom.");
        return NULL;
    }

    if(eeprom_data.Display_Var>=NUM_DISPLAY_VAR){
        ESP_LOGE(
            TAG,
            "Auto(): Unknown and unsupported display variant: %d",
            eeprom_data.Display_Var
        );
    }
    else
    {
        ESP_LOGI(TAG, "Auto(): Display: \"%s\"",
            DisplayVars[eeprom_data.Display_Var]
        );
    }
    switch (eeprom_data.Display_Var)
    {
    case Display_Var_t::_7_Colour_UC8159:
        return new Inky_UC8159(
            _RESOLUTION_5_7_INCH,
            cs_pin, dc_pin,
            reset_pin,busy_pin,
            spi_bus
        );
    default:
        ESP_LOGE(
            TAG,
            "Auto(): Unsupported Display Variant: %d: \"%s\"",
            eeprom_data.Display_Var,
            DisplayVars[eeprom_data.Display_Var]
        );
        return NULL;
    }
    return NULL;
}
