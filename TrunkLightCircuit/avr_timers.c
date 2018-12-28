#include "global.h"
#include "avr_timers.h"

#define ONE_PERCENT_OF_8_BIT (256.0 / 100.0)


void timers_default(void)
{
    timer0_default();
    timer1_default();
    timer2_default();
}

void timer0_default(void)
{
    //clear configuration registers
    TCCR0 = 0x00;
    
    //clear interrupt generation
    BIT_CLEAR(TIMSK, TOIE0);    //overflow
    
    //clear flag 
    BIT_SET(TIFR, TOV0);      //overflow
}

void timer1_default(void)
{
    //clear configuration registers
    TCCR1A = 0x00;
    TCCR1B = 0x00;
    
    //clear interrupt generation
    BIT_CLEAR(TIMSK, TOIE1);    //overflow
    BIT_CLEAR(TIMSK, OCIE1B);   //output b compare match
    BIT_CLEAR(TIMSK, OCIE1A);   //output a compare match
    BIT_CLEAR(TIMSK, TICIE1);   //input capture
    
    //clear flag generation
    BIT_CLEAR(TIFR, TOV1);      //overflow
    BIT_CLEAR(TIFR, OCF1B);     //output b compare match
    BIT_CLEAR(TIFR, OCF1A);     //output a compare match
    BIT_CLEAR(TIFR, ICF1);      //input capture
    
    //compare value 16 bit registers
    OCR1A = 0x0000;
    OCR1B = 0x0000;
}

void timer2_default(void)
{
    //clear configuration registers
    TCCR2 = 0x00;
    OCR2  = 0X00;
    ASSR  = 0X00;
    
    //clear interrupt generation
    BIT_CLEAR(TIMSK, TOIE2);    //overflow
    BIT_CLEAR(TIMSK, OCIE2);    //output compare match
    
    //clear flag generation
    BIT_CLEAR(TIFR, TOV2);      //overflow
    BIT_CLEAR(TIMSK, OCF2);     //output compare match
    
    //compare value
    OCR2 = 0x00;
}

bool SetTimerPrescale(eTIMER timer, eTimerPrescaleValues value)
{
    bool ret_value = true;

    if (timer == etimer_0)
    {
        //clear all bits/none to known state
        BIT_CLEAR(TCCR0, CS12);
        BIT_CLEAR(TCCR0, CS11);
        BIT_CLEAR(TCCR0, CS10);

        if (value == tmr_prscl_disabled_default)
        {
            //dont do anything all bits are clear already
        }
        else if (value == tmr_prscl_clk_over_1)
        {
            BIT_SET(TCCR0, CS10);
        }
        else if (value == tmr_prscl_clk_over_8)
        {
            BIT_SET(TCCR0, CS11);
        }
        else if (value == tmr_prscl_clk_over_64)
        {
            BIT_SET(TCCR0, CS11);
            BIT_SET(TCCR0, CS10);
        }
        else if (value == tmr_prscl_clk_over_256)
        {
            BIT_SET(TCCR0, CS12);
        }
        else if (value == tmr_prscl_clk_over_1024)
        {
            BIT_SET(TCCR0, CS12);
            BIT_SET(TCCR0, CS10);
        }
        else if (value == tmr_prscl_ext_tn_pin_falling_edge_timers_01_only)
        {
            //this is T0 pin for timer 0
            BIT_SET(TCCR0, CS12);
            BIT_SET(TCCR0, CS11);
        }
        else if (value == tmr_prscl_ext_tn_pin_rising_edge_timers_01_only)
        {
            //this is T0 pin for timer 0
            BIT_SET(TCCR0, CS12);
            BIT_SET(TCCR0, CS11);
            BIT_SET(TCCR0, CS10);
        }
        else
        {
            ret_value = false;
        }
    }
    else if (timer == etimer_1)
    {
        //clear all bits/none to known state
        BIT_CLEAR(TCCR1B, CS12);
        BIT_CLEAR(TCCR1B, CS11);
        BIT_CLEAR(TCCR1B, CS10);

        if (value == tmr_prscl_disabled_default)
        {
            //dont do anything all bits are clear already
        }
        else if (value == tmr_prscl_clk_over_1)
        {
            BIT_SET(TCCR1B, CS10);
        }
        else if (value == tmr_prscl_clk_over_8)
        {
            BIT_SET(TCCR1B, CS11);
        }
        else if (value == tmr_prscl_clk_over_64)
        {
            BIT_SET(TCCR1B, CS11);
            BIT_SET(TCCR1B, CS10);
        }
        else if (value == tmr_prscl_clk_over_256)
        {
            BIT_SET(TCCR1B, CS12);
        }
        else if (value == tmr_prscl_clk_over_1024)
        {
            BIT_SET(TCCR1B, CS12);
            BIT_SET(TCCR1B, CS10);
        }
        else if (value == tmr_prscl_ext_tn_pin_falling_edge_timers_01_only)
        {
            //this is T1 pin for timer 1
            BIT_SET(TCCR1B, CS12);
            BIT_SET(TCCR1B, CS11);
        }
        else if (value == tmr_prscl_ext_tn_pin_rising_edge_timers_01_only)
        {
            //this is T1 pin for timer 1
            BIT_SET(TCCR1B, CS12);
            BIT_SET(TCCR1B, CS11);
            BIT_SET(TCCR1B, CS10);
        }
        else
        {
            ret_value = false;
        }
    }
    else if (timer == etimer_2)
    {
        //clear all bits/none to known state
        BIT_CLEAR(TCCR2, CS12);
        BIT_CLEAR(TCCR2, CS11);
        BIT_CLEAR(TCCR2, CS10);

        if (value == tmr_prscl_disabled_default)
        {
            //dont do anything all bits are clear already
        }
        else if (value == tmr_prscl_clk_over_1)
        {
            BIT_SET(TCCR2, CS10);
        }
        else if (value == tmr_prscl_clk_over_8)
        {
            BIT_SET(TCCR2, CS11);
        }
        else if (value == tmr_prscl_clk_over_32_timer2_only)
        {
            BIT_SET(TCCR2, CS11);
            BIT_SET(TCCR2, CS10);
        }
        else if (value == tmr_prscl_clk_over_64)
        {
            BIT_SET(TCCR2, CS12);
        }
        else if (value == tmr_prscl_clk_over_128_timer2_only)
        {
            BIT_SET(TCCR2, CS12);
            BIT_SET(TCCR2, CS10);
        }
        else if (value == tmr_prscl_clk_over_256)
        {
            //this is T1 pin for timer 1
            BIT_SET(TCCR2, CS12);
            BIT_SET(TCCR2, CS11);
        }
        else if (value == tmr_prscl_clk_over_1024)
        {
            //this is T1 pin for timer 1
            BIT_SET(TCCR2, CS12);
            BIT_SET(TCCR2, CS11);
            BIT_SET(TCCR2, CS10);
        }
        else
        {
            ret_value = false;
        }
    }

    return ret_value;
}

/// Values above 100 will be clipped to 100
/// @NOTE right now this code only works for timer1 in 8 bit mode!!
void setPWMDutyCycle(ePWM_OUTPUT output_pin, uint8_t value_0_to_100)
{
    if (value_0_to_100 > 100)
        value_0_to_100 = 100;

    if (output_pin == epwm_1a)
    {
        OCR1A = ONE_PERCENT_OF_8_BIT * value_0_to_100;
    }
    else if (output_pin == epwm_1b)
    {
        OCR1B = ONE_PERCENT_OF_8_BIT * value_0_to_100;
    }
    else if (output_pin == epwm_2)
    {
        OCR2 = ONE_PERCENT_OF_8_BIT * value_0_to_100;
    }
}

/// @NOTE right now this code only works for timer1 in 8 bit mode!!
void setPWMVal(ePWM_OUTPUT output_pin, uint8_t val)
{
    if (output_pin == epwm_1a)
    {
        OCR1A = val;
    }
    else if (output_pin == epwm_1b)
    {
        OCR1B = val;
    }
    else if (output_pin == epwm_2)
    {
        OCR2 = val;
    }
}

/// @NOTE currently this only supports non-inverted mode. It will also set 16 bit
/// timer into 8 bit mode. This function only enables the wave form generator and 
/// output pins. It doesn't not alter the counter/compare-match values
void enablePWMOutput(ePWM_OUTPUT output_pin)
{
    if (output_pin == epwm_1a)
    {
        //turn on PWM pin OC1A 8bit mode, note config spans 2 registers A/B
        BIT_CLEAR(TCCR1A, WGM13);
        BIT_CLEAR(TCCR1A, WGM12);
        BIT_SET(TCCR1B  , WGM12);
        BIT_CLEAR(TCCR1A, WGM11);
        BIT_SET(TCCR1A  , WGM10);
        
        //set as non-inverting (toggle on compare match)
        BIT_SET(TCCR1A  , COM1A1);
        BIT_CLEAR(TCCR1A, COM1A0);
    }
    else if (output_pin == epwm_1b)
    {
        //turn on PWM pin OC1A 8bit mode, note config spans 2 registers A/B
        BIT_CLEAR(TCCR1A, WGM13);
        BIT_CLEAR(TCCR1A, WGM12);
        BIT_SET(TCCR1B  , WGM12);
        BIT_CLEAR(TCCR1A, WGM11);
        BIT_SET(TCCR1A  , WGM10);
        
        //set as non-inverting (toggle on compare match)
        BIT_SET(TCCR1A  , COM1B1);
        BIT_CLEAR(TCCR1A, COM1B0);
    }
    else if (output_pin == epwm_2)
    {
        //turn on PWM pin OC2
        BIT_SET(TCCR2  , WGM21);
        BIT_SET(TCCR2  , WGM20);
        
        //set as non-inverting (toggle on compare match)
        BIT_SET(TCCR2  , COM21);
        BIT_CLEAR(TCCR2, COM20);
    }
}

///disconnects PWM pin (notice we only disconnected the pin, the PWM
///timer is actually still running with all settings unchanged)
void disablePWMOutput(ePWM_OUTPUT output_pin)
{
    if (output_pin == epwm_1a)
    {
        //disconnect OC1A from Waveform Generation module
        BIT_CLEAR(TCCR1A, COM1A1);
        BIT_CLEAR(TCCR1A, COM1A0);
    }
    else if (output_pin == epwm_1b)
    {
        //disconnect OC1B from Waveform Generation module
        BIT_CLEAR(TCCR1A, COM1B1);
        BIT_CLEAR(TCCR1A, COM1B0);
    }
    else if (output_pin == epwm_2)
    {
        //disconnect OC2 from Waveform Generation module
        BIT_CLEAR(TCCR2, COM21);
        BIT_CLEAR(TCCR2, COM20);
    }
}

void enableTimerOverflowInterrupt(eTIMER val)
{
    if (val == etimer_0)
    {
        BIT_SET(TIMSK, TOIE0);
    }
    else if (val == etimer_1)
    {
        BIT_SET(TIMSK, TOIE1);
    }
    else if (val == etimer_2)
    {
        BIT_SET(TIMSK, TOIE2);
    }
}

void disableTimerOverflowInterrupt(eTIMER val)
{
    if (val == etimer_0)
    {
        BIT_CLEAR(TIMSK, TOIE0);
    }
    else if (val == etimer_1)
    {
        BIT_CLEAR(TIMSK, TOIE1);
    }
    else if (val == etimer_2)
    {
        BIT_CLEAR(TIMSK, TOIE2);
    }
}

void set_freq2(uint8_t unscaled_0_255)
{
    //with the prescaler this interrupt occurs at 1MHz / 1024 = 976.5625Hz
    //we use a software prescaler of 4 to get it down to 244.140625Hz
    
    //we want the flash frequency ( a full on AND off cycle) to range between 1-10Hz
    //the extra 2 in the denominator is because a cycle is ON and OFF (2 cycles)
    // (freq_after_prescaler) / (top_of_timer * 2) = flash_frequency
    
    // MIN: 244Hz / (122 * 2) =  1Hz
    // MAX: 244Hz / ( 12 * 2) = 10Hz
    // so our effective range is 110 points
    
    //we need to map 256 => 110
    OCR2 =  (256.0/110.0) * unscaled_0_255 + 12;
}
