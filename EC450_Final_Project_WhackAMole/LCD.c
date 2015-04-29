/*
 * UART Simulation for 16x2 LCD panel with serial LCD backpack
 *
 * Code based on Matt Wilson's Sparkfun Serial LCD back pack example
 * http://mattwilson.org/msp430/
 *
 *
 *
 */

#include "msp430g2553.h"

#define     TXD            0x08     // TXD on P2.3

//   Conditions for 9600 Baud SW UART, SMCLK = 1MHz
#define     Bitime         833       // 8,000,000 / 9600 = ~833

// variables for serial communication
unsigned char BitCount;
unsigned int  TXByte;

// base-10 itoa for positive numbers. Populates str with a null-terminated string.
// limit lets you overflow back to 1 when you hit the limit+1, 2*limit+1, ...
// make sure *str is an array that can hold the number of digits in your limit + 1.
void itoa(unsigned int val, char *str, unsigned int limit)
{
	int temploc = 0;
	int digit = 0;
	int strloc = 0;
	char tempstr[5]; //16-bit number can be at most 5 ASCII digits;

	if(val>limit)
		val %= limit;

	do
	{
		digit = val % 10;
		tempstr[temploc++] = digit + '0';
		val /= 10;
	} while (val > 0);

	// reverse the digits back into the output string
	while(temploc>0)
		str[strloc++] = tempstr[--temploc];

	str[strloc]=0;
}

// Output the opening page
void opening_screen()
{
	TXByte = 0xFE; Transmit();
	TXByte = 0x01; Transmit();

	TXString(" WHACK-A-MOLE!!    Loading...");
}

//Waiting for player to press Play Button
void reset_screen()
{
	TXByte = 0xFE; Transmit();
	TXByte = 0x01; Transmit();

	TXString("PRESS PLAY TO   START");
}

// displays text when game over
void game_over_screen(unsigned int current_score)
{
	TXByte = 0xFE; Transmit();
	TXByte = 0x01; Transmit();

	TXString("  GAME OVER     YOUR SCORE:");
	char times[5];
	itoa(current_score, times, 9999);
	TXString(times);
}

// display highscore after the game over screen
void high_score_screen(unsigned int highscore)
{
	TXByte = 0xFE; Transmit();
	TXByte = 0x01; Transmit();

	TXString("HIGHSCORE:      ");
	char times[5];
	itoa(highscore, times, 9999);
	TXString(times);
}

void new_high_score_screen()
{
	TXByte = 0xFE; Transmit();
	TXByte = 0x01; Transmit();

	TXString("NEW HIGHSCORE!!!");
}

// Output the number of button presses to LCD
void print_current_score(unsigned int hits)
{
	// Clear LCD
	TXByte = 0xFE; Transmit();
	TXByte = 0x01; Transmit();

	TXString("CURRENT SCORE:  ");
	char times[5];
	itoa(hits, times, 9999);
	TXString(times);
}

// Transmit a string via serial UART by looping through it and transmitting
// each character one at a time.
void TXString(char *string)
{
	while(*string != 0)
	{
		TXByte = *string;
		Transmit();
		string++;
	}
}

// Set up timer for simulated UART
void ConfigureTimerUart(void)
{
	TA1CCTL0 = OUT;                             // TXD Idle as Mark
	TA1CTL = TASSEL_2 + MC_2 + ID_0;            // SMCLK/8, continuous mode
	P2SEL |= TXD;
	P2DIR |= TXD;
}

void Transmit()
{
	BitCount = 0xA;                             // Load Bit counter, 8data + ST/SP
	TA1CCR0 += Bitime;                     		// Some time till first bit
	TXByte |= 0x100;                        	// Add mark stop bit to TXByte
	TXByte = TXByte << 1;                 		// Add space start bit
	TA1CCTL0 =  CCIS0 + OUTMOD0 + CCIE;         // TXD = mark = idle
	while ( TA1CCTL0 & CCIE );                  // Wait for TX completion
}

// Timer A1 interrupt handler
interrupt void Timer_A()
{
	TA1CCR0 += Bitime;                          // Add Offset to CCR0
	if (TA1CCTL0 & CCIS0)                       // TX on CCI0B?
	{
		if ( BitCount == 0)
			TA1CCTL0 &= ~ CCIE;                 // All bits TXed, disable interrupt
		else
		{
			TA1CCTL0 |=  OUTMOD2;               // TX Space
			if (TXByte & 0x01)
				TA1CCTL0 &= ~ OUTMOD2;          // TX Mark
			TXByte = TXByte >> 1;
			BitCount --;
		}
	}
}
ISR_VECTOR(Timer_A,".int13");
