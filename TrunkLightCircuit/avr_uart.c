/*
 * avr_uart.c
 *
 * Created: 12/21/2018 12:54:25 PM
 *  Author: Andrew
 */ 
#include "avr_uart.h"
#include <util/atomic.h>

#define UCSRC_CLEAR_VAL 0x80  /// This clears register UCSRC, all writes to the reg must have
                              /// URSEL bit (MSB 7) set since this register is shared with UBRRH

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
 //////////////////////////////////////////////////////////////////////////
 //atmel sample code begin
void USART_Init( unsigned int ubrr)
{
    /* Set baud rate */
    UBRRH = (unsigned char)(ubrr>>8);
    UBRRL = (unsigned char)ubrr;
    /* Enable receiver and transmitter */
    UCSRB = (1<<RXEN)|(1<<TXEN);
    /* Set frame format: 8data, 2stop bit */
    UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
}    

void USART_Transmit( unsigned char data )
{
    /* Wait for empty transmit buffer */
    while ( !( UCSRA & (1<<UDRE)) )
    ;
    /* Put data into buffer, sends the data */
    UDR = data;
}
//atmel sample code end
//////////////////////////////////////////////////////////////////////////

//Baud rate calculation
// UBRR = [Fosc / (16*BAUD)] -1
// 

//TXC must be cleared before each transmission (before udr is written)
//RXC checks for existing Rx data in the receive buffer
//when you write to UCSRC you must set URSEL bit (MSB) since i/o location is shared by
//  UBRRH and UCSRC
// anytime you write to UCSRA register you must set UDRE bit low


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

void uart_SetStopBit1Not2(eUARTStopBits value)
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

bool uart_SetBaudRate(eUartBaudRate value)
{
    BIT_SET(UCSRA, U2X);    //enables double data rate
    
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
   
void uart_enable(eUartBaudRate baud_rate,
                 eUARTCharSize char_size,
                 eUARTStopBits stop_bits,
                 eUartParityMode parity)
{    
    //clear UCSRC so we have a known state to easily OR settings
    UCSRC = UCSRC_CLEAR_VAL; 
    
    //we are using asynchronous mode, so UMSEL==0, 1==synchronous
    //all writes to UCSRC must have URSEL set
    UCSRC |= (1<<URSEL)     //write enable
          || (1<<UMSEL);    //async mode
          
    uart_SetBaudRate(baud_rate);
    uart_SetCharWidth(char_size);
    uart_SetStopBits(stop_bits);
    uart_SetParity(parity);
    
    //enable tx pin
    BIT_SET(UCSRB, TXEN);
        
    //enable rx pin
    BIT_SET(UCSRB, RXEN);
}

void uart_disable(void)
{
    //disable tx pin
    BIT_CLEAR(UCSRB, TXEN);
    
    //disable rx pin
    BIT_CLEAR(UCSRB, RXEN);
}
