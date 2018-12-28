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
    tmr_prscl_disabled_default,
    tmr_prscl_clk_over_1,
    tmr_prscl_clk_over_8,
    tmr_prscl_clk_over_64,
    tmr_prscl_clk_over_256,
    tmr_prscl_clk_over_1024,
    tmr_prscl_ext_tn_pin_falling_edge_timers_01_only,
    tmr_prscl_ext_tn_pin_rising_edge_timers_01_only,
    tmr_prscl_clk_over_32_timer2_only,
    tmr_prscl_clk_over_128_timer2_only,
} eTimerPrescaleValues;

void timers_default(void);
void timer0_default(void);
void timer1_default(void);
void timer2_default(void);

bool SetTimerPrescale(eTIMER timer, eTimerPrescaleValues prescale); 
void setPWMDutyCycle(ePWM_OUTPUT output_pin, uint8_t value_0_to_100);
void setPWMVal(ePWM_OUTPUT output_pin, uint8_t val);
void enablePWMOutput(ePWM_OUTPUT output_pin);
void disablePWMOutput(ePWM_OUTPUT output_pin);

//////////////////////////////////////////////////////////////////////////
//aia.test //this was an old function, it doens't belong here but maybe in the main? idk
//we will decide later, i dont think i even need this exact code, but something similar
//ill keep it for reference
void set_freq2(uint8_t unscaled_0_255);
#endif /* AVR_TIMERS_H_ */