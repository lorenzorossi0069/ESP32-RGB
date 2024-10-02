/* spiffs_lib.c
*/
#include <stdio.h>
#include <string.h>
//#include <sys/unistd.h>
//#include <sys/stat.h>
//#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

static const char *TAG = "spiffs_lib";
static esp_err_t ret;

esp_vfs_spiffs_conf_t conf = {
	  .base_path = "/spiffs",
	  .partition_label = NULL,
	  .max_files = 3,
	  .format_if_mount_failed = true //orig true, but here must only be read
	};


void init_spiffs(void) {
	ESP_LOGI(TAG, "Initializing SPIFFS");
	char message[50];

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			sprintf (message,"%s","Failed to mount or format filesystem");
			ESP_LOGI(TAG, "%s",message);
			
		} else if (ret == ESP_ERR_NOT_FOUND) {
			sprintf (message,"%s","Failed to find SPIFFS partition");
			ESP_LOGI(TAG, "%s",message);
		} else {
			sprintf (message,"%s %s","Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
			ESP_LOGI(TAG, "%s",message);
		}
	}
}


//------------

void print_spiffs_info(void) {
	//just print information
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}


