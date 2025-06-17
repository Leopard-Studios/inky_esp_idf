#include "inky_uc8159.h"
#include <stdio.h>
#include <cstring>

#include "esp_log.h"
#define _NUM_SUPPORT_RESOLUTIONS 2

#define _RESOLUTION_SETTING_5_7_INCH 0b11
#define _RESOLUTION_SETTING_4_INCH 0b10

#define _SPI_MAX_SZ 64
#define TAG "Inky_UC8159"
resolution_t _SUPPORT_RESOLUTIONS[_NUM_SUPPORT_RESOLUTIONS] =  {_RESOLUTION_5_7_INCH, _RESOLUTION_4_INCH};

Inky_UC8159::~Inky_UC8159(void){
    if(_buf){
        free(_buf);
    }
};

/*
Initialise an Inky Display structure.
@param resolution: (width, height) in pixels, default: (600, 448)
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

    memset(_buf, Colour_7_t::WHITE << 4 | Colour_7_t::WHITE, _buf_len );
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

// Set up Inky GPIO & spi
esp_err_t Inky_UC8159::_init_hw(){
    esp_err_t status;
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

esp_err_t Inky_UC8159::setup(){
    if(!_gpio_setup){
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
            (uint8_t)(( _border_colour << 5) | 0x17 )
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
Wait for busy/wait pin.
If the busy_pin is *high* (pulled up by host)
then assume we're not getting a signal from inky
and wait the timeout period to be safe.
*/
esp_err_t Inky_UC8159::_busy_wait(uint32_t timeout)
{
    if( gpio_get_level(_busy_pin) == 1 ){
        ESP_LOGW("Inky_UC8159::_busy_wait","Held high. Waiting for %ldms",timeout);
        vTaskDelay(timeout / portTICK_PERIOD_MS);
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI("Inky_UC8159::_busy_wait","Waiting...");
    
    // TODO experiement with gpio ETM
    for(uint32_t i = 0; i < timeout / 10 ; i++){
        vTaskDelay(10 / portTICK_PERIOD_MS);
         if( gpio_get_level(_busy_pin) == 1 )
            return ESP_OK;
    }

    ESP_LOGW("Inky_UC8159::_busy_wait","Timed out after %ldms",timeout);
    return ESP_ERR_TIMEOUT;
}

void Inky_UC8159::_spi_transfer(uint8_t *data, uint32_t data_len) {
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
void Inky_UC8159::_send_command
    (uint8_t command, uint8_t *data, uint32_t data_len)
{
    gpio_set_level(_cs_pin, 0);
    gpio_set_level(_dc_pin, 0);
    _spi_transfer(&command, 1);
    gpio_set_level(_dc_pin, 1);
    gpio_set_level(_cs_pin, 1);
    
    if(data_len){
        gpio_set_level(_cs_pin, 0);
        _spi_transfer(data,data_len);
        gpio_set_level(_cs_pin, 1);
    }
}

/*  
@brief Send command over SPI.
@param command command byte
*/
void Inky_UC8159::_send_command(uint8_t command){
    gpio_set_level(_cs_pin, 0);
    gpio_set_level(_dc_pin, 0);
    _spi_transfer(&command, 1);
    gpio_set_level(_dc_pin, 1);
    gpio_set_level(_cs_pin, 1);
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
    if( colour >= Colour_7_t::MAX || 
        x >= _resolution.width || x >= _resolution.height ||
        x < 0 || y < 0
    )
    {
        ESP_LOGE(TAG, "set_pixel: invalid arg");
        return;
    }
    
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

void Inky_UC8159::setBorder(Colour_7_t colour){
    _border_colour = colour;
}
