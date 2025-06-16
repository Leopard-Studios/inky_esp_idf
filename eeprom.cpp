
#include "eeprom.h"
#include <stdio.h>

/*
    @brief convert given bytes to a EPD
    @param buf array of byte, assume there are 29 bytes
    @param ret_struct pointer to the struct where the results should be placed
    @note data is little endian, ESP32 is little endian too. 
    Data format:
        // H uint16_t width
        // H uint16_t height
        // B uint8_t color
        // B uint8_t pcb_variant
        // B uint8_t display_variant
        // 22 char string str(datetime.datetime.now()).encode("ASCII"))
*/ 
void epd_from_bytes(uint8_t * buf, EPD_t * epd_result){
    epd_result->Width = ((uint16_t*)buf)[0];
    epd_result->Height = ((uint16_t*)buf)[1];
    epd_result->Colour = buf[4];
    epd_result->PCB_Var = buf[5];
    epd_result->Display_Var = buf[6];
    epd_result->Eeprom_Write_Time = std::string((const char *)&buf[7], (size_t)22);
}

esp_err_t read_eeprom( gpio_num_t scl, gpio_num_t sda, EPD_t * epd_result ){
    esp_err_t status;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = { 
            .enable_internal_pullup = true,
            .allow_pd = false
        }
    };
    status = i2c_new_master_bus(&i2c_bus_config, &bus_handle);

    if( status != ESP_OK)
        return status;

    status = read_eeprom( bus_handle, epd_result );
    
    //clean up
    i2c_del_master_bus(bus_handle);

    return status;
}

esp_err_t read_eeprom(
    i2c_master_bus_handle_t i2c_bus_handle, EPD_t * epd_result
){

    esp_err_t status;
    i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = EEPROM_ADDRESS,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = {.disable_ack_check = 0}
    };
    
    status = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle);

    if( status != ESP_OK )
        return status;

    //send 00 command and receive 29 bytes
    uint8_t tx_buf[] = {0,0}; 
    uint8_t rx_buf[29];

    //TODO timeout and check for success
    status = i2c_master_transmit_receive(dev_handle, tx_buf, 2, rx_buf, 29, 200 );

    if( status == ESP_OK )
        epd_from_bytes(rx_buf, epd_result);

    //clean up
    i2c_master_bus_rm_device(dev_handle);

    return status;

}

void print_eeprom(EPD_t * epd_result)
{
    printf("Resolution: %d x %d\n",epd_result->Width, epd_result->Height);
    printf("Colour: %d\n",epd_result->Colour);
    printf("PCB Var: %d\n",epd_result->PCB_Var);
    printf("Display Var: %d\n",epd_result->Display_Var);
}