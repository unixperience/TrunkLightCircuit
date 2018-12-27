/*
 * avr_uart.h
 * This entire library only supports asynchronous mode.
 * Created: 12/21/2018 12:54:44 PM
 *  Author: Andrew
 */ 


#ifndef AVR_UART_H_
#define AVR_UART_H_

#include "global.h"
///This tells the maximum Rx buffer length for asyncronous Rx storage, Interrupts must be enabled to use this
#define _UART_RX_BUFF_MAX_LEN 16    
#define _UART_TX_BUFF_MAX_LEN 16    

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
void UART_enableRxInterrupt(void);
void UART_disableRxInterrupt(void);

//use
void uart_enable(eUartBaudRate baud_rate,
                 eUARTCharSize char_size,
                 eUARTStopBits stop_bits,
                 eUartParityMode parity);
void uart_disable(void);

/** This will transmit a C style (null-terminated) string. 
 * @PARAM str - the string to print to UART
 * @RETURN number of bytes written
 */
uint8_t UART_transmitString(char *str);
uint8_t UART_transmitBytes(char *data, uint8_t len);
void UART_TransmitByte(char data);
void UART_transmitNewLine(void);

/** reads multiple characters of Rx data. 
 * @NOTE This function will block till all "desired_len" data bytess are read
 * @PARAM ret_data [out] the bytse read from the UART Rx Data Register
 * @PARAM desired_len [IN] the number of bytes to read from the UART Rx channel
 */
void UART_ReceievBytes(char* ret_data,  uint8_t desired_len);

/** reads a single character for Rx data. 
 * @NOTE This function will block till a data bit is read
 * @PARAM ret_data [out] the byte read from the UART Rx Data Register
 */
void UART_ReceiveByte(char* ret_data);

/** This function will retrieve data from the uart rcv buffer. This will only have data
 * if interrupts are enabled AND data has been received. It will return the data and
 * length to the calling program. 
 * this is a non-blocking function and will return immediately whether or not data exists
 * in the buffer
 * @PARAM ret_data[output] all data in the rcv buffer. This buffer should be pre-initialized
 *  to 0 and must be at least _UART_RX_BUFF_MAX_LEN bytes long
 * @PARAM ret_data_len [out] the number of bytes returned.
 */
void UART_ReadRxBuff(char* ret_data, uint8_t* ret_data_len);

/**  This function will convert a uint8_t value into a 3 digit
 *  string. This string will always be 3 chars long.
 @PARAM in_val [input] the value to convert
 @PARAM output_3_chars [output] the value as a 3 digit char string
 @NOTE the calling code must declare the appropriately sized memory for the output
 */
void convertUint8ToChar(uint8_t in_val, char* output_3_chars);

/**  This function will convert a uint16_t value into a 6 digit
 *  string. This string will always be 6 chars long.
 @PARAM in_val [input] the value to convert
 @PARAM output_6_chars [output] the value as a 6 digit char string format xx,xxxx
 @NOTE the calling code must declare the appropriately sized memory for the output
 */
void convertUint16ToChar(uint16_t in_val, char* output_6_chars);

/**  This function will convert a uint8_t value into a 4 digit
 *  string. This string will always be 4 chars long.
 @PARAM in_val [input] the value to convert
 @PARAM output_4_chars [output] the value as a 4 digit char string
 the negative sign will always be output[0]
 @NOTE the calling code must declare the appropriately sized memory for the output
 */
void convertInt8ToChar(int8_t in_val, char* output_4_chars);

/**  This function will convert a uint16_t value into a 7 digit
 *  string. This string will always be 7 chars long.
 @PARAM in_val [input] the value to convert
 @PARAM output_7_chars [output] the value as a 7 digit char string format -xx,xxxx
  the negative sign will always be output[0]
 @NOTE the calling code must declare the appropriately sized memory for the output
 */
void convertInt16ToChar(int16_t in_val, char* output_6_chars);
#endif /* AVR_UART_H_ */
