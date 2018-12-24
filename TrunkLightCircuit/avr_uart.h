/*
 * avr_uart.h
 * This entire library only supports asynchronous mode.
 * Created: 12/21/2018 12:54:44 PM
 *  Author: Andrew
 */ 


#ifndef AVR_UART_H_
#define AVR_UART_H_

#include "global.h"

typedef enum _eUARTCharSize
{
    charlen5,
    charlen6,
    charlen7,
    charlen8_default,
    charlen9,
}eUARTCharSize;

typedef enum _eUartParityMode
{
    disabled_default,
    enabled_even_parity,
    enabled_odd_parity,
}eUartParityMode;

typedef enum _eUARTStopBits
{
    one_default,
    two,
}eUARTStopBits;

/** The values for baud rate calculation are based on the clock speed F_CPU
    As of now this code only accommodates three clock speeds. These settings
    are all using double data speed, In the case of invalid speed  choice
    the defualt will be used
    F_CPU= 1000000UL // 1MHz
    F_CPU= 8000000UL // 8MHz
    F_CPU=10000000UL //16MHz
    
    Note that 1MHz speed only supports 9600, 19200 Buad_rates
*/
typedef enum _eUartBaudRate
{
    b9600_default,
    b19200,
    b115200,    /// this speed is only valid above 1.8432MHz
    b250000,    /// this speed is only valid 2MHz
    b1000000,  /// this speed is only valid above 8MHz
}eUartBaudRate;

//configuration
void uart_default(void);
void uart_SetCharWidth(eUARTCharSize value);
void uart_SetParity(eUartParityMode value);
void uart_SetStopBits(eUARTStopBits value);
bool uart_SetBaudRate(eUartBaudRate value);

//use
void uart_enable(eUartBaudRate baud_rate,
                 eUARTCharSize char_size,
                 eUARTStopBits stop_bits,
                 eUartParityMode parity);
void uart_disable(void);
#endif /* AVR_UART_H_ */
