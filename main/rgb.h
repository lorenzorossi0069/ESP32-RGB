/* rgb.h
*/

#ifndef _RGB_H_
#define _RGB_H_

//???????? quale di questi serve per define di uint32_t ?????
#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 

void rgb_task(void * arg);

typedef struct  
{
	uint32_t R;
	uint32_t G;
	uint32_t B;
	//uint32_t W; //spare 
} rgbData_t;

//Set rgb methods as normalized values to 100%
void setR_FF(uint32_t);
void setG_FF(uint32_t);
void setB_FF(uint32_t);
//void setW_255(uint32_t); //spare 

//Set rgb methods as raw PWM values
void setR_PWM(uint32_t);
void setG_PWM(uint32_t);
void setB_PWM(uint32_t);
//void setW_PWM(uint32_t); //spare 

//Get rgb methods as normalized values to 100%
uint32_t getR_FF(void);
uint32_t getG_FF(void);
uint32_t getB_FF(void);
//uint32_t getW_255(void); //spare 

//Get rgb methods as raw PWM values
uint32_t getR_PWM(void);
uint32_t getG_PWM(void);
uint32_t getB_PWM(void);
//uint32_t getW_PWM(void); //spare 


void rgb_ledc_init(void);

#endif