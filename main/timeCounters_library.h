/*  timeCounters_library.h
*/
#ifndef _TIME_COUNTERS_LIBRARY_H_
#define _TIME_COUNTERS_LIBRARY_H_
//pseudo-C-class timeData_t

typedef struct {
	uint32_t timeOn;
	uint32_t rebootsCount;
} timeData_t;

uint32_t getTimeOn(void);
void increaseTimeOn(int nSec);
uint32_t getRebootsCount(void);
void increaseRebootsCount(void);

void convertSec2stringHHSS(uint32_t, char*);

//void timeMeasure_task(void* arg);

#endif


