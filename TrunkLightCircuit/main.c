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
uint8_t gu8_FLASH_FREQ_PRESCALER;

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
            
            //flashes range from 2-20 (even numbers only); adc range 0-1024 so we convert
            //          the range from 1-10, then double it
            gu8_MAX_NUM_FLASHES = ((adc_read10_value() / 102.4) + 1) * 2;
            
            gb_NUM_ADC_CONVERSIONS = 0;
        }
        else
        {
            //switch from frequency to num_of_flashes
            gb_FLASH_SAMPLE_NOT_FREQ_SAMPLE = true;
            adc_select_input_channel(arr_adc_input[FLASH_NUM_INPUT_IDX]);
            
            //timer0 will control the speed of the brake light flashes from 1-10Hz
            //a flash is both ON and OFF, so really the range is effectively be 2-20Hz
            // Timer0 will overflow at a rate of 61.03Hz (w/1024 prescale)
            //                                  244.14Hz (w/256 prescale)   << we used this in the end 
            // so we will need an additional software prescaler to get to our desired
            // frequency range
            //
            //           1024   |  256
            // FREQ | PRESCALER | PRESCALER
            //------+-----------+-----------
            //  2   |  30       |   122.1
            //  4   |  15       |   61.0
            //  6   |  10       |   40.6
            //  8   |  7.5      |   30.5
            // 10   |  6        |   24.4
            // 12   |  5        |   20.5
            // 14   |  4.29     |   17.5
            // 16   |  3.75     |   15.3
            // 18   |  3.33     |   13.6    <--these changes get small
            // 20   |  3        |   12.2
            //flash freq 2-20Hz; adc range 0-1024 so we convert
            // Since we want to effectively double our frequency rather than doing 244Hz/val_1_to_10
            // we will do (244/2) / val_1_to_10 which is the same as our table
            //           ovf_freq/  ((    val is 0-9    ) now its 1-10)
            gu8_FLASH_FREQ_PRESCALER = 122 / ((adc_read10_value() / 1024) + 1);
            
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

//we only want this timer to do something at 20 Hz, since this interrupt is
//triggered every 61Hz, we need to skip every 3 overflows
ISR(TIMER0_OVF_vect)
{
    if (gb_BRAKE_ON)
    {
        if (gu8_NUM_TIMER0_OVF >= gu8_FLASH_FREQ_PRESCALER)
        {   
            //if we have not done all our flashes
            if (gu8_MAX_NUM_FLASHES < gu8_NUM_OCCURED_FLASHES)
            {
                gu8_NUM_OCCURED_FLASHES++;
            
                //if gu8_NUM_OCCURED_FLASHES is odd
                if (gu8_NUM_OCCURED_FLASHES %2)
                {
                    setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_LOW_BRIGHTNESS);
                }
                else
                {
                    setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_FULL_BRIGHTNESS);
                }
            }
            else
            {
                //if we already flashed, just stay solid
                setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_FULL_BRIGHTNESS);
            } 
        }
        else
        {
            gu8_NUM_TIMER0_OVF++;
        }
    }    
}

/** This interrupt is used as a 1 second "watchdog" timer for turn signals In the 
 *  combined function lights. When a turn signal is on, it does not perform brake
 *  duties till 1 second after the signal has been disabled. 
 *  Once an entire second goes by without turn signal input, we say that turn signal
 *  function is over.
 */
ISR(TIMER2_OVF_vect)
{
    //if the turn signal isn't on, we don't have to do anything with this interrupt
    if (gb_LEFT_TURN_SIGNAL_ON || gb_RIGHT_TURN_SIGNAL_ON)
    {
        if (gu8_NUM_TIMER2_OVF >= TIMER2_ADDTL_1_SEC_PRESCALE)
        {
            gu8_NUM_TIMER2_OVF = 0;
            //one second has elapsed without another turn signal lighting
            //up, we can set the turn signal flag low
            gb_LEFT_TURN_SIGNAL_ON = false;
            gb_RIGHT_TURN_SIGNAL_ON = false;
        }
        else
        {
            gu8_NUM_TIMER2_OVF++;
        }
    }
}

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
    
    //sets prescaler to 256 F_CPU = 16MHz
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 256) = 244.14Hz
    SetTimerPrescale(etimer_0, tmr_prscl_clk_over_256);
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

void init_timer2AsOneSecondTimer(void)
{
    //reset registers to a known state
    timer2_default();
    
    //sets prescaler to 1024 F_CPU = 16MHz
    //no real reason for this value, as long as its PWM we are fine
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 1024) = 61.03Hz
    SetTimerPrescale(etimer_2, tmr_prscl_clk_over_1024);
    
    enableTimerOverflowInterrupt(etimer_2);
}
#pragma endregion timers

/************************************************************************/
/*                               MAIN                                   */
/************************************************************************/
ISR(INT1_vect)
{
    // this is the only place these values are set, other places only read them
    // so even though there is a risk of reading a stale value, it is a non critical
    // error and will quickly be rectified
    if (BIT_SET(LIGHT_INPUT_PORT,BRAKE_IN))
    {
        gb_BRAKE_ON_FLAG = true;
        
        //this is for the software pre-scaler in ISR(TIMER0_OVF_VECT), which is only
        //used when gb_SEPERATE_FUNCTION_LIGHTS == true
        gu8_NUM_TIMER0_OVF = 0;
        
        //gb_SEPERATE_FUNCTION_LIGHTS == true we will flash the brake lights a certain
        //number of times every time the brake is pressed, so we reset it. its used in
        //ISR(TIMER0_OVF_VECT)
        gu8_NUM_OCCURED_FLASHES = 0;
    }
    else
    {
        gb_BRAKE_ON_FLAG = false;
    }
}

void init_external_interupts(void)
{
    //any logical change on int1 generates an interrupt request
    //set Interrupt Sense Control Bit
    BIT_SET(MCUCR,ISC10);
    
    BIT_SET(GICR,INT1);
}

void init_globals(void)
{
    gb_NUM_ADC_CONVERSIONS = 0;
    
    //the temptation might be to set these flags in a bit field in a single 8 bit register
    //but since they are set by multiple interrupts, we lessen the probability of data
    //corruption by keeping them separate
    gb_SEPARATE_FUNCTION_LIGHTS = false;
    gb_BRAKE_ON = false;
    gb_LEFT_TURN_SIGNAL_ON = false;
    gb_RIGHT_TURN_SIGNAL_ON = false;

    gu8_NUM_TIMER0_OVF = 0;
    gu8_NUM_TIMER2_OVF = 0;
}

void init_IO(void)
{
    //output = 1; input = 0
       
    //sets all pins to inputs
    DDRB = 0x00;
    DDRC = 0x00;
    DDRD = 0x00;
       
    //most of the pins are used as special functions.  So their DDR (data Direction Register)
    //definitions will be over-ridden anyway. These pins include:
    //  PB1 - PWM output OC1A, left  turn signal
    //  PB2 - PWM output OC1B, right turn signal
    //  PB3 - PWM output OC2,  brake light
    //  PB6 - OSC XTAL osc1
    //  PB7 - OSC XTAL osc2
    //  PC0 - ADC input flash freq
    //  PC1 - ADC input flash num
    //  PC2 - ADC input left  feedback/current measuring
    //  PC3 - ADC input brake feedback/current measuring
    //  PC4 - ADC input rigth feedback/current measuring
    //  PC6 - nRESET
    //  PD0 - UART Rx
    //  PD1 - UART Tx
    //  PD3 - External interrupt 1, brake input
    //
    //so now we set the remaining pins
    //  PD2 - Left input
    //  PD4 - Right input
    //  PD5 - Output status LED
    //sets output pins
    DDRD |= (1 << LED_OUTPUT_PIN);
       
    //clears output pins
    BIT_CLEAR(LED_OUTPUT_PORT, LED_OUTPUT_PIN);
}


void init(void)
{
    init_IO();
    init_globals();
    init_external_interupts();
    
    //enable global interrupts
    sei();
}

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
    gb_SEPARATE_FUNCTION_LIGHTS = true
        we have 3 light system
        we have a brake light circuti separate from the turn signals
        this will use flashing for brakes 100% gauranteed
    else
        we have two light system
        gb_SEPARATE_FUNCTION_LIGHTS = false
        a) disable brake output
        in a two light system we have only left and right lights. so they must
        do both turn signal and brake.... i think for clarity this also disables
        all flashing functionality???? 
   
   now that we've determined system type we can go to work with regular program
   */
    
    if (gb_SEPARATE_FUNCTION_LIGHTS)
    {
        // here we have a separate brake and turn signals. The turn signals only
        // need to handle themselves based on current input values
        
        // the lights are flashed by enabling and disabling the PWM output
        // this ensures no dim glow/leakage we would get if we left them on with 0% duty cycle
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], DUTY_CYCLE_FULL_BRIGHTNESS);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], DUTY_CYCLE_FULL_BRIGHTNESS);
        
        disablePWMOutput(arr_pwm_output[ARR_IDX_LEFT]);
        disablePWMOutput(arr_pwm_output[ARR_IDX_RIGHT]);
        
        //sets brake as running light
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_LOW_BRIGHTNESS);
        
        //timer0 is used for the flasher function
        enableTimerOverflowInterrupt(etimer_0);
    }
    else
    {
        //here we have no explicit brake light, only left and right lights, so they
        //must operate as brake AND turn signal
        
        //this will disable pwm on the brake light 
        //now timer2 will be repurposed as a 1 second timer
        //when turn signals are on we want it to go from full/off for contrast
        //if the signal doesn't go high for 1 second, we can resume brake duty
        init_timer2AsOneSecondTimer();
        
        //timer0 is used as flasher function, we wont have this for combined light mode
        //so we disable it completely
        timer0_default();
        
        //sets lights as running lights
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], DUTY_CYCLE_LOW_BRIGHTNESS);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], DUTY_CYCLE_LOW_BRIGHTNESS);
    }        
    
    while (1)
    {
        if (gb_SEPARATE_FUNCTION_LIGHTS)
        {
            // here we have a separate brake and turn signals. The turn signals only
            // need to handle themselves based on current input values,
            
            // we already set the duty cycle to 100%, now we are simply enabling or
            // disabling the output pin. The advantage of the method is it eliminates
            // the leakage/dim-glow if we simply set the PWM value to 0% duty cycle
            
            // LEFT TURN
            if (BIT_GET(LIGHT_INPUT_PORT, LEFT_IN))
            {
                enablePWMOutput(arr_pwm_output[ARR_IDX_LEFT]);
            }
            else
            {
                disablePWMOutput(arr_pwm_output[ARR_IDX_LEFT]);
                
            }
            
            //RIGHT TURN
            if (BIT_GET(LIGHT_INPUT_PORT, RIGHT_IN))
            {
                enablePWMOutput(arr_pwm_output[ARR_IDX_RIGHT]);
            }
            else
            {
                disablePWMOutput(arr_pwm_output[ARR_IDX_RIGHT]);
            }
            
            //BRAKE handled by interrupts (ext1 and timer0 overflow)
        }
        else
        {
            // this is where we only have a left light and a right light, the must handle both
            // brakes and turn signals.
            // We cannot simply set output based on input since we have to keep in mind what
            // the brake should be doing
            
            //brakes turn output
            //  0       0   low brightness
            //  0       1   alternate high/off (for contrast)
            //  1       0   high brightness
            //  1       1   alternate high/OFF (TURN SIGNAL OVERRIDES BRAKES)
            // after like 1 second of turn signal being off, we can resume regular brake duty
            
            // LEFT TURN
            if (BIT_GET(LIGHT_INPUT_PORT, LEFT_IN))
            {
                setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], DUTY_CYCLE_FULL_BRIGHTNESS);
                //you need the watchdog timer
                gu8_NUM_TIMER2_OVF = 0;
                gb_LEFT_TURN_SIGNAL_ON = true;
            }
            else
            {
                if (gb_LEFT_TURN_SIGNAL_ON)
                {
                    setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], DUTY_CYCLE_OFF_BRIGHTNESS);
                }
                else
                {
                    if (gb_BRAKE_ON)
                    {
                        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], DUTY_CYCLE_FULL_BRIGHTNESS);
                    }
                    else
                    {
                        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], DUTY_CYCLE_LOW_BRIGHTNESS);
                    }
                }
            }
            
            // RIGHT TURN
            if (BIT_GET(LIGHT_INPUT_PORT, RIGHT_IN))
            {
                setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], DUTY_CYCLE_FULL_BRIGHTNESS);
                //you need the watchdog timer
                gu8_NUM_TIMER2_OVF = 0;
                gb_RIGHT_TURN_SIGNAL_ON = true;
            }
            else
            {
                if (gb_RIGHT_TURN_SIGNAL_ON)
                {
                    setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], DUTY_CYCLE_OFF_BRIGHTNESS);
                }
                else
                {
                    if (gb_BRAKE_ON)
                    {
                        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], DUTY_CYCLE_FULL_BRIGHTNESS);
                    }
                    else
                    {
                        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], DUTY_CYCLE_LOW_BRIGHTNESS);
                    }
                }
            }
            
            //brake input and gb_BRAKE_ON is handled by external interrupt 1
        }
    }
}

