/*
* 	clapLight.cpp
*
*	Created: 2018-04-08 오전 12:40:56
*	Author : Cakeng (PARK JONG SEOK)
*
*	NO LICENCE INCLUDED
*	Contact cakeng@naver.com to
*	use, modify, or share the software for any purpose.
*
*/
//Clap sequence - 40~60ms peaks - 160~320ms pause ~40~60ms peaks
#define F_CPU  1200000 // 1.2MHz

#define OUTPUT PB1 // OC0B
#define DEBUGPIN  PB2
#define DEBUGPIN2 PB4
#define INPUT0 PB0 // PCINT0, Mic input
#define INPUT1 PB3 // PCINT5

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>

uint16_t ticks = 0;
bool count;
bool outputOn = false;
bool input0Flag;
uint8_t outputCounter = 0;

void sleepRoutine()
{
	cli();
	PRR = (1<<PRTIM0)|(1<<PRTIM0)|(1<<PRADC);; // Power Down Timer 0, 1, USI, ADC
	WDTCR = 0; // Watchdog disabled
	MCUCR &= ~((1<<SM1)|(1<<SM0));
	sleep_enable(); // Sleep Enabled
	sei();
	sleep_cpu();
	//Sleeping... (Wake on Watch Input0 Pin change or TIM0 overflow)
	sleep_disable();// Sleep Disabled
	cli();
	PRR = 0; // Power Down Reset
	sei();
}

void turnTimer(bool in)
{
	if(in)
	{
		TCCR0B |= (1<<CS01);  //1/8 Clock Dividers. -> 586Hz Interrupt,
	}
	else
	{
		TCCR0B &= ~(1<<CS01);
	}
}
inline void setTimer()
{
	TCCR0A = (1<<WGM01)|(1<<WGM00); // Fast PWM Mode
	TCCR0B = 0; // Turn off Timer0 at startup.
	TIMSK0 |= (1<<TOIE0); // Timer0 Overflow Interrupt flag Set.
	sei(); // Set Interrupt.
}

void checkClap()
{
	ticks  = 0;
	count = true;
	turnTimer(true);
	bool result = true;
	while(ticks<35) //70ms
	{
		_delay_us(200);
	}
	while (ticks<125) //250ms
	{
		_delay_us(200);
		if (PINB&(1<<INPUT0))
		{
			result = false;
		}
	}
	if (result)
	{
		result = false;
		while(ticks<250) //500ms
		{
			_delay_us(200);
			if (PINB&(1<<INPUT0))
			{
				result = true;
			}
		}
	}
	if (result)
	{
		while(ticks<300) // 600ms
		{
			_delay_us(200);
			if (PINB&(1<<INPUT0))
			{
				result = false;
			}
		}
	}
	if (result)
	{
		while(ticks<800) // 1600ms
		{
			_delay_us(200);
		}
		if(outputOn)
		{
			outputOn = false;
		}
		else
		{
			outputOn = true;
		}
		turnTimer(outputOn);
	}
	else
	{
		turnTimer(false);
	}
	count = false;
}

inline void powerReduction()
{
	PRR = (1<<PRADC); // ADC Off.
	ADCSRB = (1<<ACME); // Analog Comparator Off.
}

inline void setPorts()
{
	DDRB = 0;
	DDRB |= (1<<DDB4);
	DDRB |= (1<<OUTPUT);
	DDRB |= (1<<DEBUGPIN);

	DDRB &= ~(1<<INPUT0);
	DDRB &= ~(1<<INPUT1);

	PORTB = 0;
	PORTB &= ~(1<<INPUT0);
	PORTB &= ~(1<<INPUT1);
}

inline void setMicInterrupt()
{
	GIMSK = (1<<PCIE);
	PCMSK= (1<<PCINT0);// Mic input Pin Change Interrupt Enabled.
	sei(); // Set Interrupt.
}

ISR(TIM0_OVF_vect)
{
	PORTB ^= (1<<DEBUGPIN2);
	if(count)
	{

		ticks++;
	}
}

ISR(PCINT0_vect)
{
	input0Flag = true;
}

int main(void)
{
	setPorts();
	powerReduction();
	setMicInterrupt();
	setTimer();
    /* Replace with your application code */
    while (1)
    {
		sleepRoutine();
		PORTB ^= (1<<DEBUGPIN);
		if(input0Flag)
		{

			PCMSK &= ~(1<<PCINT0);
			checkClap();
			input0Flag = false;
			PCMSK |= (1<<PCINT0);
		}
		if(outputOn)
		{
			PORTB |= (1<<OUTPUT);
			_delay_us(300);
			PORTB &= ~(1<<OUTPUT);
		}
		_delay_ms(1);
    }
}

