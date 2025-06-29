#include <stdio.h>
#include "inky.h"
#include "esp_log.h"
#include "inky_uc8159.h"
#include "inky_e673.h"
#define TAG "Inky"

#define _SPI_MAX_SZ 64

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

    case Display_Var_t::Spectra_6_7_3_800x480_E673:
        return new Inky_E673(
            _RESOLUTION_7_3_INCH,
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


Inky::~Inky(){
    if(_spi_dev){
        spi_bus_remove_device(_spi_dev);
    }
}

// Set up Inky GPIO & spi
esp_err_t Inky::_init_hw(){
    esp_err_t status;
    
    //add EPD to spi bus
    if(!_spi_dev){
        
        spi_device_interface_config_t devcfg;
        memset(&devcfg, 0, sizeof(devcfg));
        devcfg.clock_speed_hz = 2000000;
        devcfg.mode = 0;
        // devcfg.spics_io_num = _cs_pin;
        devcfg.spics_io_num     = -1,
        devcfg.queue_size = 1;
        devcfg.duty_cycle_pos   = 128,
        
        status = spi_bus_add_device(_spi_bus, &devcfg, &_spi_dev);
        if( status != ESP_OK )
        return status;
    }
    
    //set up gpio pins
    gpio_reset_pin(_cs_pin);
    gpio_set_direction(_cs_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_cs_pin, 1);
    
    gpio_reset_pin(_dc_pin);
    gpio_set_direction(_dc_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_dc_pin, 0);
    
    gpio_reset_pin(_reset_pin);
    gpio_set_direction(_reset_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(_reset_pin, 1);
    
    gpio_reset_pin(_busy_pin);
    gpio_set_direction(_busy_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(_busy_pin, GPIO_FLOATING);
    
    _gpio_setup = true;
    return ESP_OK;
}


void Inky::_spi_transfer(uint8_t *data, uint32_t data_len) {
    esp_err_t ret;
    spi_transaction_t t;
    uint32_t chunk_size, chunck_count = 0;
    uint32_t sent_bytes = 0;
    while(data_len){
        memset(&t, 0, sizeof(t));       //Zero out the transaction
        if(data_len>_SPI_MAX_SZ){
            chunk_size = _SPI_MAX_SZ;
        }else{
            chunk_size = data_len ; 
        }
        t.length = 8 * chunk_size;        // transaction length is in bits
        t.tx_buffer = &data[sent_bytes];

        sent_bytes += chunk_size;
        data_len -= chunk_size;

        t.rxlength=0;
        ret = spi_device_polling_transmit(_spi_dev, &t);  //Transmit!
        // ret = spi_device_transmit(_spi_dev, &t);  //Transmit!
        if(ret!=ESP_OK){
            ESP_LOGE("_spi_transfer","device transmit: %d. chunck_count:%lu", ret, chunck_count);
        }
        assert(ret==ESP_OK);            //Should have had no issues.
        chunck_count++;
    }

}

/*  
@brief Send command over SPI.
@param command command byte
@param data array of values
@param data_len length of data array
*/
void Inky::_send_command
    (uint8_t command, uint8_t *data, uint32_t data_len)
{
    gpio_set_level(_cs_pin, 0);
    gpio_set_level(_dc_pin, 0);
    _spi_transfer(&command, 1);
    gpio_set_level(_dc_pin, 1);
    gpio_set_level(_cs_pin, 1);
    
    if(data_len){
        _send_data(data, data_len);
    }
}

//*/
void Inky::_send_data
    (uint8_t *data, uint32_t data_len)
{
        gpio_set_level(_cs_pin, 0);
        _spi_transfer(data,data_len);
        gpio_set_level(_cs_pin, 1);
}

/*  
@brief Send command over SPI.
@param command command byte
*/
void Inky::_send_command(uint8_t command){
    gpio_set_level(_cs_pin, 0);
    gpio_set_level(_dc_pin, 0);
    _spi_transfer(&command, 1);
    gpio_set_level(_dc_pin, 1);
    gpio_set_level(_cs_pin, 1);
}

/*
Wait for busy/wait pin.
If the busy_pin is *high* (pulled up by host)
then assume we're not getting a signal from inky
and wait the timeout period to be safe.
*/
esp_err_t Inky::_busy_wait(uint32_t timeout)
{
    if( gpio_get_level(_busy_pin) == 1 ){
        ESP_LOGW(TAG"::_busy_wait","Held high. Waiting for %ldms",timeout);
        vTaskDelay(timeout / portTICK_PERIOD_MS);
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG"::_busy_wait","Waiting...");
    
    // TODO experiement with gpio ETM
    for(uint32_t i = 0; i < timeout / 10 ; i++){
        vTaskDelay(10 / portTICK_PERIOD_MS);
         if( gpio_get_level(_busy_pin) == 1 )
            return ESP_OK;
    }

    ESP_LOGW(TAG"::_busy_wait","Timed out after %ldms",timeout);
    return ESP_ERR_TIMEOUT;
}



/**************************************************************************/
/*!
   @brief   Draw a "8-bit" image at the specified (x,y) position. 
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 8-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Inky::drawRGBBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                                 int16_t w, int16_t h) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, (uint16_t)(&bitmap[j * w + i]));
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a "8-bit" image at the specified (x,y) position. 
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 8-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Inky::drawRGBBitmap(int16_t x, int16_t y, uint8_t *bitmap,
                                 int16_t w, int16_t h) {
  startWrite();
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, (uint8_t)bitmap[j * w + i]);
    }
  }
  endWrite();
}

