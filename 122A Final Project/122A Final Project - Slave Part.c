#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>
#include "bit.h"
#include "scheduler.h"
#include "keypad.h"
#include "lcd.h"
#include "usart_ATmega1284.h"

enum SPI_Tick {INIT, LOOP} SPI_state;
enum Stepper_Tick{Step_INIT, A, AB, B, BC, C, CD, D, DA} stepper_state;
enum SPI_Tick2 {INIT2} SPI_state2;
enum LCD_Tick {INIT3} LCD_state;
enum LED_TRANSMITTER {INIT4, TRANSMIT, DONE} TRANSMIT_state;
enum LED_RECEIVER {INIT5, RECEIVE2, DONE2} RECEIVE_state;

unsigned char receivedData;
unsigned char SPDR1;
unsigned char SPDR2;
unsigned char temp1;
unsigned char holder;
unsigned char movement;
int counter = 0;
int temp_val = 0;
int temp_val_2 = 0;
int flag = 0;
int flag2 = 0;
int check = 0;
int move_cc = 0;
int manual = 1;

// Master code
void SPI_MasterInit(void) {
	// Set DDRB to have MOSI, SCK, and SS as output and MISO as input
	//DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
	DDRB = 0xB0;
	// Set SPCR register to enable SPI, enable master, and use SCK frequency
	//   of fosc/16  (pg. 168)
	SPCR |= 0x51;
	//SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	// Make sure global interrupts are enabled on SREG register (pg. 9)
	SREG = SREG|0x80;
	//SREG = 0x80;
}

void SPI_MasterTransmit(unsigned char cData) {
	// data in SPDR will be transmitted, e.g. SPDR = cData;
	SPDR = cData;
	// set SS low
	while(!(SPSR & (1<<SPIF))) { // wait for transmission to complete
		;
	}
	// set SS high
}

// Servant code
void SPI_ServantInit(void) {
	// set DDRB to have MISO line as output and MOSI, SCK, and SS as input
	DDRB = 0x40;
	//DDR_SPI = (1<<DD_MISO);
	// set SPCR register to enable SPI and enable SPI interrupt (pg. 168)
	SPCR = SPCR|0xC0;
	//SPCR = (1<<SPE);
	// make sure global interrupts are enabled on SREG register (pg. 9)
	SREG = SREG|0x80;
}

ISR(SPI_STC_vect) {
	// this is enabled in with the SPCR register?s ?SPI
	// Interrupt Enable?
	// SPDR contains the received data, e.g. unsigned char receivedData =
	// SPDR;
	//return SPDR;
	receivedData = SPDR;
}

void A2D_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//      analog to digital conversions.
}

// Pins on PORTA are used as input for A2D conversion
//    The default channel is 0 (PA0)
// The value of pinNum determines the pin on PORTA
//    used for A2D conversion
// Valid values range between 0 and 7, where the value
//    represents the desired pin for A2D conversion
void Set_A2D_Pin(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x05) ? pinNum : ADMUX;
	// Allow channel to stabilize
	static unsigned char i = 0;
	for ( i=0; i<15; i++ ) { asm("nop"); }
}

void SPI_Init(){
	SPI_state = INIT;
}

void SPI_Tick(){
	int IR_val = ADC;
	switch(SPI_state){
		case INIT:
		if(USART_HasReceived(0)){
			temp1 = USART_Receive(0);
			if(temp1 == 'M' || temp1 == 'm'){
				manual = 1;
				SPI_MasterTransmit('0');
			}
			
			if(temp1 == 'A' || temp1 == 'a'){
				manual = 0;
			}
			USART_Flush(0);
		}
		
		else if(IR_val > 25 && manual == 0){
			SPI_MasterTransmit('2');
		}
		
		else if(IR_val < 25 && manual == 0){
			SPI_MasterTransmit('1');
		}
		break;
		
		case LOOP:
		if(USART_HasReceived(0)){
			temp1 = USART_Receive(0);
			if(temp1 == 'M' || temp1 == 'm'){
				manual = 1;
				SPI_MasterTransmit('0');
			}
			
			if(temp1 == 'A' || temp1 == 'a'){
				manual = 0;
			}
			USART_Flush(0);
		}
		
		else if(IR_val > 25 && manual == 0){
			SPI_MasterTransmit('2');
		}
		
		else if(IR_val < 25 && manual == 0){
			SPI_MasterTransmit('1');
		}
		break;
		
		default:{
			break;
		}
	}
	
	switch(SPI_state){
		case INIT:
		SPI_state = LOOP;
		break;
		
		case LOOP:
		SPI_state = INIT;
		break;
		
		default:{
			SPI_state = INIT;
			break;
		}
	}
	
}

void SPI_Init2(){
	SPI_state2 = INIT2;
}

void SPI_Tick2(){
	switch(SPI_state2){
		case INIT2:
		movement = receivedData;
		break;
		
		default:{
			break;
		}
	}
	
	switch(SPI_state2){
		case INIT2:
		SPI_state2 = INIT2;
		break;
		
		default:{
			SPI_state2 = INIT2;
			break;
		}
	}
	
}

void LCD_begin(){
	LCD_state = INIT3;
}

void LCD_Tick(){
	int input = ADC;
	switch(LCD_state){
		case INIT3:
		if(GetKeypadKey() == '*'){
			LCD_Number_Write(input,10);
		}
		
		if(GetKeypadKey() == 'D' && flag2 == 0){
			LCD_Cursor(31);
			LCD_WriteData(1 + '0');
			LCD_Cursor(16);
			flag = 1;
			// 			SPDR1 = 0x80;
			// 			SPDR = (SPDR1 | SPDR2);
			// 			SPI_MasterTransmit(SPDR);
		}
		
		if(GetKeypadKey() == '#' && flag2 == 0){
			LCD_Cursor(31);
			LCD_WriteData(2 + '0');
			LCD_Cursor(16);
			flag = 2;
			// 			SPDR1 = 0x40;
			// 			SPDR = (SPDR1 | SPDR2);
			// 			SPI_MasterTransmit(SPDR);
		}
		
		if(GetKeypadKey() == '1' && flag == 1)
		{
			flag2 = 1;
			if(movement == '2'){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WINDOW 1: OPEN");
			}
			else if(movement == '1'){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WINDOW 1: CLOSE");
			}
			LCD_Cursor(16);
		}
		
		if(GetKeypadKey() == '1' && flag == 2)
		{
			flag2 = 1;
			if(movement == '2'){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WINDOW 2: OPEN");
			}
			else if(movement == '1'){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WINDOW 2: CLOSE");
			}
			LCD_Cursor(16);
		}
		
		if(GetKeypadKey() == '2' && flag2 == 1)
		{
			flag2 = 0;
			if(flag == 1){
				LCD_DisplayString(1, "      MENU       STATUS: ROOM 1");
				LCD_Cursor(16);
			}
			
			else if(flag == 2){
				LCD_DisplayString(1, "      MENU       STATUS: ROOM 2");
				LCD_Cursor(16);
			}
		}
		
		break;
		
		default:{
			// 			SPDR1 = 0x80;
			// 			SPDR2 = 0x08;
			// 			SPDR = (SPDR1 | SPDR2);
			// 			SPI_MasterTransmit(SPDR);
			break;
		}
	}
	
	switch(LCD_state){
		case INIT3:
		LCD_state = INIT3;
		break;
		
		default:{
			LCD_state = INIT3;
			break;
		}
	}
	
}

void RECEIVE_Init(){
	RECEIVE_state = INIT5;
}

void RECEIVE_Tick(){
	//Actions
	switch(RECEIVE_state){
		case INIT5:
		if(movement == '1' || movement == '2'){
			manual = 0;
			if(movement == '1'){
				if((move_cc != 1 && temp_val >= 0)){
					temp_val = 0 + temp_val_2;
				}
				move_cc = 1;
				check = 1;
			}
			else if(movement = '2'){
				if((move_cc != 2 && temp_val <= 720)){
					temp_val = 0 + temp_val_2;
				}
				move_cc = 2;
				check = 2;
			}
			else{
				move_cc = 0;
				check = 0;
			}
		}
		
		else{
			manual = 1;
			temp_val = 0 + temp_val_2;
		}
		break;
		
		case RECEIVE2:
		if(movement == '1' || movement == '2'){
			manual = 0;
			if(movement == '1'){
				if((move_cc != 1 && temp_val >= 0)){
					temp_val = 0 + temp_val_2;
				}
				move_cc = 1;
				check = 1;
			}
			else if(movement = '2'){
				if((move_cc != 2 && temp_val <= 720)){
					temp_val = 0 + temp_val_2;
				}
				move_cc = 2;
				check = 2;
			}
			else{
				move_cc = 0;
				check = 0;
			}
		}
		
		else{
			manual = 1;
			temp_val = 0 + temp_val_2;
		}
		break;
		
		default:
		break;
	}
	
	//Transitions
	switch(RECEIVE_state){
		case INIT5:
		RECEIVE_state = RECEIVE2;
		break;

		case RECEIVE2:
		RECEIVE_state = INIT5;
		break;
		
		default:
		RECEIVE_state = INIT5;
		break;
	}
}

void Stepper_Init(void) {
	stepper_state = Step_INIT;
	
}

void Stepper_Tick(){
	switch(stepper_state){
		case Step_INIT:
		PORTA = 0x00;
		break;
		
		case A:
		PORTA = 0x01;
		break;
		
		case AB:
		PORTA = 0x03;
		break;
		
		case B:
		PORTA = 0x02;
		break;
		
		case BC:
		PORTA = 0x06;
		break;
		
		case C:
		PORTA = 0x04;
		break;
		
		case CD:
		PORTA = 0x0C;
		break;
		
		case D:
		PORTA = 0x08;
		break;
		
		case DA:
		PORTA = 0x09;
		break;
		
		default:
		break;
	}
	
	switch(stepper_state){
		case Step_INIT:
		if(((check == 1 || check == 2) && manual == 0) || (check == 0 && manual == 1)){
			counter = (temp_val / 5.625) * 64;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = A;
		}
		
		else{
			stepper_state = Step_INIT;
		}
		break;
		
		case A:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = AB;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = DA;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = AB;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = DA;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = AB;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = DA;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case AB:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = B;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = A;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = B;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = A;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = B;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = A;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case B:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = BC;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = AB;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = BC;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = AB;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = BC;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = AB;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case BC:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = C;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = B;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = C;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = B;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = C;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = B;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case C:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = CD;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = BC;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = CD;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = BC;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = CD;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = BC;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case CD:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = D;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = C;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = D;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = C;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = D;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = C;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case D:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = DA;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = CD;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = DA;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = CD;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = DA;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = CD;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		case DA:
		if(move_cc == 1 && counter >= 0 && manual == 0){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = A;
		}
		
		else if(move_cc == 2 && counter <= 8192 && manual == 0){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = D;
		}
		
		else if(GetKeypadKey() == 'B' && counter >= 0 && manual == 1){
			counter--;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = A;
		}
		
		else if(GetKeypadKey() == 'C' && counter <= 8192 && manual == 1){
			counter++;
			temp_val_2 = (counter / 64) * 5.625;
			stepper_state = D;
		}
		
// 		else if(GetKeypadKey() == '6' && counter >= 0 && manual == 1){
// 			counter--;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = A;
// 		}
// 		
// 		else if(GetKeypadKey() == '9' && counter <= 8192 && manual == 1){
// 			counter++;
// 			temp_val_2 = (counter / 64) * 5.625;
// 			stepper_state = D;
// 		}
		
		else{
			check = 0;
			stepper_state = Step_INIT;
		}
		break;
		
		
		default:
		stepper_state = Step_INIT;
		break;
	}
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xF0; PORTC = 0xFF;      //LCD and Keypad part
	DDRD = 0xFF; PORTD = 0x00;
	//DDRD = 0x0F; PORTD = 0xF0;	//Bluetooth Communication
	

// 	tasksNum = 1;
// 	task tsks[1];
// 	tasks = tsks;

	tasksNum = 4;
	task tsks[4];
	tasks = tsks;
	
	//Master Part
// 	unsigned char i = 0;
// 	tasks[i].state = -1;
// 	tasks[i].period = 30;
// 	tasks[i].elapsedTime = tasks[i].period;
// 	tasks[i].TickFct = &SPI_Tick;
	// 	i++;
	//Slave Part
	unsigned char i = 0;
	tasks[i].state = -1;
	tasks[i].period = 30;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SPI_Tick2;
	i++;
	tasks[i].state = -1;
	tasks[i].period = 150;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LCD_Tick;
	i++;
	tasks[i].state = -1;
	tasks[i].period = 3;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Stepper_Tick;
	i++;
	tasks[i].state = -1;
	tasks[i].period = 30;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &RECEIVE_Tick;
	
	//Master
// 	initUSART(0);
// 	SPI_Init();
// 	SPI_MasterInit();
// 	A2D_init();
	
	//Slave
	SPI_ServantInit();
	Stepper_Init();
	SPI_Init2();
	LCD_begin();
	//Initializing the LCD at the beginning
	LCD_init();
	LCD_ClearScreen();
	LCD_DisplayString(1, "      MENU       STATUS: ROOM 1");
	LCD_Cursor(16);
	RECEIVE_Init();
	
	TimerSet(3);
	TimerOn();
	
	while(1)
	{
	}
}