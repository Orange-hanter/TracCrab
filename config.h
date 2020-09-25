/*
Описание логических уровней для выбор режима:

    положение 2ws  
    2ws    0 v
    4ws   +5v
    crab  +5v

    положение 4ws  
    2ws   +5v
    4ws    0v
    crab  +5v

    положение crab  
    2ws   +5v
    4ws   +5v
    crab   0v

Распиновка:
    A5 - режим 2ws  (Digital Input)
    A4 - режим 4ws  (Digital Input)
    A3 - режим crab (Digital Input)

    A2 - нет 
    A1 - потенциометр 2 правое колесо здади
    A0 - потенциометр 1 правое колесо спереди

Распиновка колёс:
    LFront          RFront


    LBack           RBack

*/
//test

#define PIN_PTR_RFront A0
#define PIN_PTR_RBack A1

#define DI_MODE_2WS A5
#define DI_MODE_4WS A4
#define DI_MODE_CRAB A3

#define PIN_DRIVER_POWER_CH1 5
#define PIN_DRIVER_DIR_CH1 4
#define PIN_DRIVER_POWER_CH2 6
#define PIN_DRIVER_DIR_CH2 7

volatile long time_to_print = 0;
volatile long time_to_update_PTRs = 0;
volatile long time_to_update_channel = 0;
volatile long time_to_update_logic = 0;
volatile long time_to_update_PWM = 0;

const float maxValuePWM = 60; // maximum of output PWM power
const float minValuePWM = 17; // minimum

const float maxValueRBack = 90;
const float minValueRBack = 26;

const float zeroRFront = 45.5; // zero point of potentiometer in persent
const float zeroRBack = 38.5;

const float voltageMultiplyer = 5.0 / 1023.0;

int task;
float delta;
static int task_2 = -1;

const int task_K_a = 1;
const int task_K_b = 0;

#define task_monitor    false
#define monitor         true        //turn on monitoring via USB
#define BOOST_PWM       false

const char lebel_debug[] = "[\u001b[33mDEBUG\u001b[0m]: ";
static const char  lebel_info[] = "[\u001b[32mINFO \u001b[0m]: ";
static const char  lebel_warn[] = "[\u001b[31mWARN \u001b[0m]: ";