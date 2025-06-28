/*
    Driver for the pimoroni "Inky Imprssion" that uses the UC8159
    ? is the UC8159 the same as AC057TC1 ? 
*/

#include "inky_uc8159.h"
#include <stdio.h>
#include <cstring>

#include "esp_log.h"
#define _NUM_SUPPORT_RESOLUTIONS 2

#define _RESOLUTION_SETTING_5_7_INCH 0b11
#define _RESOLUTION_SETTING_4_INCH 0b10

#define TAG "Inky_UC8159"
resolution_t _SUPPORT_RESOLUTIONS[_NUM_SUPPORT_RESOLUTIONS] =  {_RESOLUTION_5_7_INCH, _RESOLUTION_4_INCH};

Inky_UC8159::~Inky_UC8159(void){
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
Inky_UC8159::Inky_UC8159(
    resolution_t resolution, 
    gpio_num_t cs_pin,
    gpio_num_t dc_pin,
    gpio_num_t reset_pin, 
    gpio_num_t busy_pin,
    spi_host_device_t spi_bus
)
: Adafruit_GFX( resolution.width, resolution.height )
{
    DISPLAY_VAR = Display_Var_t::_7_Colour_UC8159;
    //store attributes
    _resolution = resolution;
    _resolution_setting = (_resolution.width == 600) ? _RESOLUTION_SETTING_5_7_INCH : _RESOLUTION_SETTING_4_INCH;
    // self.cols, self.rows, // how are cols and rows different from width and height?
    // self.offset_x, self.offset_y, //  offsets not used in uc8159 ( they are used in ssd1608 and ssd1683)
    
    // if colour not in ("multi"):
    //      raise ValueError(f"Colour {colour} is not supported!")
    // self.colour = colour // only used in ssd1608, ssd1683 and inky)
    // self.lut = colour // only used in ssd1608, ssd1683 and inky)
    
    _buf_len = _resolution.height * _resolution.width / 2;
    _buf = (uint8_t**)malloc( _buf_len );

    memset(_buf, Inky_UC8159::WHITE<< 4 | Inky_UC8159::WHITE, _buf_len );
    _spi_bus = spi_bus;
    
    _cs_pin = cs_pin;
    _dc_pin = dc_pin;
    _reset_pin = reset_pin;
    _busy_pin = busy_pin;
    
    ESP_LOGI("Inky_UC8159","created");
}

void Inky_UC8159::_reset(){
    gpio_set_level(_reset_pin, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(_reset_pin, 1);
}

esp_err_t Inky_UC8159::setup(){
    if(!_gpio_setup){
        //check that the resolution is supported
        bool supported = false;
        for( int i = 0 ; i < _NUM_SUPPORT_RESOLUTIONS ; i++)
        {
            if ( 
                _resolution.width == _SUPPORT_RESOLUTIONS[i].width &&
                _resolution.height == _SUPPORT_RESOLUTIONS[i].height 
            ){
                supported = true;
                break;
            }
        }
        
        if( ! supported )
        {
            return ESP_ERR_INVALID_ARG;
        }
    
        _init_hw();
    }
    //reset
    _reset();

    _busy_wait(1000);

    // # Resolution Setting
    // # 10bit horizontal followed by a 10bit vertical resolution
    // # big-endian
    _send_command(
        UC8159_TRES,
        (uint8_t[]){
            (uint8_t)(_resolution.width>>8),
            (uint8_t)(_resolution.width),
            (uint8_t)(_resolution.height>>8),
            (uint8_t)(_resolution.height)
        },
        4
    );

    // # Panel Setting
    // # 0b11000000 = Resolution select, 0b00 = 640x480, our panel is 0b11 = 600x448
    // # 0b00100000 = LUT selection, 0 = ext flash, 1 = registers, we use ext flash
    // # 0b00010000 = Ignore
    // # 0b00001000 = Gate scan direction, 0 = down, 1 = up (default)
    // # 0b00000100 = Source shift direction, 0 = left, 1 = right (default)
    // # 0b00000010 = DC-DC converter, 0 = off, 1 = on
    // # 0b00000001 = Soft reset, 0 = Reset, 1 = Normal (Default)
    // # 0b11 = 600x448
    // # 0b10 = 640x400
    _send_command(
        UC8159_PSR,
        (uint8_t[]){
            // See above for more magic numbers
            // display_colours == UC8159_7C
            (uint8_t)((_resolution_setting<<6) |  0b101111),
            0x08
        },
        2
    );

    // Power Settings
    _send_command(
        UC8159_PWR,
        (uint8_t[]){
            (0x06 << 3) |  // ??? - not documented in UC8159 datasheet  # noqa: W504
            (0x01 << 2) |  // SOURCE_INTERNAL_DC_DC                     # noqa: W504
            (0x01 << 1) |  // GATE_INTERNAL_DC_DC                       # noqa: W504
            (0x01),        // LV_SOURCE_INTERNAL_DC_DC
            0x00,          // VGx_20V
            0x23,          // UC8159_7C
            0x23           // UC8159_7C
        },
        4
    );

    // Set the PLL clock frequency to 50Hz
    // 0b11000000 = Ignore
    // 0b00111000 = M
    // 0b00000111 = N
    // PLL = 2MHz * (M / N)
    // PLL = 2MHz * (7 / 4)
    // PLL = 2,800,000 ???
    _send_command(
        UC8159_PLL, 
        (uint8_t[]){ 0x3C }, // 0b0011 1100
        1
    );
    
    // Send the TSE register to the display
    _send_command(
        UC8159_TSE, 
        (uint8_t[]){ 0x00 }, 
        1
    );

    // VCOM and Data Interval setting
    // 0b11100000 = Vborder control (0b001 = LUTB voltage)
    // 0b00010000 = Data polarity
    // 0b00001111 = Vcom and data interval (0b0111 = 10, default)
    _send_command(
        UC8159_CDI, 
        (uint8_t[]){
            (uint8_t)( (_border_colour << 5) | 0x17 )
        }, //0b00110111
        1
    );

    // Gate/Source non-overlap period
    // 0b11110000 = Source to Gate (0b0010 = 12nS, default)
    // 0b00001111 = Gate to Source
    _send_command(UC8159_TCON, (uint8_t[]){ 0x22 }, 1 );  //0b00100010

    // Disable external flash
    _send_command(UC8159_DAM, (uint8_t[]){ 0x00 }, 1 );

    // UC8159_7C
    _send_command(UC8159_PWS, (uint8_t[]){ 0xAA }, 1 );

    // Power off sequence
    // 0b00110000 = power off sequence of VDH and VDL, 0b00 = 1 frame (default)
    // All other bits ignored?
    _send_command(
            UC8159_PFS, (uint8_t[]){0x00}, 1  // PFS_1_FRAME
    );


    ESP_LOGI("setup","complete");

    return ESP_OK;
}

/*
Update display and Show buffer on display.
Dispatches display update to correct driver.
*/
void Inky_UC8159::show()
{
    ESP_LOGI("show", "send buf");

    _send_command(UC8159_DTM1, (uint8_t *)_buf, _buf_len );
    
    _send_command(UC8159_PON);
    _busy_wait(200);
    
    ESP_LOGI("_update", "refresh");
    _send_command(UC8159_DRF);
    _busy_wait(32000);
    
    _send_command(UC8159_POF);
    _busy_wait(200);
}



/*
Set a single pixel.
@param x: x position on display
@param y: y position on display
@param colour: colour to set
*/
void Inky_UC8159::drawPixel(int16_t x, int16_t y, uint16_t colour)
// esp_err_t Inky_UC8159::set_pixel(uint16_t x, uint16_t y, uint16_t colour)
{

    
    if( colour >= Inky_UC8159::MAX || 
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

void Inky_UC8159::setBorder(uint16_t colour){
    _border_colour = colour;
        _send_command(
        UC8159_CDI, 
        (uint8_t[]){
            (uint8_t)( (_border_colour << 5) | 0x17 )
        }, //0b00110111
        1
    );

}
