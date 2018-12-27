/*
 * TrunkLightCircuit.c
 *
 * Created: 12/17/2018 8:57:07 PM
 * Author : Andrew
 */ 

#include <avr/io.h>
#include "main.h"
#include "global.h"
#include "avr_uart.h"

void init_uart_debug()
{
    //clear all register configuration
    uart_default();
    
    uart_enable(b1000000, charlen8_default, one_default, disabled_default);
    UART_enableRxInterrupt();
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
    char uart_data = 'f';
    char dat = '!';
    char ret_data[_UART_RX_BUFF_MAX_LEN] = {0};
    char int_str[7] = {0};
    uint8_t ret_len;
    //0-input
    //1-output
    DDRB = 0xFF;
    PORTB = 0xAA;
    uint8_t debug_cnt = 0;
    
    init_uart_debug();
    
    /* Replace with your application code */
    while (1) 
    {
        _delay_ms(2000);
        debug_cnt++;
        
        if (debug_cnt == 1)
        {
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
        }
        else if (debug_cnt == 2)
        {
            PORTB = 0xFF;//PORTB = UCSRA;
        }
        else if (debug_cnt == 3)
        {
            PORTB = 0xFF;//PORTB = UCSRB;
        }
        else if (debug_cnt == 4)
        {
            PORTB = 0xFF;//PORTB = UCSRC;
            PORTB = 0xFF;//PORTB = UCSRC;
            debug_cnt = 0;
        }
        
        //ECHO
        /*UART_ReceiveByte(&uart_data);
        UART_TransmitByte(uart_data);
        */
        UART_ReadRxBuff(ret_data, &ret_len);
        UART_transmitBytes("\r\nReceived data bytes:", 22);
        convertUint8ToChar(ret_len, int_str);
        UART_transmitBytes(int_str, 3);
        UART_transmitBytes("\r\n",2);
        UART_transmitBytes(ret_data, ret_len);
        /*uart_data = ret_data[0];
        
        if (uart_data == '0')
        {
            UART_transmitBytes("Got 0\r\n\0", 8);
        }
        else if (uart_data == '1')
        {
            UART_transmitBytes("Got 1\r\n\0", 8);
        }
        else if (uart_data == '2')
        {
            UART_transmitBytes("Got 2\r\n\0", 8);
        }
        else if (uart_data == '3')
        {
            UART_transmitBytes("Got 3\r\n\0", 8);
        }*/
        
    }
}

