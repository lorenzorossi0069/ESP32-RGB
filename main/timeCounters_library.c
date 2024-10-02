/*  timeCounters_library.h
*/

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/event_groups.h" 
//#include <freertos/task.h> //in case of TickType_t xTaskGetTickCount()
//#include "sys/time.h"

#include "time.h"
#include "esp_system.h" 
#include "esp_event.h" //deprec "esp_event_loop.h"
#include "esp_log.h"
#include "driver/gpio.h" 
//#include "GPIO_library.h"
#include "timeCounters_library.h" //to get typedef
#include "nvs_library.h"

#define REFRESH_INTERVAL_MS	1000

static const char *TAG = "timeCounters_library";

//typedef timeData_t is shared in header file
timeData_t timeData={0,0}; //will be updated with NVS data

uint32_t getTimeOn(void){
	return timeData.timeOn;
}
void increaseTimeOn(int nSec) {
	timeData.timeOn += nSec;
}

uint32_t getRebootsCount(void) {
	return timeData.rebootsCount;
}
void increaseRebootsCount(void) {
	timeData.rebootsCount+=1;
	ESP_LOGI(TAG, "num reboots = %d",getRebootsCount());
}


void convertSec2stringHHSS(uint32_t numSec, char *timeString){
	#define SECONDS 60
	#define MINUTES 60
	
	uint32_t upperUnitTime=numSec; 
	
	//get seconds
	uint32_t secs= upperUnitTime % SECONDS;	
	//get division result
	upperUnitTime=upperUnitTime/SECONDS;
	
	//get minutes
	uint32_t minutes=upperUnitTime % MINUTES;	
	//get division result
	upperUnitTime=upperUnitTime/MINUTES;	
	
	//get hours
	uint32_t hours=upperUnitTime;	
		
	//expand (division result) hereafter for days etc...
	
	sprintf(timeString,"%02dh: %02dm: %02ds",hours,minutes,secs);
}





