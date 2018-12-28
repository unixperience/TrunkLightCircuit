/*
 * TrunkLightCircuit.c
 *
 * Created: 12/17/2018 8:57:07 PM
 * Author : Andrew
 */ 

#include "main.h"

/************************************************************************/
/* these are from the olf program                                       */

uint8_t gu8_MAX_NUM_FLASHES;
uint8_t gu8_FLASH_FREQ;

/**This flag is used to indicate the brake light should be on, it is set/cleared by external
 * interrupt1 handler and read by timer2 overflow handler
 */
bool gb_BRAKE_ON_FLAG;                 
uint8_t gu8_NUM_OCCURED_FLASHES;
bool gb_FLASH_SAMPLE_NOT_FREQ_SAMPLE;

/************************************************************************/


/************************************************************************/
/*                                UART                                  */
/************************************************************************/
#pragma region uart
void init_uart_debug(void)
{
    //clear all register configuration
    uart_default();
    
    //          baud    , msg length      , stop bits  , parity check
    uart_enable(b1000000, charlen8_default, one_default, disabled_default);
    UART_enableRxInterrupt();
}
#pragma endregion uart

/************************************************************************/
/*                                 ADC                                  */
/************************************************************************/
#pragma region adc
void init_adc()
{
    //For this program we need adc inputs 0-5
    //clear all registers and configuration
    adc_reset();
    
    //ideally the ADC clock should be between 50kHz-200kHz
    //you can still get lower resolution with a higher clock speed though
    //not sure what exactly this is, we may take this down to 125kHz
    #if (F_CPU == 8000000UL)
    //this is F_CPU / prescale = 8MHz/128 = 250kHz
    adc_set_prescale(clk_over_128);
    #elif if (F_CPU == 16000000UL)
    //this is F_CPU / prescale = 8MHz/128 = 250kHz
    adc_set_prescale(clk_over_64);
    #endif

    adc_right_shift_result();
    adc_enableFreeRunningMode();
    adc_enable_interrupt_on_conversion();
    adc_enable();
}

ISR(ADC_vect)
{
    //aia.test this needs a lot of work but only after you get the adc function done
    
    //with our ADC clock speed 125k/250k this should be ever 3-5 seconds
    if (gb_NUM_ADC_CONVERSIONS == FLASH_CYCLES)
    {
        //if we change input voltages we need to trash some values,
        
        if (gb_FLASH_SAMPLE_NOT_FREQ_SAMPLE)
        {
            //switch from num_flashes to frequency
            gb_FLASH_SAMPLE_NOT_FREQ_SAMPLE = false;
            adc_select_input_channel(arr_adc_input[FLASH_FREQ_INPUT_IDX]);
            
            //flashes range from 0-10; adc range 0-1024 so we convert
            gu8_MAX_NUM_FLASHES = adc_read10_value() / 102.4;
            
            gb_NUM_ADC_CONVERSIONS = 0;
        }
        else
        {
            //switch from frequency to num_of_flashes
            gb_FLASH_SAMPLE_NOT_FREQ_SAMPLE = true;
            adc_select_input_channel(arr_adc_input[FLASH_NUM_INPUT_IDX]);
            
            //flash freq 1-10Hz; adc range 0-1024 so we convert
            gu8_FLASH_FREQ = adc_read10_value();
            set_freq2(gu8_FLASH_FREQ);
            
            gb_NUM_ADC_CONVERSIONS = 0;
        }
    }
    else
    {
        //reads old value
        arr_adc_conv_val[gb_NUM_ADC_CONVERSIONS % 3] = adc_read10_value();
        
        //processes value
        if (arr_adc_conv_val[gb_NUM_ADC_CONVERSIONS % 3] > gbCURRENT_LIMIT)
        {
            disablePWMOutput(arr_pwm_output[gb_NUM_ADC_CONVERSIONS % 3]);
        }
        
        //moves to new input
        gb_NUM_ADC_CONVERSIONS++;
        adc_select_input_channel(arr_adc_input[gb_NUM_ADC_CONVERSIONS % 3]);
        
        //start next conversion, but don't wait for completion
        adc_start_conversion(false);
    }
}
#pragma endregion adc

/************************************************************************/
/*                               TIMERS                                 */
/************************************************************************/
#pragma region timers

//16bit reads -> read low -> read high
//16bit writes -> write high -> write low
void init_timers(void)
{
    //clears all timer related interrupts
    TIMSK = 0x00;
    
    //clears all timer related flag generation
    TIFR = 0x00;

    init_timer0();
    init_timer1();
    init_timer2();
}


void init_timer0(void)
{
    //resets counter
    TCNT0 = 0x00;
    
    //reset registers to a known state
    timer0_default();
    
    //timer0 can only generate overflow interrupts
    
    //sets prescaler to 1024 F_CPU = 16MHz
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 1024) = 61.03Hz
    // this is the slowest timer0 can go.
    SetTimerPrescale(etimer_0, tmr_prscl_clk_over_1024);
}

/* Timer 1 has two channels A and B, these channels will be used for the two directional
 * turn signals Left and Right (respectively). Since these two outputs have the same 
 * logical function they will be set up identically  
 */
void init_timer1(void)
{
    //reset registers to a known state
    timer1_default();
    
    //enables PWM on channels a and b
    enablePWMOutput(arr_pwm_output[ARR_IDX_LEFT]);
    enablePWMOutput(arr_pwm_output[ARR_IDX_RIGHT]);
    
    //sets prescaler to 64 F_CPU = 16MHz
    //no real reason for this value, as long as its PWM we are fine
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 488.3Hz
    SetTimerPrescale(etimer_1, tmr_prscl_clk_over_64);
}

/* Timer 2 has one PWM output pin OC2 and will be used for the brake light function in
 * a non-integrated lamp (separate output for left,brake,right). In an integrated system, 
 * this timer and output will be disabled
 */
void init_timer2(void)
{
    //reset registers to a known state
    timer2_default();
    
    //enable pwm
    enablePWMOutput(arr_pwm_output[ARR_IDX_BRAKE]);
    
    //sets prescaler to 64 F_CPU = 16MHz
    //no real reason for this value, as long as its PWM we are fine
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 488.3Hz
    SetTimerPrescale(etimer_2, tmr_prscl_clk_over_64);
}
#pragma endregion timers

/************************************************************************/
/*                               MAIN                                   */
/************************************************************************/
/** @brief Writes the current position of the cursor
 *         into the arguments row and col.
 *	Just a regular detailed description goes here
 *  it can be multiple lines
 *  @param row The address to which the current cursor
 *         row will be written.
 *  @param col The address to which the current cursor
 *         column will be written.
 *  @return Void.
 */
int main(void)
{
    char ret_data[_UART_RX_BUFF_MAX_LEN] = {0};
    char int_str[7] = {0};
    uint8_t ret_len;
    //0-input
    //1-output
    DDRB = 0xFF;
    PORTB = 0xAA;
    init_uart_debug();
    
    /*initialization order
    1. uart
    2. adc (must be done before timers)
    3. timers
    
    now set pwm of brake very low,
    monitor voltage levels, is there current flowing?
    if yes
        we have 3 light system
        we have a brake light circuti separate from the turn signals
        this will use flashing for brakes 100% gauranteed
    else
        we have two light system
        
        a) disable brake output
        in a two light system we have only left and right lights. so they must
        do both turn signal and brake.... i think for clarity this also disables
        all flashing functionality???? 
   
   now that we've determined system type we can go to work with regular program
   */
    
    
    /* Replace with your application code */
    while (1) 
    {
        _delay_ms(3000);
        //flash LED sequence
        _delay_ms(100);
        PORTB = 0x00;
        _delay_ms(100);
        PORTB = 0xFF;
        
        _delay_ms(100);
        PORTB = 0x00;
        _delay_ms(100);
        PORTB = 0xFF;
        
        _delay_ms(100);
        PORTB = 0x00;
        _delay_ms(100);
        PORTB = 0xFF;
        
        _delay_ms(100);
        PORTB = 0x00;
        _delay_ms(100);
        PORTB = 0xFF;
        
        _delay_ms(100);
        PORTB = 0x00;
        _delay_ms(100);
        PORTB = 0xFF;

        //this is supposed to read from the Rx Buffer
        UART_ReadRxBuff(ret_data, &ret_len);
        convertUint8ToChar(ret_len, int_str);
        UART_transmitBytes("\r\nReceived data bytes:", 22);
        UART_transmitBytes(int_str, 3);
        UART_transmitNewLine();
        UART_transmitBytes(ret_data, ret_len);
        UART_transmitNewLine();

    }
}

