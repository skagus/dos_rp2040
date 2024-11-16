#pragma once

#define OS_MSEC(x)		(x)
#define MS_PER_TICK     (10)

typedef enum _TaskId
{
	TID_CONSOLE,
	TID_LED,
	TID_ADC,
	TID_BUT,
	TID_LED_MAT,
	NUM_TASK,
} TaskId;

typedef enum _Evt
{
	EVT_TICK,
	EVT_UART,
	EVT_LED_CMD,
	EVT_LED_MAT,
	EVT_BUT_CHANGE,
	NUM_EVT,
} EvtId;
