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
#endif /* MAIN_H_ */