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

/** Resets all timers to default settings
*/
void timers_default(void);

/** Resets timer0 to default settings. That is:
 * - disabled
 * - no interrupt/flag generation
 * - no prescalers selected
 */ 
void timer0_default(void);

/** Resets both channels of timer1 to default settings. That is:
 * - disabled
 * - no interrupt/flag generation
 * - no outputs enabled
 * - no waveform generation selected
 * - no prescalers selected
 */ 
void timer1_default(void);

/** Resets timer2 to default settings. That is:
 * - disabled
 * - no interrupt/flag generation
 * - no outputs enabled
 * - no waveform generation selected
 * - no prescalers selected
 */ 
void timer2_default(void);

/** Selects the system clock prescaler to use for the timer clock
 *  @NOTE some settings are timer specifc and will fail if used on the
 *  improper clock
 *  @PARAM timer - which timer to apply settings to
 *  @PARAM prescale - the selected prescale value (including external pin)
 *  @RETURN if setting was successfully applied
 */
bool SetTimerPrescale(eTIMER timer, eTimerPrescaleValues prescale); 

/** Sets the pwm duty cycle for a timer waveform generation model
 *  This function can be called even if PWM output is disabled
 *  @PARAM output_pin - waveform genearation pin to apply duty cycle to
 *  (this is not the same as the timer since some timers have multiple output
 *  channels)
 *  @PARAM value_0_to_100 - Desired duty cycle 0-100%, values >100 will
 *  be clipped to 100
 *  @ NOTE - all PWM is generated with 8 bit counter
 */
void setPWMDutyCycle(ePWM_OUTPUT output_pin, uint8_t value_0_to_100);

/** sets pwm duty cycle with more granular control range 0-255 (as opposed
 *  to setPWMDutyCycle which is 0-100)
 *  This function can be called even if PWM output is disabled
 *  @PARAM output_pin - waveform genearation pin to apply duty cycle to
 *  (this is not the same as the timer since some timers have multiple output
 *  channels)
 *  @PARAM val- Desired duty cycle 0-255
 *  @ NOTE - all PWM is generated with 8 bit counter
 */
void setPWMVal(ePWM_OUTPUT output_pin, uint8_t val);

/** enables PWM output of the specified pin
 *  @PARAM output_pin which output to enable
*  (this is not the same as the timer since some timers have multiple output
*  channels)
*/
void enablePWMOutput(ePWM_OUTPUT output_pin);

/** disables PWM output of the specified pin
 *  @PARAM output_pin which output to disable
*  (this is not the same as the timer since some timers have multiple output
*  channels)
*/
void disablePWMOutput(ePWM_OUTPUT output_pin);

/** enables overflow interrupt for select timer. The calling code MUST
 *  implement the actual ISR, This function is only a way to enable it
 *  without configuring registers directly.
 *  Timer will not start until clock source/prescaler is chosen, if a prescaler
 *  is not chosen interrupts will not fire. Valid ISR handles are
 *   ISR(TIMER0_OVF_vect)
 *   ISR(TIMER1_OVF_vect)
 *   ISR(TIMER2_OVF_vect)
 * @PARAM the timer you wish to enable interrupts for
 */
void enableTimerOverflowInterrupt(eTIMER val);

/** Disables interrupt generation for the specified timer
 * @PARAM the timer you wish to enable interrupts for
 */
void disableTimerOverflowInterrupt(eTIMER val);

#endif /* AVR_TIMERS_H_ */