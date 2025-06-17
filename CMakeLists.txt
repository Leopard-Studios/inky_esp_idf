idf_component_register(SRCS "eeprom.cpp" "inky.cpp" "inky_uc8159.cpp"
                    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi adafruit_gfx
                    INCLUDE_DIRS "include")
