#include "inky_e673.h"
#include "esp_log.h"

#define TAG "Inky_E673"

Inky_E673::~Inky_E673(void){
    if(_buf){
        free(_buf);
    }
};

/*
Initialise an Inky Display structure.
@param resolution: (width, height) in pixels
@param cs_pin: chip-select pin for SPI communication
@param dc_pin: data/command pin for SPI communication
@param reset_pin: device reset pin
@param busy_pin: device busy/wait pin
@param spi_bus: the spi initilised bus  
*/
Inky_E673::Inky_E673(
    resolution_t resolution, 
    gpio_num_t cs_pin,
    gpio_num_t dc_pin,
    gpio_num_t reset_pin, 
    gpio_num_t busy_pin,
    spi_host_device_t spi_bus
)
: Adafruit_GFX( resolution.width, resolution.height ){  
    _resolution = resolution;    
    _spi_bus = spi_bus;
    
    _cs_pin = cs_pin;
    _dc_pin = dc_pin;
    _reset_pin = reset_pin;
    _busy_pin = busy_pin;

    //2 pixels per byte
    _buf_len = _resolution.height * _resolution.width / 2;
    _buf = (uint8_t**)malloc( _buf_len );


    memset(_buf, Inky_E673::WHITE<< 4 | Inky_E673::WHITE, _buf_len );
}


void Inky_E673::_magic_waveshare(){

    _send_command(
        EL673_CMDH, 
        (uint8_t[]){0x49, 0x55, 0x20, 0x08, 0x09, 0x18},
        6
    );
    _send_command(EL673_PWR, (uint8_t[]){0x3F}, 1 );
    _send_command(EL673_PSR, (uint8_t[]){0x5F, 0x69}, 2 );
    
    _send_command(EL673_POFS, (uint8_t[]){0x00, 0x54, 0x00, 0x44}, 4 );
    
    
    _send_command(EL673_BTST1, (uint8_t[]){ 0x40, 0x1F, 0x1F, 0x2C}, 4 );
    _send_command(EL673_BTST2, (uint8_t[]){ 0x6F, 0x1F, 0x17, 0x49}, 4 );
    _send_command(EL673_BTST3, (uint8_t[]){ 0x6F, 0x1F, 0x1F, 0x22}, 4 );
    
    _send_command(EL673_PLL, (uint8_t[]){0x03}, 1 );
    _send_command(EL673_CDI, (uint8_t[]){0x3F}, 1 );
    
    _send_command(EL673_TCON, (uint8_t[]){0x02, 0x00}, 2 );
    _send_command(EL673_TRES, (uint8_t[]){0x03, 0x20, 0x01, 0xE0}, 4 );
    _send_command(0x84, (uint8_t[]){0x01}, 1 );
    _send_command(EL673_PWS, (uint8_t[]){0x2F}, 1 );

}
void Inky_E673::_magic_pimoroni(){
    _send_command(
        EL673_CMDH, 
        (uint8_t[]){0x49, 0x55, 0x20, 0x08, 0x09, 0x18},
        6
    );
    _send_command(EL673_PWR, (uint8_t[]){0x3F}, 1 );
    _send_command(EL673_PSR, (uint8_t[]){0x5F, 0x69}, 2 );

    _send_command(EL673_BTST1, (uint8_t[]){ 0x40, 0x1F, 0x1F, 0x2C}, 4 );
    _send_command(EL673_BTST3, (uint8_t[]){ 0x6F, 0x1F, 0x1F, 0x22}, 4 );
    _send_command(EL673_BTST2, (uint8_t[]){ 0x6F, 0x1F, 0x17, 0x17}, 4 );

    _send_command(EL673_POFS, (uint8_t[]){0x00, 0x54, 0x00, 0x44}, 4 );
    _send_command(EL673_TCON, (uint8_t[]){0x02, 0x00}, 2 );
    _send_command(EL673_PLL, (uint8_t[]){0x08}, 1 );
    _send_command(EL673_CDI, (uint8_t[]){0x3F}, 1 );
    _send_command(EL673_TRES, (uint8_t[]){0x03, 0x20, 0x01, 0xE0}, 4 );
    _send_command(EL673_PWS, (uint8_t[]){0x2F}, 1 );
    _send_command(EL673_VDCS, (uint8_t[]){0x01}, 1 );
}

esp_err_t Inky_E673::setup(){
    if(!_gpio_setup){
        _init_hw();
    }

    _reset();

    _busy_wait(300);

    //magic commands...I can't find a datasheet, 
    //no idea where Pimoroni / waveshare pulled these from
    _magic_pimoroni();
    // _magic_waveshare();

    return ESP_ERR_NOT_FINISHED;
}

void Inky_E673::drawPixel(int16_t x, int16_t y, uint16_t colour){
    if( colour >=  Inky_E673::Colour::MAX || 
        x >= _width || y >= _height ||
        x < 0 || y < 0
    )
    {
        ESP_LOGE(TAG, "drawPixel: invalid arg. X:%d Width:%d, Y:%d Height:%d: Colour:%u ",
                x, _width, y, _height, colour );
        return;
    }
     
    //map x,y to buffer based on rotation
    //1=90 cw, 2=180, 3=270cw / 90 ccw
    int16_t new_x, new_y;
    switch (rotation)
    {
    case 1://90cw
        // x,y = w-y,x
        new_x = (_resolution.width-1)-y;
        new_y = x;
        break;
    case 2://180
        // x,y = w-x, h-y
        new_x = (_resolution.width-1)-x;
        new_y = (_resolution.height-1)-y;
        break;
    case 3://270cw /90ccw
        //x,y = y,h-x
        new_x = y;
        new_y = (_resolution.height-1)-x;
        break;
    default:
        new_x=x, new_y=y;
        break;
    }
    x=new_x;
    y=new_y;


    uint8_t (*ptrBuf)[_resolution.width/2] = 
        (uint8_t(*)[_resolution.width/2])_buf;
    if(x%2)
    {
        ptrBuf [y][x/2] &= 0xF0;
        ptrBuf [y][x/2] |= colour;
    }else{
        ptrBuf [y][x/2] &= 0x0F;
        ptrBuf [y][x/2] |= colour << 4;
    }
}

void Inky_E673::refresh(){
    _send_command(EL673_PON);
    _busy_wait(300);

    //second setting of the BTST2 register
    _send_command(EL673_BTST2, (uint8_t[]){0x6F, 0x1F, 0x17, 0x49}, 4 );
    
    ESP_LOGI(TAG, "show: refresh");
    _send_command(EL673_DRF, (uint8_t[]){0x00}, 1 );
    _busy_wait(32000);
    
    //power off
    _send_command(EL673_POF, (uint8_t[]){0x00}, 1 );
    _busy_wait(300);
}

void Inky_E673::show(){
    ESP_LOGI(TAG, "show: send buf");
    _send_command(EL673_DTM1, (uint8_t *)_buf, _buf_len );
    refresh();
}

void Inky_E673::_reset(){
    gpio_set_level(_reset_pin, 0);
    vTaskDelay(30 / portTICK_PERIOD_MS);
    gpio_set_level(_reset_pin, 1);
    vTaskDelay(30 / portTICK_PERIOD_MS);
}
