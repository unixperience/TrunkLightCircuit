#include "global.h"
#include "avr_timers.h"

//16bit reads -> read low -> read high
//16bit writes -> write high -> write low
void timers_init(void)
{
    //clears all timer related interrupts
    TIMSK = 0x00;
    
    //clears all timer related flag generation
    TIFR = 0x00;

    timer0_init();
    timer1_init();
    timer2_init();
}


void timer0_init(void)
{
    TCCR0 = 0x00;
    
    //sets in CTC clear on compare mode
     BIT_SET(TCCR0, WGM21);
    
    //sets prescaler to 1024 F_CPU = 16MHz
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 1024) = 61.03Hz
    // this is the slowest timer0 can go. wed like much slower but 16bit timer is for PWM
    SetTimerPrescale(etimer_0, clk_over_64);
    
    //resets counter
    TCNT0 = 0x00;
    

}

/* Timer 1 has two channels A and B, these channels will be used for the two directional
 * turn signals Left and Right (respectively). Since these two outputs have the same 
 * logical function they will be set up identically  
 */
void timer1_init(void)
{
    //reset registers to a known state
    TCCR1A = 0x00;
    TCCR1B = 0x00;
    
    //enable OC1A and OC1B special function (no longer regular GPIO)
    //toggle pins OC1A and OC1B on compare match for fast PWM mode
    BIT_SET(TCCR1A, COM1A0);    //pin OC1A
    BIT_SET(TCCR1A, COM1B0);    //pin OC1B

    //sets timer as 8bit fast pwm (command spans to both AB registers)
    BIT_SET(TCCR1A, WGM10);
    BIT_SET(TCCR1B, WGM12);
    
    //sets prescaler to 64 F_CPU = 16MHz
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 488.3Hz
    SetTimerPrescale(etimer_1, clk_over_64);
    
    //sets to 50% duty cycle
    OCR1A = PWM_8BIT_50_DUTY_CYC;
    OCR1B = PWM_8BIT_50_DUTY_CYC;
    
    //resets 16bit counter in one atomic operation
    TCNT1 = 0x0000;
}

/* Timer 2 has one PWM output pin OC2 and will be used for the brake light function in
 * a non-integrated lamp (separate output for left,brake,right). In an integrated system, 
 * this timer and output will be disabled
 */
void timer2_init(void)
{
    TCCR2 = 0x00;
    
    //sets in CTC clear on compare mode
    TCCR2 |= BIT_SET(WGM21);
    
    //sets prescaler to 64 F_CPU = 16MHz
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 488.3Hz
    SetTimerPrescale(etimer_2, clk_over_64);
    
    //resets counter
    TCNT2 = 0x00;
    
    //initializes TOP value
    OCR2 = 0xFF;
    
    //enable output compare interrupt timer2
    TIMSK |= BIT_SET(OCIE2);
    sei();
}

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
    
    //clear flag generation
    BIT_CLEAR(TIFR, TOV0)       //overflow
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
    BIT_CLEAR(TIFR, TOV1)       //overflow
    BIT_CLEAR(TIFR, OCF1B)      //output b compare match
    BIT_CLEAR(TIFR, OCF1A)      //output a compare match
    BIT_CLEAR(TIFR, ICF1)       //input capture
    
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
    BIT_CLEAR(TIFR, TOV2)       //overflow
    BIT_CLEAR(TIMSK, OCF2);     //output compare match
    
    //compare value
    OCR2 = 0x00;
}

bool set_pwm1a(uint8_t duty_cycle)
{
    uint8_t saved_sreg;
    
    if (duty_cycle > 100)
    return false;
    
    //to prevent data collisions we must disable interrupts temporarily
    saved_sreg = SREG;    //saves global interrupt flag
    cli();                //disables interrupts
    
    OCR1A = ONE_PERCENT_OF_8_BIT * duty_cycle;
    
    SREG = saved_sreg;    //restores interrupts
    sei();
    
    return true;
}

uint8_t get_pwm1a(void)
{
    return (OCR1A / ONE_PERCENT_OF_8_BIT);
}

bool set_pwm1b(uint8_t duty_cycle)
{
    uint8_t saved_sreg;
    
    if (duty_cycle > 100)
    return false;
    
    //to prevent data collisions we must disable interrupts temporarily
    saved_sreg = SREG;    //saves global interrupt flag
    cli();             //disables interrupts
    
    OCR1B = ONE_PERCENT_OF_8_BIT * duty_cycle;
    
    SREG = saved_sreg;    //restores interrupts
    sei();
    
    return true;
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

/************************************************************************/
/*                                                                      */
/************************************************************************/
#define ONE_PERCENT_OF_8_BIT (256.0 / 100.0)

bool SetTimerPrescale(eTIMER timer, eTimerPrescaleValues prescale)
{
    bool ret_value = true;

    if (timer == etimer_0)
    {
        //clear all bits/none to known state
        BIT_CLEAR(TCCR0, CS12);
        BIT_CLEAR(TCCR0, CS11);
        BIT_CLEAR(TCCR0, CS10);

        if (value == disabled_default)
        {
            //dont do anything all bits are clear already
        }
        else if (value == clk_over_1)
        {
            BIT_SET(TCCR0, CS10);
        }
        else if (value == clk_over_8)
        {
            BIT_SET(TCCR0, CS11);
        }
        else if (value == clk_over_64)
        {
            BIT_SET(TCCR0, CS11);
            BIT_SET(TCCR0, CS10);
        }
        else if (value == clk_over_256)
        {
            BIT_SET(TCCR0, CS12);
        }
        else if (value == clk_over_1024)
        {
            BIT_SET(TCCR0, CS12);
            BIT_SET(TCCR0, CS10);
        }
        else if (value == ext_tn_pin_falling_edge_timers_01_only)
        {
            //this is T0 pin for timer 0
            BIT_SET(TCCR0, CS12);
            BIT_SET(TCCR0, CS11);
        }
        else if (value == ext_tn_pin_rising_edge_timers_01_only)
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

        if (value == disabled_default)
        {
            //dont do anything all bits are clear already
        }
        else if (value == clk_over_1)
        {
            BIT_SET(TCCR1B, CS10);
        }
        else if (value == clk_over_8)
        {
            BIT_SET(TCCR1B, CS11);
        }
        else if (value == clk_over_64)
        {
            BIT_SET(TCCR1B, CS11);
            BIT_SET(TCCR1B, CS10);
        }
        else if (value == clk_over_256)
        {
            BIT_SET(TCCR1B, CS12);
        }
        else if (value == clk_over_1024)
        {
            BIT_SET(TCCR1B, CS12);
            BIT_SET(TCCR1B, CS10);
        }
        else if (value == ext_tn_pin_falling_edge_timers_01_only)
        {
            //this is T1 pin for timer 1
            BIT_SET(TCCR1B, CS12);
            BIT_SET(TCCR1B, CS11);
        }
        else if (value == ext_tn_pin_rising_edge_timers_01_only)
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

        if (value == disabled_default)
        {
            //dont do anything all bits are clear already
        }
        else if (value == clk_over_1)
        {
            BIT_SET(TCCR2, CS10);
        }
        else if (value == clk_over_8)
        {
            BIT_SET(TCCR2, CS11);
        }
        else if (value == clk_over_32_timer2_only)
        {
            BIT_SET(TCCR2, CS11);
            BIT_SET(TCCR2, CS10);
        }
        else if (value == clk_over_64)
        {
            BIT_SET(TCCR2, CS12);
        }
        else if (value == clk_over_128_timer2_only)
        {
            BIT_SET(TCCR2, CS12);
            BIT_SET(TCCR2, CS10);
        }
        else if (value == clk_over_256)
        {
            //this is T1 pin for timer 1
            BIT_SET(TCCR2, CS12);
            BIT_SET(TCCR2, CS11);
        }
        else if (value == clk_over_1024)
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
        OCR1A = ONE_PERCENT_OF_8_BIT * value_0_to_100;
    }
    else if (output_pin == epwm_2)
    {
        OCR1A = ONE_PERCENT_OF_8_BIT * value_0_to_100;
    }
}

/// @NOTE currently this only supports non-inverted mode. It will also set 16 bit
/// timer into 8 bit mode
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
        BIT_CLEAR(TCCR1A  , COM1A1);
        BIT_CLEAR(TCCR1A, COM1A0);
    }
    else if (output_pin == epwm_1b)
    {
        //disconnect OC1B from Waveform Generation module
        BIT_CLEAR(TCCR1A  , COM1B1);
        BIT_CLEAR(TCCR1A, COM1B0);
    }
    else if (output_pin == epwm_2)
    {
        //disconnect OC2 from Waveform Generation module
        BIT_CLEAR(TCCR2, COM21);
        BIT_CLEAR(TCCR2, COM20);
    }
}
