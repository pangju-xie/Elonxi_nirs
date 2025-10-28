get_filename_component(PROJECT_ROOT ${CMAKE_SOURCE_DIR} ABSOLUTE)

set(src_dirs
            LED
            ADC
            MPU9250
            CMT2300
            filters
            UART
            )

set(include_dirs
            LED
            ADC
            MPU9250
            CMT2300
            filters
            UART
            )

set(requires
            driver
            espressif__mdns
            esp_adc
            esp_wifi
            nvs_flash
            )

set(priv_include_dirs
                    ../../main/APP
)

idf_component_register(SRC_DIRS ${src_dirs} 
                        INCLUDE_DIRS ${include_dirs} 
                        REQUIRES ${requires} 
                        PRIV_INCLUDE_DIRS ${priv_include_dirs}
                        )

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)