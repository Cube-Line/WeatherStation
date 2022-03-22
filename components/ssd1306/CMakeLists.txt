# idf_component_register(SRCS "ssd1306.c"
#                   INCLUDE_DIRS "."
#                   REQUIRES )
set(COMPONENT_SRCS  ssd1306.c ssd1306_i2c.c ssd1306_spi.c)
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component()