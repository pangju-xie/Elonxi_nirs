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
            esp_adc)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)