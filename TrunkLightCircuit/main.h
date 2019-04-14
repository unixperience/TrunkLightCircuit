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
#include "StatusLED.h"

/************************************************************************/
/*                                UART                                  */
/************************************************************************/
void init_uart_debug(void);

/************************************************************************/
/*                                 ADC                                  */
/************************************************************************/
#define FLASH_FREQ_INPUT_IDX 3
#define FLASH_NUM_INPUT_IDX  4
#define FDBK_CYCLES         60      ///this is an additional modulus on adc conversions, the flash
//this is the order
//notice we preserve ARR_IDX_xxxx ordering
//FEEDBACK_LEFT, FEEDBACK_BRAKE, FEEDBACK_RIGHT, FREQ_FLASH, NUM_FLASH
#define ARR_IDX_FL_FREQ 3
#define ARR_IDX_FL_NUM  4
const eADCInput arr_adc_input[5] = {ADC4, ADC3, ADC2, ADC0, ADC1};

const eADCReference FLASH_REF = AVcc;
const eADCReference FDBK_REF  = AVcc; //aia.testInternal_2p56V;

typedef enum
{
    STATE_ADC_SWITCH_TO_5V_REF,
    STATE_ADC_READ_FEEDBACK,    
    STATE_ADC_READ_FREQ_FLASHES,
    STATE_ADC_READ_NUM_FLASHES,
    STATE_ADC_TEST,
}ADC_STATE_MACHINE;

volatile ADC_STATE_MACHINE ge_ADC_STATE;

//feedback ref is 5V. Feedback resistor is 0.680 Ohms. V=IR @1A: (1)*.68 = .68V
// (5V/1024) = 1 bit of adc resolution = 0.00488V
//feedback ref goes through non-inverting opamp Av = 6.55 
// V@1A / adc_resolution = (0.68V*6.55)/0.00488V = 912 = adc_reading at 1amp
#define FEEDBACK_1_AMP      912
#define FEEDBACK_1p12_AMP   1024    //max value
#define FEEDBACK_100_mAMP   91

//adc input
const uint16_t gbCURRENT_LIMIT = FEEDBACK_1_AMP;

static volatile uint16_t arr_adc_conv_val[3] = {0};
static volatile uint8_t gb_NUM_ADC_CONVERSIONS = 0;

void init_adc(bool enable_interrupts);
ISR(ADC_vect);

/************************************************************************/
/*                               TIMERS                                 */
/************************************************************************/
#define TIMER0_ADDTL_PRESCALE 3
// 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 976.5625Hz
// 976 / 4 Hz = ~122
#define TIMER1_ADDTL_4Hz_PRESCALE 244
// 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 1024) = 61.03Hz
//  to get down to 1 second we need to wait 61 ovf before it starts
#define TIMER2_ADDTL_1_SEC_PRESCALE 61

//the overflow interrupt should occur at 244Hz, we want the led to
//flash at 2Hz so we need to slow it down
#define TIMER0_ADDTL_2HZ_PRESCALE 122

volatile uint8_t gu8_NUM_TIMER0_OVF;
volatile uint8_t gu8_NUM_TIMER1_OVF;
volatile uint8_t gu8_NUM_TIMER2_OVF;

uint8_t gu8_heartbeat_prescale;

//LEFT, BRAKE, RIGHT
const ePWM_OUTPUT arr_pwm_output[3] = {epwm_1a, epwm_2, epwm_1b};
    
ISR(TIMER0_OVF_vect);   /** for LED flashing */
ISR(TIMER1_OVF_vect);   /** for status LED */
ISR(TIMER2_OVF_vect);   /** for Turn signal flag*/

void init_timers(void);
void init_timer0(void);
void init_timer1(void);
void init_timer2(void);

void init_timer2AsOneSecondTimer(void);

/************************************************************************/
/*                           RGB STATUS LED                             */
/************************************************************************/
#define LED_OUTPUT_PORT     PORTD
#define LED_B_OUTPUT_PIN    PIND5
#define LED_G_OUTPUT_PIN    PIND6
#define LED_R_OUTPUT_PIN    PIND7
#define LED_ACTIVE_LOW      true

void init_RGB_status_LED(void);
/************************************************************************/
/*                               MAIN                                   */
/************************************************************************/

typedef enum
{
    DebugDisabled,
    DebugADC,
    DebugUART,
    DebugPWM,
    DebugLED,
}eDEBUG_MODES;

#define ARR_IDX_LEFT    0
#define ARR_IDX_BRAKE   1
#define ARR_IDX_RIGHT   2

//LIGHT INPUTS PORT D
#define LIGHT_INPUT_PORT    PIND
#define LEFT_IN             PIND2   //this is also int0 pin
#define BRAKE_IN            PIND3   //this is also int1 pin
#define RIGHT_IN            PIND4

//Status LED
#define LED_OUTPUT_PORT     PORTD
#define LED_B_OUTPUT_PIN    PIND5
#define LED_G_OUTPUT_PIN    PIND6
#define LED_R_OUTPUT_PIN    PIND7
#define LED_OUTPUT_PIN      LED_R_OUTPUT_PIN    //aia.test you need to do mroe with status LED

//pwm values for light levels
#define DUTY_CYCLE_FULL_BRIGHTNESS 100 /// used for braking or turn signal
#define DUTY_CYCLE_LOW_BRIGHTNESS   15 /// used for running lights
#define DUTY_CYCLE_OFF_BRIGHTNESS    0 /// turns lights off used for turn signal and/or brake flashing

/**This flag is used to indicate the brake light should be on, it is set/cleared by external
 * interrupt1 handler and read by timer2 overflow handler
 */
volatile bool gb_BRAKE_ON;
volatile bool gb_LEFT_TURN_SIGNAL_ON;
volatile bool gb_RIGHT_TURN_SIGNAL_ON;
volatile bool gb_OVERCURRENT_TRIPPED;

volatile uint16_t gu8_MAX_NUM_FLASHES;
volatile uint16_t gu16_FLASH_FREQ_PRESCALER;
volatile uint8_t gu8_NUM_OCCURED_FLASHES;
volatile uint16_t gu16_adc_test_val;

// for brake input
ISR(INT1_vect);

void init_external_interupts(void);
void init_globals(void);
void init_IO(void);
void init(void);
#endif /* MAIN_H_ */