/*
 * TrunkLightCircuit.c
 *
 * Created: 12/17/2018 8:57:07 PM
 * Author : Andrew
 */ 

#include "main.h"

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
void init_adc(bool enable_interrupts)
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
    #elif (F_CPU == 16000000UL)
    //this is F_CPU / prescale = 8MHz/128 = 250kHz
    adc_set_prescale(clk_over_64);
    #endif

    adc_right_shift_result();
    
    if (enable_interrupts)
    {
        adc_enableFreeRunningMode();
        adc_enable_interrupt_on_conversion();
    }    
    
    adc_enable();
}

ISR(ADC_vect)
{
    if (ge_ADC_STATE == STATE_ADC_READ_FEEDBACK)
    {
        //reads old value
        arr_adc_conv_val[gb_NUM_ADC_CONVERSIONS % 3] = adc_read10_value();
            
        //processes value
        if (arr_adc_conv_val[gb_NUM_ADC_CONVERSIONS % 3] > gbCURRENT_LIMIT)
        {
            disablePWMOutput(arr_pwm_output[gb_NUM_ADC_CONVERSIONS % 3]);
                
            //this value can only be set. it is only cleared by a system reset
            gb_OVERCURRENT_TRIPPED = true;
                
            #ifdef DEBUG
            UART_transmitString("Overcurrent!\r\n\0");
            #endif // DEBUG
        }
            
        #ifdef DEBUG
        UART_transmitString("ADC\0");
        UART_transmitUint8(gb_NUM_ADC_CONVERSIONS % 3);
        UART_transmitString(":\0");
        UART_transmitUint16(arr_adc_conv_val[gb_NUM_ADC_CONVERSIONS % 3]);
        UART_transmitNewLine();
        #endif // DEBUG

        //moves to new input
        gb_NUM_ADC_CONVERSIONS++;
        
        //with our ADC clock speed 125k/250k this should be ever 3-5 seconds
        if (gb_NUM_ADC_CONVERSIONS > FDBK_CYCLES)
        {
            //move to next state
            adc_select_input_channel(arr_adc_input[ARR_IDX_FL_FREQ]);
            adc_select_ref(FLASH_REF);
            ge_ADC_STATE = STATE_ADC_SWITCH_TO_5V_REF;
        }
        else
        {
            adc_select_input_channel(arr_adc_input[gb_NUM_ADC_CONVERSIONS % 3]);
        }
            
        //start next conversion, but don't wait for completion
        adc_start_conversion(false);
    }
    else if (ge_ADC_STATE == STATE_ADC_SWITCH_TO_5V_REF)
    {
        //dont read the value, its bad after switch sources
        
        ge_ADC_STATE = STATE_ADC_READ_FREQ_FLASHES;
        //start next conversion, but don't wait for completion
        adc_start_conversion(false);
    }
    else if (ge_ADC_STATE == STATE_ADC_READ_FREQ_FLASHES)
    {
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
        gu8_FLASH_FREQ_PRESCALER = adc_read10_value(); //aia.teset 122.0 / ((adc_read10_value() / 102.4) + 1);
        
        #ifdef DEBUG
        UART_transmitString("fl freq:\0");
        UART_transmitUint8(gu8_FLASH_FREQ_PRESCALER);
        UART_transmitNewLine();
        #endif // DEBUG
        
        //move to next state
        adc_select_input_channel(arr_adc_input[ARR_IDX_FL_NUM]);
        ge_ADC_STATE = STATE_ADC_READ_NUM_FLASHES;
        adc_start_conversion(false);
    }
    else if (ge_ADC_STATE == STATE_ADC_READ_NUM_FLASHES)
    {
        //flashes range from 2-20 (even numbers only); adc range 0-1024 so we convert
        //          the range from 1-10, then double it
        gu8_MAX_NUM_FLASHES = adc_read10_value(); // aia.test ((adc_read10_value() / 102.4) + 1) * 2;
        
        #ifdef DEBUG
        UART_transmitString("fl num:\0");
        UART_transmitUint8(gu8_MAX_NUM_FLASHES);
        UART_transmitNewLine();
        #endif // DEBUG
        
        //move to next state
        ge_ADC_STATE = STATE_ADC_SWITCH_TO_2p56V_REF;
        
        gb_NUM_ADC_CONVERSIONS = 0;
        adc_select_ref(FDBK_REF);
        adc_select_input_channel(arr_adc_input[gb_NUM_ADC_CONVERSIONS % 3]);
        adc_start_conversion(false);
    }
    else if (ge_ADC_STATE == STATE_ADC_SWITCH_TO_2p56V_REF)
    {
        //dont read the value, its bad after switch sources
        
        //move to next state
        ge_ADC_STATE = STATE_ADC_READ_FEEDBACK;
        adc_start_conversion(false);
    }
    else if (ge_ADC_STATE == STATE_ADC_TEST)
    {
        //This isn't a regular runtime state, its just for setup and testing
        gu16_adc_test_val = adc_read10_value(); 
         
        UART_transmitString("adc:\0");
        UART_transmitUint8(gu16_adc_test_val);
        UART_transmitNewLine();

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
                    #ifdef DEBUG
                    UART_transmitString("flashing\0");
                    #endif // DEBUG
                    setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_LOW_BRIGHTNESS);
                }
                else
                {
                    setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_FULL_BRIGHTNESS);
                }
            }
            else
            {
                #ifdef DEBUG
                UART_transmitString("solid\0");
                #endif // DEBUG
                
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

/** this interrupt is used to flash the status led. As long as over current
 * wasn't detecteed. Over current will just light thee LED solid red
 */
ISR(TIMER1_OVF_vect)
{
    if (!gb_OVERCURRENT_TRIPPED)
    {
        if (gu8_NUM_TIMER1_OVF < TIMER1_ADDTL_4Hz_PRESCALE)
        {
            gu8_NUM_TIMER1_OVF++;
        }
        else
        {
            //toggle led
            LED_OUTPUT_PORT ^= (1 << LED_OUTPUT_PIN);
            gu8_NUM_TIMER1_OVF = 0;
        }
    }
    else
    {
        BIT_SET(LED_OUTPUT_PORT, LED_OUTPUT_PIN);
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
            
            #ifdef DEBUG
            UART_transmitString("Turn sig off\r\n\0");
            #endif // DEBUG
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
    
        
    //the status LED will be controlled by the turn signal overflow interrupt
    //this is because no matter if we are in separate function mode or not,
    //this time is always running
    enableTimerOverflowInterrupt(etimer_1);
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
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 976.5625Hz
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
    
    //sets prescaler to 64 F_CPU = 16MHz
    //no real reason for this value, as long as its PWM we are fine
    // 16MHz / (8_bit_max * prescaler) = 16MHz / (256 * 64) = 976.5625Hz
    SetTimerPrescale(etimer_2, tmr_prscl_clk_over_64);
    
    //enable pwm
    enablePWMOutput(arr_pwm_output[ARR_IDX_BRAKE]);
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
        gb_BRAKE_ON = true;
        
        //this is for the software pre-scaler in ISR(TIMER0_OVF_VECT), which is only
        //used when gb_SEPERATE_FUNCTION_LIGHTS == true
        gu8_NUM_TIMER0_OVF = 0;
        
        //gb_SEPERATE_FUNCTION_LIGHTS == true we will flash the brake lights a certain
        //number of times every time the brake is pressed, so we reset it. its used in
        //ISR(TIMER0_OVF_VECT)
        gu8_NUM_OCCURED_FLASHES = 0;
        
        #ifdef DEBUG
        UART_transmitString("Brake on\r\n\0");
        #endif // DEBUG
    }
    else
    {
        gb_BRAKE_ON = false;
        
        #ifdef DEBUG
        UART_transmitString("Brake off\r\n\0");
        #endif // DEBUG
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
    ge_ADC_STATE = STATE_ADC_SWITCH_TO_5V_REF;
    
    //the temptation might be to set these flags in a bit field in a single 8 bit register
    //but since they are set by multiple interrupts, we lessen the probability of data
    //corruption by keeping them separate
    gb_BRAKE_ON = false;
    gb_LEFT_TURN_SIGNAL_ON = false;
    gb_RIGHT_TURN_SIGNAL_ON = false;
    gb_OVERCURRENT_TRIPPED = false;

    gu8_MAX_NUM_FLASHES =  4;
    gu8_FLASH_FREQ_PRESCALER = 4;
    
    gu8_NUM_TIMER0_OVF = 0;
    gu8_NUM_TIMER1_OVF = 0;
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
    //  PB1 - PWM output OC1A, left  turn signal    (must be 'output' to activate driver)
    //  PB2 - PWM output OC1B, right turn signal    (must be 'output' to activate driver)
    //  PB3 - PWM output OC2,  brake light          (must be 'output' to activate driver)
    //  PD2 - Left input
    //  PD4 - Right input
    //  PD5 - Output status LED
    //sets output pins
    DDRB |= (1 << PINB1)
         |  (1 << PINB2)
         |  (1 << PINB3);
    DDRD |= (1 << LED_OUTPUT_PIN);
       
    //clears output pins
    BIT_CLEAR(LED_OUTPUT_PORT, LED_OUTPUT_PIN);
}


int main_pwm_test(void)
{
    init_uart_debug();
    init_timers();
    
    while(1)
    {
        UART_transmitString("L:100 r:  0\r\n\0");
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], 0);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], 100);        
        _delay_ms(2500);
        
        UART_transmitString("L: 75 r: 25\r\n\0");
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], 25);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], 75);
        _delay_ms(2500);
        
        UART_transmitString("L: 50 r: 50\r\n\0");
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], 50);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], 50);
        _delay_ms(2500);
        
        UART_transmitString("L: 25 r: 75\r\n\0");
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], 75);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], 25);
        _delay_ms(2500);
        
        UART_transmitString("L:  0 r:100\r\n\0");
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], 100);
        setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], 0);
        _delay_ms(2500);
    }
}

int main(void)
{
    uint16_t value1;
    uint16_t value2;
    
    
    init_uart_debug();
    init_adc(false);    
    UART_transmitString("Displaying adc value\r\n\0");
    init_timers();
    setPWMDutyCycle(arr_pwm_output[ARR_IDX_RIGHT], 100);
    setPWMDutyCycle(arr_pwm_output[ARR_IDX_LEFT], 100);
    
    while(1)
    {
        adc_select_ref(FLASH_REF);
        
        adc_select_input_channel(arr_adc_input[ARR_IDX_FL_NUM]);
        adc_start_conversion(true);
        value1 = adc_read10_value();        
        
        adc_select_input_channel(arr_adc_input[ARR_IDX_FL_FREQ]);
        adc_start_conversion(true);
        value2 = adc_read10_value();
        
        //print
        UART_transmitString("Num:\0");
        UART_transmitUint16(value1);
        UART_transmitString(" Freq:\0");
        UART_transmitUint16(value2);
        
        adc_select_ref(FDBK_REF);
        adc_select_input_channel(arr_adc_input[ARR_IDX_LEFT]);
        adc_start_conversion(true);
        value1 = adc_read10_value();
        
        adc_select_input_channel(arr_adc_input[ARR_IDX_RIGHT]);
        adc_start_conversion(true);
        value2 = adc_read10_value();
        
        //print
        UART_transmitString(" Left:\0");
        UART_transmitUint16(value1);
        UART_transmitString(" Right:\0");
        UART_transmitUint16(value2);
        UART_transmitNewLine();
        _delay_ms(250);
    }
    
    return 0;
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
int main_old(void)
{
    //in the future you may use these
    char ret_data[_UART_RX_BUFF_MAX_LEN] = {0};
    uint8_t num;
    uint8_t ret_len;
    uint16_t brake_light_test_reading;
    bool separate_function_lights;
    bool debug_mode_enabled = false;
    eDEBUG_MODES curr_debug_mode;
    
    /*initialization order
    1. uart
    2. adc (must be done before timers)
    3. timers    
    */
#ifdef DEBUG
    init_uart_debug();
    
    UART_transmitString("Trunk Light FW starting v 1.0\r\n\0");
#endif // DEBUG

    init_IO();
    init_globals();
    init_external_interupts();
    //no interrupts
    init_adc(false);
    init_timers();
        
    //enable global interrupts
    sei();

    /*    
    now set pwm of brake very low,
    monitor voltage levels, is there current flowing?
    if yes
    separate_function_lights = true
        we have 3 light system
        we have a brake light circuti separate from the turn signals
        this will use flashing for brakes 100% gauranteed
    else
        we have two light system
        separate_function_lights = false
        a) disable brake output
        in a two light system we have only left and right lights. so they must
        do both turn signal and brake.... i think for clarity this also disables
        all flashing functionality???? 
   
    now that we've determined system type we can go to work with regular program
    */
    setPWMDutyCycle(arr_pwm_output[ARR_IDX_BRAKE], DUTY_CYCLE_FULL_BRIGHTNESS);
    adc_select_ref(FDBK_REF);
    adc_select_input_channel(arr_adc_input[ARR_IDX_FL_FREQ]);
    _delay_ms(10);
    //adc start conversion and wait for result, normally the first one is inaccurate
    //so we wont even check it
    adc_start_conversion(true);
    _delay_ms(8000);
    
    adc_start_conversion(true);
    brake_light_test_reading = adc_read10_value();
    
    #ifdef DEBUG
    UART_transmitString("Brake value:\0");
    UART_transmitUint16(brake_light_test_reading);
    UART_transmitNewLine();
    #endif // DEBUG
    
    //init adc for program use, with interrupts
    init_adc(true);
    
    //if we have at least 100mA flowing, we know we have a brake light connected
    if (brake_light_test_reading >= FEEDBACK_100_mAMP)
    {
        separate_function_lights = true;
        #ifdef DEBUG
        UART_transmitString("Detected Separate fn\r\n\0");
        #endif // DEBUG
    }
    else
    {
        separate_function_lights = false;
        #ifdef DEBUG
        UART_transmitString("Detected Integrated fn\r\n\0");
        #endif // DEBUG
    }
    
    if (separate_function_lights)
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
    
    //start adc state machine
    //adc_start_conversion(false);

    while (1)
    {
        UART_ReadLineRxBuff(ret_data, &ret_len);
        
        if (ret_len > 0)
        {
            if (ret_data[0] == 'd')
            {
                UART_transmitString("Entering debug mode\r\n\0");
                debug_mode_enabled = true;
                curr_debug_mode = DebugDisabled;
            }
            
            if (debug_mode_enabled)
            {
                if (curr_debug_mode == DebugDisabled)
                {
                    if (ret_data[0] == 'Q')
                    {
                        UART_transmitString("Leaving debug mode, reset device for normal operation\r\n\0");
                        debug_mode_enabled = false;
                    }
                    else if (ret_data[0] == 'a' && ret_data[1] == 'd' && ret_data[2] == 'c')
                    {
                        curr_debug_mode = DebugADC;
                        UART_transmitString("Starting ADC debug mode");
                    
                        adc_disable_interrupt_on_conversion();
                        ge_ADC_STATE = STATE_ADC_TEST;
                        adc_select_ref(AVcc);
                        adc_select_input_channel(ADC4);
                        adc_enable_interrupt_on_conversion();                    
                    
                        adc_start_conversion(false);
                    }
                    else if (ret_data[0] == 'u')
                    {
                        curr_debug_mode = DebugUART;
                        UART_transmitString("Starting uart debug mode");
                    }
                    else if ((ret_data[0] == 'p') && (ret_data[1] == 'w') && (ret_data[2] == 'm'))
                    {
                        curr_debug_mode = DebugPWM;
                        UART_transmitString("Starting PWM debug mode");
                        //ensures PWM is running
                        init_timers();
                    }
                }//if (curr_debug_mode == DebugDisabled)
                else if (curr_debug_mode == DebugADC)
                {
                    if (ret_data[0] == 'Q')
                    {
                        UART_transmitString("Leaving adc debug mode\r\n\0");
                        curr_debug_mode = DebugDisabled;
                        
                        adc_disable_interrupt_on_conversion();
                    }
                    else if (ret_data[0] == 'g')
                    {
                        UART_transmitString("Displaying ground\r\n\0");
                        adc_select_input_channel(GND_0V_mega8);
                    }
                    else if (ret_data[0] == 't')
                    {
                        UART_transmitString("Displaying 1.30V reference\r\n\0");
                        adc_select_input_channel(REF_1P30v_mega8);
                    }
                    else if (ret_data[0] == 'r')
                    {
                        if (ret_data[1] == '5')
                        {
                            UART_transmitString("Setting Reference to 5V Vcc\r\n\0");
                            adc_select_ref(AVcc);
                        }
                        if (ret_data[1] == '2')
                        {
                            UART_transmitString("Setting reference to 2.56V\r\n\0");
                            adc_select_ref(Internal_2p56V);
                        }
                    }
                    else if (ret_data[0] == '0')
                    {
                        UART_transmitString("Displaying Channel 0\r\n\0");
                        adc_select_input_channel(ADC0);
                        adc_start_conversion(true);
                        brake_light_test_reading = adc_read10_value();
                        UART_transmitUint16(brake_light_test_reading);
                        UART_transmitNewLine();
                        
                    }
                    else if (ret_data[0] == '1')
                    {
                        UART_transmitString("Displaying Channel 1\r\n\0");
                        adc_select_input_channel(ADC1);
                        adc_start_conversion(true);
                        brake_light_test_reading = adc_read10_value();
                        UART_transmitUint16(brake_light_test_reading);
                        UART_transmitNewLine();
                    }
                    else if (ret_data[0] == '2')
                    {
                        UART_transmitString("Displaying Channel 2\r\n\0");
                        adc_select_input_channel(ADC2);
                        adc_start_conversion(true);
                        brake_light_test_reading = adc_read10_value();
                        UART_transmitUint16(brake_light_test_reading);
                        UART_transmitNewLine();
                    }
                    else if (ret_data[0] == '3')
                    {
                        UART_transmitString("Displaying Channel 3\r\n\0");
                        adc_select_input_channel(ADC3);
                        adc_start_conversion(false);
                        _delay_ms(100);
                        brake_light_test_reading = adc_read10_value();
                        UART_transmitUint16(brake_light_test_reading);
                        UART_transmitNewLine();
                    }
                    else if (ret_data[0] == '4')
                    {
                        UART_transmitString("Displaying Channel 4\r\n\0");
                        adc_select_input_channel(ADC4);
                        adc_start_conversion(false);
                        _delay_ms(100);
                        brake_light_test_reading = adc_read10_value();
                        UART_transmitUint16(brake_light_test_reading);
                        UART_transmitNewLine();
                    }
                }//else if (curr_debug_mode == DebugADC)
                /*
                else if (curr_debug_mode == DebugPWM)
                {
                    if (ret_data[0] == 'Q')
                    {
                        UART_transmitString("Leaving PWM debug mode\r\n\0");
                        curr_debug_mode = DebugDisabled;
                        init_timers();
                    }
                    else if (ret_data[0] == 's')
                    {
                        num = ret_data[4] - 48;
                        num *= 10;
                        
                        UART_transmitString("PWM:");
                        UART_transmitUint8(num);
                        UART_transmitNewLine();
                        if ((ret_data[1] == '1') || (ret_data[2] == 'a'))
                        {
                            setPWMDutyCycle(epwm_1a,num);
                        } 
                        else if ((ret_data[1] == '1') || (ret_data[2] == 'b'))
                        {
                            setPWMDutyCycle(epwm_1b,num);
                        }
                        else if (ret_data[1] == '2')
                        {
                            setPWMDutyCycle(epwm_2,num);
                        }
                    }//else if (ret_data[0] == 's')
                    else if (ret_data[0] == 'd')
                    {
                        if ((ret_data[1] == '1') || (ret_data[2] == 'a'))
                        {
                            disablePWMOutput(epwm_1a);
                        }
                        else if ((ret_data[1] == '1') || (ret_data[2] == 'b'))
                        {
                            disablePWMOutput(epwm_1b);
                        }
                        else if (ret_data[1] == '2')
                        {
                            disablePWMOutput(epwm_2);
                        }
                    }//else if (ret_data[0] == 'd')
                    else if (ret_data[0] == 'e')
                    {
                        if ((ret_data[1] == '1') || (ret_data[2] == 'a'))
                        {
                            enablePWMOutput(epwm_1a);
                        }
                        else if ((ret_data[1] == '1') || (ret_data[2] == 'b'))
                        {
                            enablePWMOutput(epwm_1b);
                        }
                        else if (ret_data[1] == '2')
                        {
                            enablePWMOutput(epwm_2);
                        }
                    }//else if (ret_data[0] == 'e') 
                }// else if (curr_debug_mode == DebugPWM) */
            }//if (debug_mode_enabled)
            
//             
//             adc_select_input_channel(arr_adc_input[ARR_IDX_FL_FREQ]);
//             UART_transmitString("sc\r\n\0");
//             if (!adc_start_conversion(true))
//             UART_transmitString("ADC ERROR not enabled\0");
//                             
//             adc_val = adc_read10_value();
//                             
//             UART_transmitString("FREQ:\0");
//             UART_transmitUint16(adc_val);
//                             
//             adc_select_input_channel(arr_adc_input[ARR_IDX_FL_NUM]);
//             adc_start_conversion(true);
//             adc_val = adc_read10_value();
//                             
//             UART_transmitString("     NUM:\0");
//             UART_transmitUint16(adc_val);
                            
            UART_transmitNewLine();
            
        }//end ret_len > 0
        
        if (separate_function_lights)
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
                #ifdef DEBUG
                UART_transmitString("LEFT on\r\n\0");
                #endif // DEBUG
            }
            else
            {
                disablePWMOutput(arr_pwm_output[ARR_IDX_LEFT]);
            }
            
            //RIGHT TURN
            if (BIT_GET(LIGHT_INPUT_PORT, RIGHT_IN))
            {
                enablePWMOutput(arr_pwm_output[ARR_IDX_RIGHT]);
                #ifdef DEBUG
                UART_transmitString("RIGHT on\r\n\0");
                #endif // DEBUG
            }
            else
            {
                disablePWMOutput(arr_pwm_output[ARR_IDX_RIGHT]);
            }
            
            //BRAKE handled by interrupts (ext1 and timer0 overflow)
            
        }//if (separate_function_lights)
        else //if (!separate_function_lights)
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
                
                #ifdef DEBUG
                UART_transmitString("Left on\r\n\0");
                #endif // DEBUG
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
                
                #ifdef DEBUG
                UART_transmitString("Right on\r\n\0");
                #endif // DEBUG
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
            
        }//end else (!seperate_function_lights)
    }//end while(1)
    
}//main

