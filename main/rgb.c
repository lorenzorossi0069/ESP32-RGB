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

#include "nvs_library.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define REFRESH_INTERVAL_MS	(1000)


#define R_GPIO     (27)  //maybe final will be (14)
#define G_GPIO     (26)  //maybe final will be (27)
#define B_GPIO     (14)  //maybe final will be (26)
//#define W_GPIO   (TBD) //spare 

#define RGB_TIMER 				LEDC_TIMER_0
#define LEDC_TEST_CH_NUM       (3) 				//(4) in case of 4-th channel W
#define LEDC_TEST_DUTY         (1000) 			//orig 4000
#define LEDC_FADE_TIME    	   (1000) 			//orig 3000

#define RGB_FREQ_HZ 			(5000) 						//orig 5000
#define RGB_DUTY_RESOLUTION 	LEDC_TIMER_13_BIT 			//orig 13 bit with 5000 Hz
#define RGB_MAX_DUTY 			((2)<<((RGB_DUTY_RESOLUTION)-1))

static const char *TAG = "rgb";
static const char *TAG_2 = "rgb (Tag2)";
static rgbData_t rgbData={0,0,0}; //will be updated with NVS data
static rgbData_t rgbDataOld={0,0,0}; //will be updated with NVS data
extern timeData_t timeData;

static ledc_timer_config_t ledc_timer_rgb;

static ledc_channel_config_t ledc_channel_R;
static ledc_channel_config_t ledc_channel_G;
static ledc_channel_config_t ledc_channel_B;

//Set rgbData methods from raw value for PWM (max RGB_MAX_DUTY)
void setR_PWM(uint32_t newVal_PWM) {
	rgbData.R = newVal_PWM;
}
void setG_PWM(uint32_t newVal_PWM) {
	rgbData.G = newVal_PWM;
}
void setB_PWM(uint32_t newVal_PWM) {
	rgbData.B = newVal_PWM;
}
//void setW_PWM(uint32_t newVal_PWM){
//	rgbData.W = newVal_PWM;
//}

//Set rgbData methods from normalized input (max 255=0xFF)
void setR_FF(uint32_t newVal_256) {
	//assert (newVal_256 <256); //check
	rgbData.R = newVal_256 *RGB_MAX_DUTY/256;
}
void setG_FF(uint32_t newVal_256) {
	//assert (newVal_256 <256); //check
	rgbData.G = newVal_256 *RGB_MAX_DUTY/256;
}
void setB_FF(uint32_t newVal_256) {
	//assert (newVal_256 <256); //check
	rgbData.B = newVal_256 *RGB_MAX_DUTY/256;
}
//void setW_FF(uint32_t newVal_256){
//	//assert (newVal_256 <256); //check
//	rgbData.W = newVal_256 *RGB_MAX_DUTY/256;
//}


//Get rgb methods (raw value for PWM)
uint32_t getR_PWM(void){
	return rgbData.R;
}
uint32_t getG_PWM(void){
	return rgbData.G;
}
uint32_t getB_PWM(void){
	return rgbData.B;
}
//uint32_t getW_PWM(void){
//	return rgbData.W;
//}

//Get rgb methods normalized to 255
uint32_t getR_FF(void){
	return getR_PWM() *256/RGB_MAX_DUTY;
}
uint32_t getG_FF(void){
	return getG_PWM() *256/RGB_MAX_DUTY;
}
uint32_t getB_FF(void){
	return getB_PWM() *256/RGB_MAX_DUTY;
}
//uint32_t getW_FF(void){
//	return getW_PWM() *256/RGB_MAX_DUTY;
//}

void rgb_ledc_init(void) {
	 //Configure timer (the same for all rgb(w) channels
    ledc_timer_rgb.duty_resolution = RGB_DUTY_RESOLUTION, // resolution of PWM duty
    ledc_timer_rgb.freq_hz = RGB_FREQ_HZ,                 // frequency of PWM signal
    ledc_timer_rgb.speed_mode = LEDC_HIGH_SPEED_MODE,     // timer mode
    ledc_timer_rgb.timer_num = RGB_TIMER,             	  // timer index
    ledc_timer_rgb.clk_cfg = LEDC_AUTO_CLK,               // Auto select the source clock

    ledc_timer_config(&ledc_timer_rgb);
	
	
    //Configure Red channel  
	ledc_channel_R.channel    = LEDC_CHANNEL_0;
    ledc_channel_R.duty       = 0;
    ledc_channel_R.gpio_num   = R_GPIO;
    ledc_channel_R.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_R.hpoint     = 0;
    ledc_channel_R.timer_sel  = RGB_TIMER;

    ledc_channel_config(&ledc_channel_R);
	
	//Configure Green channel
    ledc_channel_G.channel    = LEDC_CHANNEL_1;
    ledc_channel_G.duty       = 0;
    ledc_channel_G.gpio_num   = G_GPIO;
    ledc_channel_G.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_G.hpoint     = 0;
    ledc_channel_G.timer_sel  = RGB_TIMER;

    ledc_channel_config(&ledc_channel_G);
	
	//Configure Blue channel
    ledc_channel_B.channel    = LEDC_CHANNEL_2; 
    ledc_channel_B.duty       = 0;
    ledc_channel_B.gpio_num   = B_GPIO;
    ledc_channel_B.speed_mode = LEDC_HIGH_SPEED_MODE; 
    ledc_channel_B.hpoint     = 0;
    ledc_channel_B.timer_sel  = RGB_TIMER;

    ledc_channel_config(&ledc_channel_B);


    // Initialize fade service.
    ledc_fade_func_install(0);
}

void color_set(uint32_t R_val, uint32_t G_val, uint32_t B_val) {
	
	ESP_LOGI(TAG_2,"RGB_MAX_DUTY= %d",RGB_MAX_DUTY);
	ESP_LOGI(TAG,"new values [R G B] = [%d, %d, %d]",R_val, G_val, B_val);		
	
	//note: optionally ledc_set_fade_step_and_start() is thread safe
	
    ledc_set_fade_with_time(ledc_channel_R.speed_mode, ledc_channel_R.channel, R_val,    LEDC_FADE_TIME);
	ledc_fade_start        (ledc_channel_R.speed_mode, ledc_channel_R.channel, LEDC_FADE_NO_WAIT);
	
	ledc_set_fade_with_time(ledc_channel_G.speed_mode, ledc_channel_G.channel, G_val,    LEDC_FADE_TIME);
	ledc_fade_start        (ledc_channel_G.speed_mode, ledc_channel_G.channel, LEDC_FADE_NO_WAIT);
	
	ledc_set_fade_with_time(ledc_channel_B.speed_mode, ledc_channel_B.channel, B_val,    LEDC_FADE_TIME);			
    ledc_fade_start        (ledc_channel_B.speed_mode, ledc_channel_B.channel, LEDC_FADE_NO_WAIT);
	
	//TBD sostituire delay con interrupt a fine fade...
	vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS); //attesa fine fade
}

/* void loadDataFromNVS(nvs_handle my_handle, nvsData_t* p_nvsData) {
	esp_err_t err;	

	//err= nvs_get_u32(my_handle, "sec_TimeOn", &(p_timeData->timeOn));

	err= nvs_get_u32(my_handle, "sec_PowerOn", &(p_timeData->powerOn));
	
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error in loadDataFromNVS()");
	}
}*/

/*void saveDataToNVS(nvs_handle my_handle, timeData_t timeData) {
	esp_err_t err;	
	
	//err= nvs_set_u32(my_handle, "sec_TimeOn", timeData.timeOn);
	
	
	err|= nvs_commit(my_handle);
	if (err!=ESP_OK) {
		ESP_LOGI(TAG,"got an error in saveTimingDataToNVS()");
	}
	else {
		ESP_LOGI(TAG,"committed to NVS"); // just for debug 
	}
}
*/

/* //timeData
uint32_t getTimeOn(void){
	return rgbTimeData.TimeOn;
}

uint32_t getRebootsCount(void){
	return rgbTimeData.rebootsCount;
}
void increaseRebootsCount(void){
	rebootsCount+=1;
}
*/

void rgb_task(void * arg) {
	bool changedRGB = false;
	bool justRebooted = true;
		
	ESP_LOGI(TAG, "initializing rgb_task()");		
		
//1-st THING: load saved data
	loadDataFromNVS(&timeData, &rgbData);
	
//2-nd THING: increase Reboots Counter (ONLY HERE, i.e. before loop begin)	
	increaseRebootsCount(); 
	ESP_LOGI(TAG, "reboot Number %d",getRebootsCount());	
		
//3-rd THING: save new increased bootloadr counter (non ora, è fatto dopo) 
	//saveDataToNVS(&timeData, &rgbData);
		
//prepare timer	
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = REFRESH_INTERVAL_MS/portTICK_PERIOD_MS; //normalization to msec
		
    xLastWakeTime = xTaskGetTickCount (); //returns count of ticks since task was called.
	
	//2 debug info
	printf("\n INFO debug xFrequency = %d)\n",(int)xFrequency);
	printf("\n INFO debug REFRESH_INTERVAL_MS = %d)\n",(int)REFRESH_INTERVAL_MS);
	
    for( ;; ) {
        // Wait for the next cycle.
		
        vTaskDelayUntil( &xLastWakeTime, xFrequency);
		increaseTimeOn(REFRESH_INTERVAL_MS/1000);
		
		if ((rgbDataOld.R != rgbData.R)||(rgbDataOld.G != rgbData.G)||(rgbDataOld.B != rgbData.B))  {
			
			ESP_LOGI(TAG, "update RGB values from [%d %d %d] to [%d %d %d]",
				rgbDataOld.R, rgbDataOld.G, rgbDataOld.B, rgbData.R, rgbData.G, rgbData.B);
				
			//save on RAM new values
			rgbDataOld.R = rgbData.R;
			rgbDataOld.G = rgbData.G;
			rgbDataOld.B = rgbData.B;	
			
			ESP_LOGI(TAG, "changing color fade..........");
			color_set(rgbData.R, rgbData.G, rgbData.B);
			
			changedRGB = true;			
		}
		
		//salvo tempo funzionamento ogni ora (3600 sec), oppure se cambia RGB, oppure per RebootCounter
		if ((timeData.timeOn%3600==0)||(changedRGB==true)||(justRebooted==true)) {
			saveDataToNVS(&timeData, &rgbData);	
			ESP_LOGI(TAG, "****** saveDataToNVS ****** \n");	

			changedRGB = false;		
			justRebooted = false;	
		}	
	}		
}






