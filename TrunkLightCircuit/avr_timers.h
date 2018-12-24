/*
 * avr_timers.h
 * 
 *
 * Created: 12/17/2018 9:52:08 PM
 *  Author: Andrew
 */ 


#ifndef AVR_TIMERS_H_
#define AVR_TIMERS_H_

#include "global.h"

typedef enum _eTIMER
{
    etimer_0,
    etimer_1,
    etimer_2,
}eTIMER;

typedef enum _ePWM_OUTPUT
{
    epwm_1a,
    epwm_1b,
    epwm_2,
}ePWM_OUTPUT;

//NOT ALL VALUES ARE VALID FOR ALL TIMERS
typedef enum _eTimerPrescaleValues
{
    disabled_default,
    clk_over_1,
    clk_over_8,
    clk_over_64,
    clk_over_256,
    clk_over_1024,
    ext_tn_pin_falling_edge_timers_01_only,
    ext_tn_pin_rising_edge_timers_01_only,
    clk_over_32_timer2_only,
    clk_over_128_timer2_only,
} eTimerPrescaleValues;

//initialization 
void timers_init(void);
void timer0_init(void);
void timer1_init(void);
void timer2_init(void);

void timers_default(void);
void timer0_default(void);
void timer1_default(void);
void timer2_default(void);
bool set_pwm1a(uint8_t duty_cycle);
uint8_t get_pwm1a(void);
bool set_pwm1b(uint8_t duty_cycle);
void set_freq2(uint8_t unscaled_0_255);


/************************************************************************/
/*new timer interface                                                   */
/************************************************************************/
bool SetTimerPrescale(eTIMER timer, eTimerPrescaleValues prescale); 
void setPWMDutyCycle(ePWM_OUTPUT output_pin, uint8_t value_0_to_100);
void enablePWMOutput(ePWM_OUTPUT output_pin);
void disablePWMOutput(ePWM_OUTPUT output_pin);
#endif /* AVR_TIMERS_H_ */