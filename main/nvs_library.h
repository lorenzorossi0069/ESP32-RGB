/* nvs_library.h
*/
#ifndef _NVS_LIBRARY_H_
#define _NVS_LIBRARY_H_

#include "nvs_flash.h" //per avere nvs_handle

//project-specific headers
#include "rgb.h"
#include "timeCounters_library.h"

//public functions
void initialize_NVS(void);

//extern nvs_handle my_handle;
//void openNvsNamespace(char* namespace, nvs_handle* my_handle); 
//void loadTimeDataFromNVS(nvs_handle my_handle, timeData_t* p_timeData);
//void loadrgbDataFromNVS(nvs_handle my_handle, rgbData_t* p_rgbData);
//void saveTimeDataToNVS(nvs_handle my_handle, timeData_t timeData);
//void savergbDataToNVS(nvs_handle my_handle, rgbData_t rgbData);
//void nvs_close(nvs_handle);	

void loadDataFromNVS(timeData_t* , rgbData_t* );
void saveDataToNVS  (timeData_t* , rgbData_t* );

#endif