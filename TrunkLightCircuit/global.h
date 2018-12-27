/*
 * global.h
 *
 * Created: 12/17/2018 9:05:40 PM
 *  Author: Andrew
 */ 


#ifndef GLOBAL_H_
#define GLOBAL_H_

#define F_CPU  8000000UL
#include <util/delay.h>
#include <avr/io.h> //this should work but for some reason its not 
//  it should automatically see I chose atmega8A and use the iom8a.h 
//but it isn't
//#include <avr/iom8a.h
#include <avr/interrupt.h>

//bit macros
#define BIT_GET(p,x) ((p) & (1 << x))
#define BIT_SET(p,x) ((p) |= ( 1 << x))
#define BIT_CLEAR(p,x) ((p) &= ~(1 << x))
#define BIT_FLIP(p,x) ((p) ^= (1 << x))
#define BIT_WRITE(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))


#define BRAKE_LOW_DUTY_CYCLE     10
#define BRAKE_100_DUTY_CYCLE     99
#define BRAKE_OFF_DUTY_CYCLE      1
#define PWM_8BIT_50_DUTY_CYC    128

typedef uint8_t bool;
#define true 1
#define false 0

/// This code is compatible with two lighting configurations
/// 1. separate   3 light outputs: left, brake, right
/// 2. integrated 2 light outputs: left/brake, right/brake
/// TRUE - we are in case 2 (integrated lights)
/// FALSE= we are in case 1 
bool gbINTEGRATED_TURN_AND_BRAKE;

#endif /* GLOBAL_H_ */