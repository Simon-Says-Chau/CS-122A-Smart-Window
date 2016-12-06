//Micricontroller		:All 8-bit AVR Micricontrollers with Similar Register Configuration with ATmega16
//System Clock			:8 MHz
//Header File Version 		:1.1
//Author			:Arun Kumar Garg 
//				:ABLab Solutions
//				:www.ablab.in
//				:arun@ablab.in
//Date				:1st August 2011

/*Copyright (c) 2011 ABLab Solutions All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the 	following disclaimer in the documentation 
     and/or other materials provided with the distribution.
   * Neither the name of the copyright holders nor the names of contributors may be used to endorse or promote products derived from this software without 
     specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


/*The functions and macros defined in this header file are for USART of all 8-bit AVR Micricontrollers with Similar Register Configuration 
with ATmega 16*/

#ifndef	_USART_H_
#define	_USART_H_ 	1



#include<avr/io.h>
#include<util/delay.h>

#define F_CPU 8000000UL // Assume uC operates at 8MHz
#define BAUD_RATE 9600
#define BAUD_PRESCALE (((F_CPU / (BAUD_RATE * 16UL))) - 1)

/*The function is declared to initialize the USART with following cinfiguration:-
USART mode - Asynchronous
Baud rate - 9600
Data bits - 8
Stop bit - 1
Parity - No parity.*/

void usart_init();

/*The function is declared to transmit data.*/

void usart_data_transmit(unsigned char data );

/*The function is declared to receive data.*/

unsigned char usart_data_receive( void );

/*The function is declared to transmit string.*/

void usart_string_transmit(char *string);

/*The function is declared to receive string.*/

char *usart_string_receive(char *receive_string,unsigned char terminating_character);

/*Function definition*/

void usart_init()
{
UBRR0H = (BAUD_PRESCALE >> 8);
UBRR0L = BAUD_PRESCALE;
UCSR0B|= (1<<RXEN0)|(1<<TXEN0);
UCSR0C |= (1 << UCSZ00)|(3<<UCSZ01);
}


void usart_data_transmit(unsigned char data )
{
while ( !( UCSR0A & (1<<UDRE0)) )
;
UDR0 = data;
_delay_ms(1);
}

unsigned char usart_data_receive( void )
{
while ( !(UCSR0A & (1<<RXC0)) )
;
return UDR0;
}

void usart_string_transmit(char *string)
{
while(*string)
{
usart_data_transmit(*string++);
}
}

char *usart_string_receive(char *receive_string,unsigned char terminating_character)
{
	unsigned char temp=0x00;
	for(unsigned char i=0;;i++)
	{
		*(receive_string+i)=usart_data_receive();
		if(*(receive_string+i)==terminating_character)
			break;
		else
		temp++;
	}
	*(receive_string+temp)='\0';
	return receive_string;
}
#endif