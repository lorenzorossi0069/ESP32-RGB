/*  Led_RGB_esp32_Main.c
*/
#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h" 
#include "esp_system.h" 
#include "esp_wifi.h" 
#include "esp_event.h" //deprec "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h" 
#include "freertos/semphr.h" //if using Semaphores
#include "mdns.h" 
#include "lwip/api.h" 
#include "lwip/err.h"
#include "lwip/netdb.h"
//#include "esp_event_loop.h"
//#include "esp_vfs.h"
//#include "esp_vfs_fat.h"
//#include "spiffs_vfs.h" 
#include "esp_netif.h"  //replace tcpip_adapter.h

//my libraries
#include "WebServer_library.h"
//no//#include "GPIO_library.h"
#include "WiFi_library.h"
//no//#include "timeCounters_library.h"
#include "nvs_library.h"
#include "WiFi_library.h"
#include "rgb.h"
#include "esp_spiffs.h"
#include "spiffs_lib.h"

static const char *TAG = "ledRGBesp32_Main";
static const char *TAG_0 = "ledRGBesp32_Main(Tag_0)"; //major priority
static char* wifi_mode_names[3]={"AP","STA","No WiFi"};	

void setConsoleLoggingLevel(void);

// Main application
void app_main(){
//==========================================================
//==========================================================
	#define WIFI_AP		0
	#define WIFI_STA	1	
	#define WIFI_NO		2 
	
	/*set WiFi mode mode in menuconfig with custom kconfig.projbuild  */
	#if CONFIG_SET_MODE_AP
		const int wifi_mode = WIFI_AP;		
	#elif CONFIG_SET_MODE_STA
		const int wifi_mode = WIFI_STA;
	#else
		const int wifi_mode = WIFI_NO;
	#endif 
	
//===========================================================
//===========================================================	

	setConsoleLoggingLevel(); //hide selected console printout	
	ESP_LOGI(TAG_0,"===========================================================");
	ESP_LOGI(TAG_0,"build at %s %s",__DATE__, __TIME__);
	ESP_LOGI(TAG_0, "wifi_mode is %s; (set using menuconfig)",wifi_mode_names[wifi_mode]);
	ESP_LOGI(TAG_0,"===========================================================");
			
	initialize_NVS();
	
	init_spiffs();
	
	print_spiffs_info();
					
    if (wifi_mode == WIFI_STA){	
		wifi_init_sta();		
	}
	else if (wifi_mode == WIFI_AP){
		wifi_init_softap();	
	}
	else {
		ESP_LOGI(TAG, "NO WiFi enabled"); 
	}
	
	//no//gpio_setup();
	rgb_ledc_init();
	
    //no//xTaskCreate(timeMeasure_task, "timeMeasure_task", 2048, NULL, 4, NULL);
		
	//read gpio inputs at startup (even without interrupt) by giving debounce semaphore for a run
	//(it is like an ISR happened)
	//no//xSemaphoreGiveFromISR(xSem_debounce, NULL); 

	//Web server is started by WiFi_event_handler (after WiFi start (AP or STA):
	if ((wifi_mode == WIFI_STA)||(wifi_mode == WIFI_AP)) {		
	
		//mDNS does not works when ESP32 is AP and browser is android or Windows without Bonjour service (or linux without Avahi)
		initialize_mDNS("webesp32");
	
		xTaskCreate(&http_server_task, "http_server_task", 4096, NULL, 5, NULL); // TBD posso ridurre stack ??
		
	    xTaskCreate(&rgb_task, "rgb_task", 2048, NULL, 5, NULL); 
	
	}
}

void setConsoleLoggingLevel(void){
	//(set LOG_DEFAULT_LEVEL in menuconfig among following items)
	//--------------------------------------------------------------
	//ESP_LOG_NONE 
	//ESP_LOG_ERROR
	//ESP_LOG_WARN
	//(*)ESP_LOG_INFO <= orig in menuconfig (CONFIG_LOG_DEFAULT_LEVEL_INFO)
	//debug
	//verbose

	//NOTE: following instructions can ONLY DOWNGRADE (eg to NONE) the menuconfig settings (*) above
	
	////uncomment following lines to disable logging per translation unit :
	//---------- App level ----------------
	esp_log_level_set("ControlMain", ESP_LOG_NONE);
	//esp_log_level_set("WiFi_library", ESP_LOG_NONE);	
	esp_log_level_set("WiFi_library (Tag2)", ESP_LOG_NONE);		
	//esp_log_level_set("WebServer_library", ESP_LOG_NONE);	
	//esp_log_level_set("WebServer_library(Tag_2)", ESP_LOG_NONE);
	//esp_log_level_set("nvs_library", ESP_LOG_NONE);
	esp_log_level_set("mDNS_library", ESP_LOG_NONE);
	esp_log_level_set("timeCounters_library", ESP_LOG_NONE);
	//esp_log_level_set("rgb", ESP_LOG_NONE);
	esp_log_level_set("rgb (Tag2)", ESP_LOG_NONE);
	//esp_log_level_set("spiffs_lib", ESP_LOG_NONE);
	//---------- ESP-IDF level ----------------
	//esp_log_level_set("gpio", ESP_LOG_NONE);
	esp_log_level_set("wifi", ESP_LOG_NONE);
	////-------- for RELEASE it is better set ALL ("*") to NONE
	//esp_log_level_set("*", ESP_LOG_NONE);  //disable all
}



