/*
 * Project: Si4463 Radio Library for AVR and Arduino (Channel scanner example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Channel scanner
 *
 * Listen to each channel, record the highest RSSI value and print a pretty graph
 */

#define BAUD 1000000

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <stdio.h>
#include "Si446x.h"

static volatile uint8_t channel;

static int put(char c, FILE* stream)
{
	if(c == '\n')
		put('\r', stream);
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}

static FILE uart_io = FDEV_SETUP_STREAM(put, NULL, _FDEV_SETUP_WRITE);

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{
	(void)(length); // Stop warnings about unused parameters
	(void)(rssi);

	Si446x_RX(channel);
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
	(void)(rssi);

	Si446x_RX(channel);
}

void main(void)
{
	clock_prescale_set(clock_div_1);

	// UART
	//PORTD |= _BV(PORTD0);
	//DDRD |= _BV(DDD1);
	UBRR0 = UBRR_VALUE;
#if USE_2X
	UCSR0A = _BV(U2X0);
#endif
	UCSR0B = _BV(TXEN0);

	// LED indicator
	DDRC |= _BV(DDC5);
	PORTC |= _BV(PORTC5);

	stdout = &uart_io;

	// Start up
	Si446x_init();

	// Interrupts on
	sei();

	while(1)
	{
		// Receive mode on selected channel
		Si446x_RX(channel);
		
		// Check the RSSI value and store the highest, do this for about 2 seconds
		int16_t peakRssi = -999;
		for(uint16_t i=0;i<1800;i++)
		{
			_delay_ms(1);
			int16_t rssi = Si446x_getRSSI();
			if(rssi > peakRssi)
				peakRssi = rssi;
		}

		// Print out a pretty graph
		uint16_t bars = (130 - (peakRssi * -1)) / 4;

		printf_P(PSTR("%03hhu: "), channel);
		for(uint16_t i=0;i<bars;i++)
			printf_P(PSTR("|"));
		printf_P(PSTR(" (%d)\n"), peakRssi);

		channel++;
	}
}
