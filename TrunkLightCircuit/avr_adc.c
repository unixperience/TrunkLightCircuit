/*
 * avr_adc.c
 *
 * Created: 12/19/2018 12:20:38 PM
 *  Author: Andrew Asuelime
 */ 

#include "avr_adc.h"
#include <avr/io.h>
#include "avr_uart.h"

void adc_reset(void)
{
    //reset to initial values
    ADMUX   = 0x00;
    ADCSRA  = 0x00;
}

void adc_enable_interrupt_on_conversion(void)
{
    //enables interrupt
    BIT_SET(ADCSRA,ADIE);
    sei();    //enables global interrupts
}

void adc_disable_interrupt_on_conversion(void)
{
    //disables interrupt
     BIT_CLEAR(ADCSRA,ADIE);
}

void adc_enableFreeRunningMode(void)
{
    BIT_SET(ADCSRA, ADFR);
}

void adc_disableFreeRunningMode(void)
{
    BIT_CLEAR(ADCSRA, ADFR);
}

void adc_select_input_channel(eADCInput value)
{
    // Normally I would write these as 4 BIT_SET/CLEAR instructions, but I'm
    // not sure the compiler would optimize this to one register write instruction
    // since this might be used in free running mode we want "atomic" writes 
    uint8_t curr_setting;
    
    //the input selection bits are the bottom 4
    curr_setting = ADMUX & 0xF0;
    
    ADMUX = curr_setting | value;
}

/*
bool select_adc_input(uint8_t adc_mux_input)
{
    if (adc_mux_input >= 8)
    return false;
    
    //clears all selections
    //ADMUX &= BIT_CLR(MUX3) & BIT_CLR(MUX2) & BIT_CLR(MUX1) & BIT_CLR(MUX0);
    ADMUX &= ~0x0F;
    
    //selects appropriate input
    //The mux select bits are 3..0, the input in range 0-3 is bits 2..0 so we can
    //simple set the mux to the value
    ADMUX |= adc_mux_input;
    
    return true;
}
*/

void adc_enable(void)
{
    BIT_SET(ADCSRA,ADEN);
}

void adc_disable(void)
{
     BIT_CLEAR(ADCSRA,ADEN);
}

void adc_left_shift_result(void)
{
    BIT_SET(ADMUX, ADLAR);
}

void adc_right_shift_result(void)
{
    BIT_CLEAR(ADMUX, ADLAR);
}

void adc_select_ref(eADCReference value)
{
    if (value == AREF_default)
    {
        BIT_CLEAR(ADMUX, REFS1);
        BIT_CLEAR(ADMUX, REFS0);
    }
    else if (value == AVcc)
    {
        BIT_CLEAR(ADMUX, REFS1);
        BIT_SET(ADMUX  , REFS0);
    }
    else if (value == Internal_2p56V)
    {
        BIT_SET(ADMUX  , REFS1);
        BIT_SET(ADMUX  , REFS0);
    }
}

bool adc_start_conversion(bool block_till_complete)
{
    //if the adc is not enabled return false
    if (!BIT_GET(ADCSRA, ADEN))
    {
        UART_transmitString("ERROR: ADC Disabled\r\n\0");
        return false;
    }        
    
    //set start conversion bit
    BIT_SET(ADCSRA, ADSC);
    
    //if we are blocking AND we are NOT in free running mode
    if (block_till_complete) // && !(BIT_GET(ADCSRA,ADFR)))
    {
        //loop until bit is clear
        while (BIT_GET(ADCSRA,ADSC))
        {
            ;    //do nothing
        }
    }
    
    return true;
}

uint8_t adc_read8H_value(void)
{
    return ADCH;
}

uint8_t adc_read8L_value(void)
{
    return ADCL;
}

uint16_t adc_read10_value(void)
{
    uint16_t ret_val = 0x0000;
    
    //must start with low register first
    //ret_val |= ADCL; //(ADCL >> 6);
    //ret_val |= (ADCH << 8);
    ret_val = ADC;
    ret_val &= MASK_10_BIT;
    
    return ret_val;
}

void adc_set_prescale(eADCPrescaleValues value)
{
    if (value == clk_over_2_default)
    {
        BIT_CLEAR(ADCSRA, ADPS2);
        BIT_CLEAR(ADCSRA, ADPS1); 
        BIT_CLEAR(ADCSRA, ADPS0);
    }
    else if (value == clk_over_4)
    {
        BIT_CLEAR(ADCSRA, ADPS2);
        BIT_SET(ADCSRA  , ADPS1);
        BIT_CLEAR(ADCSRA, ADPS0);
    }
    else if (value == clk_over_8)
    {
        BIT_CLEAR(ADCSRA, ADPS2);
        BIT_SET(ADCSRA  , ADPS1);
        BIT_SET(ADCSRA  , ADPS0);
    }
    else if (value == clk_over_16)
    {
        BIT_SET(ADCSRA  , ADPS2);
        BIT_CLEAR(ADCSRA, ADPS1);
        BIT_CLEAR(ADCSRA, ADPS0);
    }
    else if (value == clk_over_32)
    {
        BIT_SET(ADCSRA  , ADPS2);
        BIT_CLEAR(ADCSRA, ADPS1);
        BIT_SET(ADCSRA  , ADPS0);
    }
    else if (value == clk_over_64)
    {
        BIT_SET(ADCSRA  , ADPS2);
        BIT_SET(ADCSRA  , ADPS1);
        BIT_CLEAR(ADCSRA, ADPS0);
    }
    else if (value == clk_over_128)
    {
        BIT_SET(ADCSRA  , ADPS2);
        BIT_SET(ADCSRA  , ADPS1);
        BIT_SET(ADCSRA  , ADPS0);
    }
}