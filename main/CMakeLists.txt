idf_component_register(SRCS "led_RGB_esp32_Main.c"
			    "WebServer_library.c"
			    "WiFi_library.c"
			    "nvs_library.c"
			    "timeCounters_library.c"
			    "rgb.c"
			    "spiffs_lib.c"
                    INCLUDE_DIRS "."
			)
spiffs_create_partition_image(spiffs ../spiffs_source FLASH_IN_PROJECT)

