/* nvs_library.c
*/
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
*/
#include "esp_partition.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

//project-specific headers
#include "rgb.h"
#include "timeCounters_library.h"

//static variables
static const char *TAG = "nvs_library";
static const char *rgb_NS = "rgbNamespace";

//Initialize NVS
void initialize_NVS(void) {
    esp_err_t ret = nvs_flash_init(); //init default NVS partition, otherwise nvs_flash_init_partition() 
	
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_LOGI(TAG, "NVS partition was truncated and needs to be erased"); //info

		ESP_ERROR_CHECK(nvs_flash_erase()); //if also erase fail, maybe something is wrong with flash...
		ret = nvs_flash_init(); //init again, after erase
	}
    ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "info: nvs_flash_init() ret= %d",ret); //info
}

// Open
/*void openNvsNamespace(char* namespace, nvs_handle* p_my_handle) {
	esp_err_t err;
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");

    err = nvs_open(namespace, NVS_READWRITE, p_my_handle);
	
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
}
*/

//vvvvvvvvvvvvvvvvvvv
//tip for abstraction:
//nvs_iterator_t nvs_entry_find(const char *part_name, const char *namespace_name, nvs_type_ttype)
//^^^^^^^^^^^^^^^^^^^


void loadDataFromNVS(timeData_t* p_timeData, rgbData_t* p_rgbData) {	
	nvs_handle my_handle;	
	esp_err_t err = ESP_OK;	
	
	err = nvs_open(rgb_NS, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle to read!\n", esp_err_to_name(err));
    }
	err|= nvs_get_u32(my_handle, "sec_TimeOn", &(p_timeData->timeOn));
	err|= nvs_get_u32(my_handle, "num_Boots",  &(p_timeData->rebootsCount));
	
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error getting timeData from NVS");
	}
	
	err|= nvs_get_u32(my_handle, "Rval", &(p_rgbData->R));
	err|= nvs_get_u32(my_handle, "Gval", &(p_rgbData->G));
	err|= nvs_get_u32(my_handle, "Bval", &(p_rgbData->B));
	//err= nvs_get_u32(my_handle, "Wval", &(p_rgbData->W)); //spare
	
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error getting rgbData data from NVS");
	}
	
	nvs_close(my_handle);
}

void saveDataToNVS(timeData_t* p_timeData, rgbData_t* p_rgbData) {
	nvs_handle my_handle;
	esp_err_t err = ESP_OK;	
	
	err = nvs_open(rgb_NS, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle to write!\n", esp_err_to_name(err));
		exit(1);
    }

	err= nvs_set_u32(my_handle, "sec_TimeOn",  getTimeOn());
	err= nvs_set_u32(my_handle, "num_Boots", getRebootsCount());
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error setting timeData to NVS");
	}
	
	err= nvs_set_u32(my_handle, "Rval",  getR_PWM());
	err= nvs_set_u32(my_handle, "Gval",  getG_PWM());
	err= nvs_set_u32(my_handle, "Bval",  getB_PWM());
	//err= nvs_set_u32(my_handle, "Wval",  getW_PWM()); //spare 
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error setting rgbData to NVS");
	}
	
	err|= nvs_commit(my_handle);	
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error during nvs_commit()");
	}
	//else {
	//	//ESP_LOGI(TAG,"data correctly saved to NVS"); // just for debug
	//}
	
	nvs_close(my_handle);
}

/*void savergbDataToNVS(nvs_handle my_handle, rgbData_t rgbData) {
	esp_err_t err = 0;	

	err= nvs_set_u8(my_handle, "Rval",  rgbData.R);
	err= nvs_set_u8(my_handle, "Gval",  rgbData.G);
	err= nvs_set_u8(my_handle, "Bval",  rgbData.B);
	//err= nvs_set_u8(my_handle, "Wval",  rgbData.W);
	
	err|= nvs_commit(my_handle);
	
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error in savergbDataToNVS()");
	}
	else {
		ESP_LOGI(TAG,"committed to NVS"); // just for debug 
	}
	nvs_close(nvs_handle);	
}*/


/* diagnostic tool functions:
 
//1) check NVS 
	const esp_partition_t* nvs_partition = 
		esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
	if(!nvs_partition) 
		ESP_LOGI(TAG, "FATAL ERROR: No NVS partition found\n");
	else {
		ESP_LOGI(TAG, "nvs_partition->flash_chip = %X",(int) nvs_partition->flash_chip); //eg 3FFB12B0
		ESP_LOGI(TAG, "nvs_partition->type = %d",(int) nvs_partition->type); //eg 1 (0=app, 1=data)
		ESP_LOGI(TAG, "nvs_partition->subtype = %d",(int) nvs_partition->subtype); //eg 2
		ESP_LOGI(TAG, "nvs_partition->address = %X",nvs_partition->address); //eg 0x9000
		ESP_LOGI(TAG, "nvs_partition->size = %X",nvs_partition->size); //eg 0x4000
		ESP_LOGI(TAG, "nvs_partition->label = %s",nvs_partition->label); //eg nvs
	}

//2) Example of nvs_get_stats() to get the number of used entries and free entries:
	nvs_stats_t nvs_stats;
	nvs_get_stats("nvs", &nvs_stats); //also NULL = "nvs"
	ESP_LOGI(TAG, "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
       nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
*/


