idf_component_register(SRC_DIRS "."
                    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi 
                    REQUIRES adafruit_gfx
                    INCLUDE_DIRS "include")
