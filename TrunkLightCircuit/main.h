/*
 * main.h
 *
 * Created: 12/17/2018 9:04:51 PM
 *  Author: Andrew
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include <avr/io.h>
#include "global.h"
#include "avr_uart.h"
#include "avr_adc.h"
#include "avr_timers.h"

/************************************************************************/
/*                                UART                                  */
/************************************************************************/
void init_uart_debug(void);

/************************************************************************/
/*                                 ADC                                  */
/************************************************************************/
#define FLASH_FREQ_INPUT_IDX 3
#define FLASH_NUM_INPUT_IDX 4
#define FLASH_CYCLES 30         ///this is an additional modulus on adc conversions, the flash
//this is the order
//notice we preserve ARR_IDX_xxxx ordering
//FEEDBACK_LEFT, FEEDBACK_BRAKE, FEEDBACK_RIGHT, FREQ_FLASH, NUM_FLASH
const eADCInput arr_adc_input[5] = {ADC2, ADC3, ADC4, ADC0, ADC1};

const eADCReference FLASH_REF = AVcc;
const eADCReference FDBK_RED  = Internal_2p56V;

//adc input
const uint16_t gbCURRENT_LIMIT = 50;

static volatile uint16_t arr_adc_conv_val[3] = {0};
static volatile uint8_t gb_NUM_ADC_CONVERSIONS = 0;

void init_adc();
ISR(ADC_vect);

/************************************************************************/
/*                               TIMERS                                 */
/************************************************************************/
//LEFT, BRAKE, RIGHT
const ePWM_OUTPUT arr_pwm_output[3] = {epwm_1a, epwm_2, epwm_1b};
    
void init_timers(void);
void init_timer0(void);
void init_timer1(void);
void init_timer2(void);

/************************************************************************/
/*                               MAIN                                   */
/************************************************************************/
#define ARR_IDX_LEFT    0
#define ARR_IDX_BRAKE   1
#define ARR_IDX_RIGHT   2

//LIGHT INPUTS PORT D
#define LIGHT_INPUT_PORT    PIND
#define LEFT_IN             PIND2   //this is also int0 pin
#define BRAKE_IN            PIND3   //this is also int1 pin
#define RIGHT_IN            PIND4

//pwm values for light levels
#define DUTY_CYCLE_FULL_BRIGHTNESS 100 /// used for braking or turn signal
#define DUTY_CYCLE_LOW_BRIGHTNESS   15 /// used for running lights
#define DUTY_CYCLE_OFF_BRIGHTNESS    0 /// turns lights off used for turn signal and/or brake flashing

bool gb_SEPARATE_FUNCTION_LIGHTS;
bool gb_BRAKE_ON;

#define TIMER0_ADDTL_PRESCALE 3

uint8_t gu8_NUM_TIMER0_OVF;
#endif /* MAIN_H_ */