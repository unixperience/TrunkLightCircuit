/*
 * avr_uart.c
 *
 * Created: 12/21/2018 12:54:25 PM
 *  Author: Andrew
 */ 


//TXC must be cleared before each transmission (before udr is written)
//RXC checks for existing Rx data in the receive buffer
//when you write to UCSRC you must set URSEL bit (MSB) since i/o location is shared by
//  UBRRH and UCSRC
// anytime you write to UCSRA register you must set UDRE bit low


#include "avr_uart.h"
#include <util/atomic.h>
#include <avr/interrupt.h>

#define ASCII_0         48
#define UCSRC_CLEAR_VAL 0x80  /// This clears register UCSRC, all writes to the reg must have
                              /// URSEL bit (MSB 7) set since this register is shared with UBRRH'
                             
static volatile char rcv_buff[_UART_RX_BUFF_MAX_LEN] = {0};
static volatile uint8_t rcv_buff_len = 0;

/************************************************************************/
/* UART CONFIGURATION FUNCTIONS                                         */
/************************************************************************/

/** Private function _uart_ReadUCSRC reads the UART Control Status Register C
    this register has a special read sequence, so it is put into a helper function
    for ease of use
    @RETURN uint8_t the value of UCSRC
    */
uint8_t _uart_ReadUCSRC(void)
{
    uint8_t ret_value;
    
    //this is a special timed sequence, since UCSRC and UBRRH
    //are the same physical register, there is a special procedure
    //to read them.
    //To read UCSRC you must read the register twice, atomically
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        ret_value = UBRRH;
        ret_value = UCSRC;
    }
    
    return (ret_value);
}

void uart_default(void)
{
    //configuration registers
    UCSRA = 0x00;
    UCSRB = 0x00;
    UCSRC = UCSRC_CLEAR_VAL;
    
    //vbaud rate selection
    UBRRL = 0x00;
    UBRRH = 0x00;    
}

void uart_SetCharWidth(eUARTCharSize value)
{
    uint8_t ucsrc_val;
    
    ucsrc_val = _uart_ReadUCSRC();
    
    //all UCSRC writes must have URSEL bit 7 set
    ucsrc_val |= (1 << URSEL);
    
    if (value == charlen5)
    {
        BIT_CLEAR(UCSRB     , UCSZ2);
        BIT_CLEAR(ucsrc_val , UCSZ1);
        BIT_CLEAR(ucsrc_val , UCSZ0);
    }
    else if (value == charlen6)
    {
        BIT_CLEAR(UCSRB     , UCSZ2);
        BIT_CLEAR(ucsrc_val , UCSZ1);
        BIT_SET(ucsrc_val   , UCSZ0);
    }
    else if (value == charlen7)
    {
        BIT_CLEAR(UCSRB     , UCSZ2);
        BIT_SET(ucsrc_val   , UCSZ1);
        BIT_CLEAR(ucsrc_val , UCSZ0);
    }
    else if (value == charlen8_default)
    {
        BIT_CLEAR(UCSRB     , UCSZ2);
        BIT_SET(ucsrc_val   , UCSZ1);
        BIT_SET(ucsrc_val   , UCSZ0);
    }
    else if (value == charlen9)
    {
        BIT_SET(UCSRB       , UCSZ2);
        BIT_SET(ucsrc_val   , UCSZ1);
        BIT_SET(ucsrc_val   , UCSZ0);
    }
    
    UCSRC = ucsrc_val;
}    


void uart_SetParity(eUartParityMode value)
{
    uint8_t ucsrc_val;
    
    ucsrc_val = _uart_ReadUCSRC();
    
    //all UCSRC writes must have URSEL bit 7 set
    ucsrc_val |= (1 << URSEL);
    
    if (value == disabled_default)
    {
        BIT_CLEAR(ucsrc_val , UPM1);
        BIT_CLEAR(ucsrc_val , UPM0);
    }
    else if (value == enabled_even_parity)
    {
        BIT_SET(ucsrc_val   , UPM1);
        BIT_CLEAR(ucsrc_val , UPM0);
    }
    else if (value == enabled_odd_parity)
    {
        BIT_SET(ucsrc_val   , UPM1);
        BIT_SET(ucsrc_val   , UPM0);
    }
    
    UCSRC = ucsrc_val;
}

void uart_SetStopBits(eUARTStopBits value)
{
    uint8_t ucsrc_val;
    
    ucsrc_val = _uart_ReadUCSRC();
    
    //all UCSRC writes must have URSEL bit 7 set
    ucsrc_val |= (1 << URSEL);
    
    if (value == one_default)
    {
        BIT_CLEAR(ucsrc_val ,USBS);
    }        
    else if (value == two)
    {
        BIT_SET(ucsrc_val   ,USBS);
    }     
    
    UCSRC = ucsrc_val;   
}

//Baud rate calculation
// UBRR = [Fosc / (16*BAUD)] -1
bool uart_SetBaudRate(eUartBaudRate value)
{
    BIT_SET(UCSRA, U2X);    //enables double data rate, only valid for async mode (UMSEL=0) 
                            //which is all we support
    
#if (F_CPU==16000000UL)
    if (value == b1000000)
    {
        UBRRH = 0x00;
        UBRRL = 0x01;
    }
    else if (value == b250000)
    {
        UBRRH = 0x00;
        UBRRL = 0x07;
    }
    else if (value == b115200)
    {
        UBRRH = 0x00;
        UBRRL = 0x10;   //16 decimal
    }
    else if (value == b19200)
    {
        UBRRH = 0x00;
        UBRRL = 0x67;   //103 decimal
    }
    else //(value == b9600_default)
    {
        UBRRH = 0x00;
        UBRRL = 0xCF;   //207 decimal
    }
    
    return true;
#elif F_CPU==8000000UL
    if (value == b1000000)
    {
        UBRRH = 0x00;
        UBRRL = 0x00;
    }
    else if (value == b250000)
    {
        UBRRH = 0x00;
        UBRRL = 0x03;
    }
    else if (value == b115200)
    {
        UBRRH = 0x00;
        UBRRL = 0x08; 
    }
    else if (value == b19200)
    {
        UBRRH = 0x00;
        UBRRL = 0x33;   //51 decimal
    }
    else //(value == b9600_default)
    {
        UBRRH = 0x00;
        UBRRL = 0x67;   //103 decimal
    }
    
    return true;
#elif F_CPU==1000000UL
    if (value == b19200)
    {
        UBRRH = 0x00;
        UBRRL = 0x06;
    }
    else //(value == b9600_default)
    {
        UBRRH = 0x00;
        UBRRL = 0x0C;
    }     
    
    return true;
#else //if ((F_CPU != 16000000UL) &&  (F_CPU != 8000000UL) &&  (F_CPU != 1000000UL) )
    return false;
#endif
} 

void UART_enableRxInterrupt(void)
{
    //system enable interrupt
    sei();
    
    //set RX Complete interrupt enable
    BIT_SET(UCSRB, RXCIE);
}

void UART_disableRxInterrupt(void)
{
    //set RX Complete interrupt enable
    BIT_CLEAR(UCSRB, RXCIE);
}

/************************************************************************/
/* UART USE FUNCTIONS                                                   */
/************************************************************************/
void uart_enable(eUartBaudRate baud_rate,
                 eUARTCharSize char_size,
                 eUARTStopBits stop_bits,
                 eUartParityMode parity)
{    
    int ii;
    //clear UCSRC so we have a known state to easily OR settings
    UCSRC = UCSRC_CLEAR_VAL; 
    
    //enable tx pin
    BIT_SET(UCSRB, TXEN);
        
    //enable rx pin
    BIT_SET(UCSRB, RXEN);
    
    uart_SetBaudRate(baud_rate);
    uart_SetCharWidth(char_size);
    uart_SetStopBits(stop_bits);
    uart_SetParity(parity);
    
    rcv_buff_len = 0;
    for (ii=0; ii < _UART_RX_BUFF_MAX_LEN; ii++)
    {
        rcv_buff[ii] = 0;
    }
}

void uart_disable(void)
{
    //disable tx pin
    BIT_CLEAR(UCSRB, TXEN);
    
    //disable rx pin
    BIT_CLEAR(UCSRB, RXEN);
}

bool _UART_TxIsBusy(void)
{
    //if the UDR (Uart Data Register) is Empty then TX is NOT busy
    //otherwise it is
    if (BIT_GET(UCSRA,UDRE))
    {
        return false;
    }
    else
    {
        return true;
    }
}

uint8_t UART_transmitString(char *str)
{
    uint8_t bytes_sent = 0;
    
    while (*str != 0)
    {
        //wait till previous write is done
        while (_UART_TxIsBusy() == true)
        {
            ;
        }
        
        //puts data in transmit register
        UDR  = *str;

        //advance pointer, increase counter
        str++;
        bytes_sent++;
    }
    
    UART_transmitNewLine();
    
    return bytes_sent;
}

uint8_t UART_transmitBytes(char *data, uint8_t len)
{
    uint8_t bytes_sent = 0;
    
    while (len > 0)
    {
        //wait till previous write is done
        while (_UART_TxIsBusy() == true)
        {
            ;
        }
        
        //puts data in transmit register
        UDR  = *data;

        //advance pointer, decrease counter
        data++;
        len--;
        bytes_sent++;
    }
    
    return bytes_sent;
}

void UART_TransmitByte(char data)
{
    UART_transmitBytes(&data, 1);
}

void UART_transmitNewLine(void)
{
    UART_transmitBytes("\r\n",2);
}

void UART_transmitUint8(uint8_t val)
{
    char str[STR_UINT8_LEN];
    
    convertUint8ToChar(val, str);
    UART_transmitBytes(str, STR_UINT8_LEN);
}

void UART_transmitUint16(uint16_t val)
{
    char str[STR_UINT16_LEN];
    
    convertUint16ToChar(val, str);
    UART_transmitBytes(str, STR_UINT16_LEN);
}

void UART_transmitInt8(int8_t val)
{
    char str[STR_INT8_LEN];
    
    convertInt8ToChar(val, str);
    UART_transmitBytes(str, STR_INT8_LEN);
}

void UART_transmitInt16(int16_t val)
{
    char str[STR_INT16_LEN];
    
    convertInt16ToChar(val, str);
    UART_transmitBytes(str, STR_INT16_LEN);
}

/** This private support function will tell whether or not the USRT RX data Register (UDR)
 * has pending data or not. 
 * @NOTE this doesn't read the data, it just tells its presence
 */
bool _UART_RxIsData(void)
{
    //if the Receive is Complete then we HAVE data
    //otherwise we dont
    if (BIT_GET(UCSRA,RXC))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void UART_ReceievBytes(char* ret_data,  uint8_t desired_len)
{
    uint8_t num_bytes_rcv;
    
    for (num_bytes_rcv=0; num_bytes_rcv < desired_len; num_bytes_rcv++)
    {
        //wait for data to arrive in Rx register
        while (_UART_RxIsData() == false)
        {
            ;
        }
        
        //read it to our buffer
        ret_data[num_bytes_rcv] = UDR;
    }
}

void UART_ReceiveByte(char* ret_data)
{
    UART_ReceievBytes(ret_data, 1);
}

/** This function will retrieve data from the uart rcv buffer. This will only have data
 * if interrupts are enabled AND data has been received. It will return the data and
 * length to the calling program
 * @PARAM ret_data[output] all data in the rcv buffer. This buffer should be pre-initialized
 *  to 0 and must be at least _UART_RX_BUFF_MAX_LEN bytes long
 * @PARAM ret_data_len [out] the number of bytes returned.
 */
void UART_ReadRxBuff(char* ret_data, uint8_t* ret_data_len)
{
    uint8_t ii;    
    
    //UART_disableRxInterrupt();
    *ret_data_len = rcv_buff_len;
    for (ii = 0; ii < rcv_buff_len; ii++)
    {
        //aia.test is this gonna be one off because the while condition??? shouldn't
        //copy data from buffer to return value
        ret_data[ii] = rcv_buff[ii];
        
        //clear rcv buffer
        rcv_buff[ii] = 0;
    }
    rcv_buff_len = 0;
     
    //UART_enableRxInterrupt();
}

/** Vector supported by ATmega16, ATmega32, ATmega323, ATmega8
 * This ISR will automatically read values from the UART Rx register and store them into a 
 * a buffer to be read at a later time.
 * @NOTE If the buffer is already full, all additional bytes will be lost.
 * At this point there is no fault handling in this code, but it could be added in the future
*/
ISR(USART_RXC_vect)
{
    //uint8_t rx_err_flags;
    uint8_t rx_dump_register;
    //must read error flags before reading data
    //for now, we aren't using this data, but could check for parity or overflow errors
    //rx_err_flags = UCSRA;
    
    //if we still have space in the buffer
    if (rcv_buff_len < _UART_RX_BUFF_MAX_LEN)
    {        
        rcv_buff[rcv_buff_len] = UDR;
        rcv_buff_len++;
    }
    else
    {
        //we have to read the value to clear all flags and stop the 
        //interrupt from triggering, even if we are just dumping the 
        //value
        rx_dump_register = UDR;
    }
}

void convertUint8ToChar(uint8_t in_val, char* output_3_chars)
{
    uint8_t ones;
    uint8_t tens;
    uint8_t hundreds;
    uint8_t remainder;
    
    remainder = in_val;
    
    //now we integer divide, to isolate our digit of interest   
    hundreds = remainder / 100;
    remainder -= (hundreds * 100);
    
    tens = remainder / 10;
    ones = remainder - (tens * 10);
    
    if (in_val >= 100)
    {
        output_3_chars[0] = (char)(ASCII_0 + hundreds);
    }
    else
    {
        output_3_chars[0] = ' ';
    }
    
    if (in_val >= 10)
    {
        output_3_chars[1] = (char)(ASCII_0 + tens);
    }
    else
    {
        output_3_chars[1] = ' ';
    }
    
    output_3_chars[2] = (char)(ASCII_0 + ones);
}

void convertUint16ToChar(uint16_t in_val, char* output_6_chars)
{
    uint8_t ones;
    uint8_t tens;
    uint8_t hundreds;
    uint16_t thousands;
    uint16_t ten_thousands;
    uint16_t remainder;
    
    remainder = in_val;
    
    //now we integer divide, to isolate our digit of interest
    ten_thousands = remainder / 10000;
    remainder -= (ten_thousands * 10000);
    
    thousands = remainder / 1000;
    remainder -= (thousands * 1000);
    
    hundreds = remainder / 100;
    remainder -= (hundreds * 100);
    
    tens = remainder / 10;
    ones = remainder - (tens * 10);
    
    if (in_val >= 10000)
    {
        output_6_chars[0] = (char)(ASCII_0 + ten_thousands);
    }
    else
    {
        output_6_chars[0] = ' ';
    }
    
    if (in_val >= 1000)
    {
        output_6_chars[1] = (char)(ASCII_0 + thousands);
        output_6_chars[2] = ',';
    }
    else
    {
        output_6_chars[1] = ' ';
        output_6_chars[2] = ' ';
    }
    
    if (in_val >= 100)
    {
        output_6_chars[3] = (char)(ASCII_0 + hundreds);
    }
    else
    {
        output_6_chars[3] = ' ';
    }
    
    if (in_val >= 10)
    {
        output_6_chars[4] = (char)(ASCII_0 + tens);
    }
    else
    {
        output_6_chars[4] = ' ';
    }
    
    output_6_chars[5] = (char)(ASCII_0 + ones);
}

void convertInt8ToChar(int8_t in_val, char* output_4_chars)
{
    uint8_t ones;
    uint8_t tens;
    uint8_t hundreds;
    uint8_t remainder;
    uint8_t is_neg;
    
    if (in_val < 0)
    {
        is_neg = 1;
        in_val = -1 * in_val;
    }
    else
    {
        is_neg = 0;
    }
    
    remainder = in_val;
    
    //now we integer divide, to isolate our digit of interest
    hundreds = remainder / 100;
    remainder -= (hundreds * 100);
    
    tens = remainder / 10;
    ones = remainder - (tens * 10);
    
    //format
    output_4_chars[3] = (char)(ASCII_0 + ones);
    
    if (in_val >= 10)
    {
        output_4_chars[2] = (char)(ASCII_0 + tens);
    }
    else
    {
        output_4_chars[2] = ' ';
    }
    
    
    if (in_val >= 100)
    {
        output_4_chars[1] = (char)(ASCII_0 + hundreds);
    }
    else
    {
        output_4_chars[1] = ' ';
    }
    
    if(is_neg)
    {
        output_4_chars[0] = '-';
    }
    else
    {
        output_4_chars[0] = ' ';
    }
}

void convertInt16ToChar(int16_t in_val, char* output_7_chars)
{
    uint8_t ones;
    uint8_t tens;
    uint8_t hundreds;
    uint16_t thousands;
    uint16_t ten_thousands;
    uint16_t remainder;
    uint8_t is_neg;
    
    if (in_val < 0)
    {
        is_neg = 1;
        in_val = -1 * in_val;
    }
    else
    {
        is_neg = 0;
    }
    
    remainder = in_val;
    
    //now we integer divide, to isolate our digit of interest
    ten_thousands = remainder / 10000;
    remainder -= (ten_thousands * 10000);
    
    thousands = remainder / 1000;
    remainder -= (thousands * 1000);
    
    hundreds = remainder / 100;
    remainder -= (hundreds * 100);
    
    tens = remainder / 10;
    ones = remainder - (tens * 10);
    
    //format
    output_7_chars[6] = (char)(ASCII_0 + ones);
    
    if (in_val >= 10)
    {
        output_7_chars[5] = (char)(ASCII_0 + tens);
    }
    else
    {
        output_7_chars[5] = ' ';
    }
    
    if (in_val >= 100)
    {
        output_7_chars[4] = (char)(ASCII_0 + hundreds);
    }
    else
    {
        output_7_chars[4] = ' ';
    }
    
    if (in_val >= 1000)
    {
        output_7_chars[2] = (char)(ASCII_0 + thousands);
        output_7_chars[3] = ',';
    }
    else
    {
        output_7_chars[2] = ' ';
        output_7_chars[3] = ' ';
    }
    
    if (in_val >= 10000)
    {
        output_7_chars[1] = (char)(ASCII_0 + ten_thousands);
    }
    else
    {
        output_7_chars[1] = ' ';
    }
    
    if(is_neg)
    {
        output_7_chars[0] = '-';
        is_neg = 0;
    }
    else
    {
        output_7_chars[0] = ' ';
    }
}