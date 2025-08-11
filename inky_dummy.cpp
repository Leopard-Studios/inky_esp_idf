#include "inky_dummy.h"
#include "esp_log.h"
#define TAG "Inky_DUMMY"
Inky_DUMMY::~Inky_DUMMY(){
    if(_buf){
        free(_buf);
    }

}
Inky_DUMMY::Inky_DUMMY(
    resolution_t resolution, 
    gpio_num_t cs_pin,
    gpio_num_t dc_pin,
    gpio_num_t reset_pin, 
    gpio_num_t busy_pin,
    spi_host_device_t   spi_bus
) : Adafruit_GFX( resolution.width, resolution.height ){

    //init buffer
    //2 pixels per byte
    _buf_len = _resolution.height * _resolution.width / 2;
    _buf = (uint8_t**)malloc( _buf_len );
    memset(_buf, Inky_DUMMY::WHITE<< 4 | Inky_DUMMY::WHITE, _buf_len );
    
    
    
}
 
esp_err_t Inky_DUMMY::setup(){
    ESP_LOGI(TAG, "Inky_DUMMY setup");
    return ESP_OK;
}

void Inky_DUMMY::show(){
    ESP_LOGI(TAG, "Inky_DUMMY show");
    // TODO print buffer to console
}


void Inky_DUMMY::drawPixel(int16_t x, int16_t y, uint16_t colour){
    if( colour >= Inky_DUMMY::MAX || 
        x >= _width || y >= _height ||
        x < 0 || y < 0
    )
    {
         ESP_LOGE(TAG, "drawPixel: invalid arg. X:%d Width:%d, Y:%d Height:%d: Colour:%u ",
                x, _width, y, _height, colour );
        return;
    }
}