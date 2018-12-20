/*
 * avr_adc.h
 *
 * Created: 12/19/2018 12:20:55 PM
 *  Author: Andrew
 */ 


#ifndef AVR_ADC_H_
#define AVR_ADC_H_

#include "global.h"

#define ADC_INPUT_FREQ      PINC1
#define ADC_INPUT_NUM_FLASH PINC0
#define MASK_10_BIT     0X03FF

enum eADCPrescaleValues
{
    //bottom 3 bits only, ADPS2...ADPS0 clear them then you can binary OR the enum
    clk_over_2_default  = 0x00,
    clk_over_4          = 0x02,
    clk_over_8          = 0x03,
    clk_over_16         = 0x04,
    clk_over_32         = 0x05,
    clk_over_64         = 0x06,
    clk_over_128        = 0x07,
};

enum eADCReference
{
    //top 2 bits of ADMUX only, REFS1...REFS0 clear them then you can binary OR the enum
    AREF_default    = 0x00, /**pin, Vref turned on. If there is a fixed voltage on this pin, 
                               no other selection will work despite the users selection*/
    AVcc            = 0x40, /**Ideally with external cap at AREF */
    Internal_2p56V  = 0xC0, /**Ideally with external cap at AREF */
};    

enum eADCInput
{
    //bottom 4 bits of ADMUX only, MUX3...MUX0 clear them then you can binary OR the enum
    ADC0        = 0x00,
    ADC1        = 0x01,
    ADC2        = 0x02,
    ADC3        = 0x03,
    ADC4        = 0x04,
    ADC5        = 0x05,
    ADC6        = 0x06,
    ADC7        = 0x07,
    REF_1P30v   = 0x0E,  /// Internal 1.30v reference value for internal test
    GND_0V      = 0x0F,  /// Internal 0v reference value for internal test
};
    
void adc_default(void);
void adc_enable_interrupt_on_conversion(void);
void adc_disable_interrupt_on_conversion(void);

/** selects the ADC channel
 @PARAM adc_mux_input value 0-7 corresponding to the ADC channel, if the 
 value is outside this value the state will not be changed, AND the function
 will return false
 @RETURN false adc_mux_input is out of range
         true otherwise
 */
void adc_select_input_channel(eADCInput value);
void adc_enable(void);
void adc_disable(void);

void adc_left_shift_result(void);
void adc_right_shift_result(void);

void adc_select_ref(eADCReference value);

/** 
 Starts a single adc conversion
 @PARAM block_till_complete if the function should wait for the conversion 
 to complete (true), or if it should simply start the conversion and 
 return(false)
 @RETURN false if ADC is not enabled
         true otherwise
 @NOTE if the function blocks, you are able to read the value after the
 function completes, it is guaranteed to correspond to the value read
 when this function was called. otherwise the value read might correspond
 to the previous conversion
 */
bool adc_start_conversion(bool block_till_complete);

/**
 reads the most recent 8bit value from the ADC. Recall adc is 10 bit
 this reads the 8 most significant bits(10-2)
 @RETURN the 8 bit value of last conversion
 */
uint8_t adc_read8_value(void);

/**
 reads the most recent 10bit value from the ADC.  
 @RETURN the 10 bit value of last conversion stored in bits 9-0
 the top bits 15-10 will be 0
 */
uint16_t adc_read10_value(void);

/**
 Sets the prescaler bits for the ADC  
 @RETURN n/a
 */
void adc_set_prescale(eADCPrescaleValues value);

#endif /* AVR_ADC_H_ */