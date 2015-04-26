/*
 *
 * WackaMole Game
 *
 * Ada Wong, Larry Sun
 * EC450 A1 Spring 2015
 *
 * WackaMole game using four "moles" and a play/reset button
 * Uses flash memory to store the highscore and LCD panel to
 * display the opening screen, current score, and highscore.
 *
 *
 */

#include "msp430g2553.h"
#include "LCD.h"

// define button and LED pins
#define 	TA0_BIT 		0x02				//P1.1 - Speaker
#define 	MOLE_LED1 		0x01				//P2.0
#define 	MOLE_LED2 		0x04				//P1.2
#define 	MOLE_LED3 		0x08				//P1.3
#define 	MOLE_LED4 		0x10				//P1.4
#define 	SR_BUTTON 		0x40				//P1.6 - on-board green LED
#define 	MOLE_BUTTON1 	0x80				//P1.7
#define 	MOLE_BUTTON2 	0x20				//P2.5
#define 	MOLE_BUTTON3 	0x10				//P2.4
#define 	MOLE_BUTTON4 	0x04				//P2.2

#define		MAX_MISSES		10					// maximum misses allowed before game over
#define		MULTIPLIER		5					// score multiplier for different stages

// define/initialize global variables
volatile unsigned soundOn = 0; 					// state of sound: 0 or OUTMOD_4 (0x0080)
volatile unsigned char soundTime = 0;			// Duration of sounds
volatile unsigned char tone_index = 0;			// Choose which tone to play
volatile unsigned int tones [] = {5000,7000};	// array of sound effects

volatile unsigned char state = 0;				//'p' for play mode, 'r' for reset/stopped
volatile unsigned char misses = 0; 				// how many times the player has missed a mole
volatile unsigned char hits = 0;				// how many times the player hits a mole
volatile unsigned char stage = 0; 				// current stage which will set the speed of the moles (higher stage means faster moles)

volatile unsigned int next_mole = 0;			// time before the next mole will be turned on
volatile unsigned int mole_counter = 0;			// counts down to determine how long the mole should be up
volatile unsigned int current_score = 0;		// player's score when playing

volatile unsigned int rand = 0;					// result from random_gen gets put into here
volatile unsigned int rand_on = 1;				// Actual mole that gets randomly chosen

volatile unsigned int mole1_on = 0;				// set to 1 if mole1 is up, 0 if off
volatile unsigned int mole2_on = 0;				// set to 1 if mole2 is up, 0 if off
volatile unsigned int mole3_on = 0;				// set to 1 if mole3 is up, 0 if off
volatile unsigned int mole4_on = 0;				// set to 1 if mole4 is up, 0 if off

volatile unsigned char last_mole1, last_mole2, last_mole3, last_mole4, last_play;	// for debouncing button presses
volatile unsigned int mole1_counter, mole2_counter, mole3_counter, mole4_counter;	// counter for how long the moles stay up

// LCD-changing state variables
volatile unsigned int game_over = 0;		// flag set when game is over
volatile unsigned int new_high_score = 0;	// flag set when new highscore is made
volatile unsigned int temp_score = 0;		// temporary storage for current score, used to display final score

////Initialize functions
void init_timer(void); 			// routine to setup the timer
void init_buttons(void);		//setup buttons and LEDs
unsigned random_gen(void);		//random generator

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++// Flash
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

//Information memory / flash memory variable
#pragma DATA_SECTION(high_score,".infoD");
volatile const int high_score=0;

void eraseD() // erase information memory segment D
{
	FCTL2 = FWKEY+FSSEL_2+3; 	// SMCLK source + divisor 24 (assuming 1Mhz clock)
	FCTL3 = FWKEY; 				// unlock flash (except for segment A)
	FCTL1 = FWKEY+ERASE; 		// setup to erase
	*( (int *) 0x1000) = 0; 	// dummy write to segment D word 0
	FCTL1=FWKEY; 				// clear erase and write modes
	FCTL3=FWKEY+LOCK; 			// relock the segment
}
void writeDword(int value, int *address)
{
	// write a single word.
	if ( (((unsigned int) address) >= 0x1000) &&
			(((unsigned int) address) <0x1040) ){// inside infoD?
		FCTL3 = FWKEY; 			// unlock flash (except for segment A)
		FCTL1 = FWKEY + WRT ; 	// enable simple write
		*address = value; 		// actual write to memory
		FCTL1 = FWKEY ; 		// clear write mode
		FCTL3 = FWKEY+LOCK; 	// relock the segment
	}
}

// Application level routine to update the three words stored in infoD
void updateData(int h)
{
	// writes h into the info memory
	eraseD(); // clear infoD
	writeDword(h,(int *) &high_score);	//write in updated high score
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++// Main
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

void main()
{
	WDTCTL = WDTPW + WDTHOLD; 	// Stop watchdog time
	BCSCTL1 = CALBC1_8MHZ; 		// 1Mhz calibration for clock
	DCOCTL = CALDCO_8MHZ;

	WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 0 + 1); // initialize watch dog timer

	// initializing global variables
	next_mole = 1000;
	mole_counter = 800;
	mole1_counter = mole_counter;
	mole2_counter = mole_counter;
	mole3_counter = mole_counter;
	mole4_counter = mole_counter;
	rand = 0;
	state = 'r';

	// initializing functions
	init_timer();
	init_buttons();
	ConfigureTimerUart();

	IE1 |= WDTIE;

	__enable_interrupt();
	__delay_cycles(20000000);

	opening_screen();	// display opening splash screen

	__delay_cycles(20000000);

	while(1)	// run when the screen needs to update the score or show opening screens
	{
		if(state == 'r'){
			__delay_cycles(5000000);
			if(game_over==1){ // game over screen
				game_over = 0;
				game_over_screen(temp_score);
				temp_score = 0;
				__delay_cycles(20000000);
				if(new_high_score==1){			// if new high score happens, display new high score screen
					new_high_score_screen();
					__delay_cycles(20000000);
					new_high_score = 0;			// clear flag
				}
				high_score_screen(high_score);	// current high score screen
				__delay_cycles(25000000);
			}
			reset_screen();						// display the start screen
		}
		else{
			__delay_cycles(10000);				// gives time for the LCD to be stable?
			print_current_score(current_score);
		}
		__bis_SR_register(LPM0_bits + GIE); 	// enable general interrupts and power down CPU
	}
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++// Init buttons and timer
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

void init_buttons()
{
	P1DIR |= MOLE_LED2+MOLE_LED3+MOLE_LED4;		//output LEDs
	P2DIR |= MOLE_LED1;
	P1OUT &= ~(MOLE_LED2+MOLE_LED3+MOLE_LED4);	//Turn off LEDs
	P2OUT &= ~(MOLE_LED1);

	P1OUT |= MOLE_BUTTON1+SR_BUTTON; 					// pullup
	P1REN |= MOLE_BUTTON1+SR_BUTTON; 					// enable resistor
	P2OUT |= MOLE_BUTTON2+MOLE_BUTTON3+MOLE_BUTTON4;	// pullup
	P2REN |= MOLE_BUTTON2+MOLE_BUTTON3+MOLE_BUTTON4;	// enable resistor
}

void init_timer(){ 			// initialization and start of timer
	TA0CTL |=TACLR; 		// reset clock
	TA0CTL |= TASSEL_2 + MC_2;
	TA0CCTL0=soundOn+CCIE; 	// compare mode, outmod=sound, interrupt CCR1 on
	TA0CCR0 = TAR+tones[tone_index];
	P1SEL|=TA0_BIT; 		// connect timer output to pin
	P1DIR|=TA0_BIT;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++// Random Generator
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

unsigned random_gen(void)
{
	unsigned int i, j;
	unsigned int result = 0;
	unsigned old_TA0CCTL0 = TA0CCTL0;

	// setup Timer_A
	BCSCTL3 = (~LFXT1S_3 & BCSCTL3) | LFXT1S_2; // Source ACLK from VLO
	TA0CCTL0 = CM_1 + CCIS_1 + CAP;

	for(i=0 ; i<16 ; i++)
	{
		result <<= 1; // shift left result
		char k = 0;

		for (j=0 ; j<5; j++){
			while(!(TA0CCTL0 & CCIFG)); // wait until Capture flag is set
			TA0CCTL0 &= ~CCIFG; 		// clear flag
			if(TA0CCR0 & 0x01) 			// check LSB
				k++;
		}
		if (k>=3)
			result |= 0x01;
	}

	TA0CCTL0 = old_TA0CCTL0;

	return result;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++// Sound handler
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


void interrupt sound_handler(){
	TA0CCR0 += tones[tone_index];
	if (soundOn && soundTime>=80){
		soundOn &= (~OUTMOD_4);
		soundTime = 0;
	}
	TA0CCTL0 = CCIE + soundOn; 		// update control register with current soundOn
}
ISR_VECTOR(sound_handler,".int09") 	// declare interrupt vector for Timer_A1


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++// WDT handler
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

interrupt void WDT_interval_handler()
{
	unsigned char mole1, mole2, mole3, mole4, play;

	mole1 = (P1IN & MOLE_BUTTON1);	// mole buttons states
	mole2 = (P2IN & MOLE_BUTTON2);
	mole3 = (P2IN & MOLE_BUTTON3);
	mole4 = (P2IN & MOLE_BUTTON4);
	play = (P1IN & SR_BUTTON);		// start/reset button

	/****************** Sound Control ******************/

	if(soundOn!=0)
		soundTime++;

	/***************** State Control *******************/

	if(state == 'p'){								// Play state
		if(--next_mole==0){
			rand = random_gen();					// Generates random number up to 2^16
			rand_on = (rand % 4) + 1;				// Chooses a mole based on random number
			switch(rand_on){						// Picks the next mole
				case 1:{
					if(!mole1_on)
						mole1_on = 1;
				}break;
				case 2:{
					if(!mole2_on)
						mole2_on = 1;
				} break;
				case 3:{
					if(!mole3_on)
						mole3_on = 1;
				} break;
				case 4:{
					if(!mole4_on)
						mole4_on = 1;
				} break;
			}
			next_mole = 1000 - (stage*100); // Time between moles decreases as game proceeds
			mole_counter = 800 - (stage*50); // Time interval for mole to be up
		}

		if(mole1_on){					// If mole 1 is "up"
			P2OUT |= MOLE_LED1;
			if(--mole1_counter==0){
				misses++;
				mole1_on = 0;
				mole1_counter = mole_counter;
				tone_index = 1;
				soundOn |= OUTMOD_4;
				soundTime = 0;
			}
		} else{
			P2OUT &= ~MOLE_LED1;
		}

		if(mole2_on){
			P1OUT |= MOLE_LED2;
			if(--mole2_counter==0){
				misses++;
				mole2_on = 0;
				mole2_counter = mole_counter;
				tone_index = 1;
				soundOn |= OUTMOD_4;
				soundTime = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED2;
		}

		if(mole3_on){
			P1OUT |= MOLE_LED3;
			if(--mole3_counter==0){
				misses++;
				mole3_on = 0;
				mole3_counter = mole_counter;
				tone_index = 1;
				soundOn |= OUTMOD_4;
				soundTime = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED3;
		}

		if(mole4_on){
			P1OUT |= MOLE_LED4;
			if(--mole4_counter==0){
				misses++;
				mole4_on = 0;
				mole4_counter = mole_counter;
				tone_index = 1;
				soundOn |= OUTMOD_4;
				soundTime = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED4;
		}

		if(hits==10){
			stage++;	// increases the stage every 10 hits
			hits = 0;
		}

		if(misses==MAX_MISSES){
			state = 'r';	// resets the game if the player misses 10 moles :(
			game_over = 1;
			temp_score = current_score;
			__bic_SR_register_on_exit(LPM0_bits);
		}
	}
	else if(state == 'r'){					// Reset State
		if(current_score>high_score){		// check and see if there's a new high score
			new_high_score = 1;
			updateData(current_score);
		}
		misses = 0;
		hits = 0;
		stage = 0;
		current_score=0;
		next_mole = 1000;
		mole_counter = 800;
		mole1_on = 0;
		mole2_on = 0;
		mole3_on = 0;
		mole4_on = 0;
		P1OUT &= ~(MOLE_LED2+MOLE_LED3+MOLE_LED4);	//Turn off LEDs
		P2OUT &= ~MOLE_LED1;
	}

	/***************** Button control ******************/

	if(last_play && (play==0)){
		if(state == 'r'){
			tone_index = 0;
			state = 'p';
		} else if(state == 'p'){
			state = 'r';
		}
		soundOn |= OUTMOD_4;
		soundTime = 0;
		__bic_SR_register_on_exit(LPM0_bits);
	}

	if(last_mole1 && (mole1==0)){
		if(mole1_on){
			hits++;
			current_score += (stage+1)*MULTIPLIER;
			mole1_on = 0;
			tone_index = 0;
			mole1_counter = mole_counter;
		} else{
			misses++;
			tone_index = 1;
		}
		soundOn |= OUTMOD_4;
		soundTime = 0;

		__bic_SR_register_on_exit(LPM0_bits);
	}

	if(last_mole2 && (mole2==0)){
		if(mole2_on){
			hits++;
			current_score += (stage+1)*MULTIPLIER;
			mole2_on = 0;
			tone_index = 0;
			mole2_counter = mole_counter;
		} else{
			misses++;
			tone_index = 1;
		}
		soundOn |= OUTMOD_4;
		soundTime = 0;
		__bic_SR_register_on_exit(LPM0_bits);
	}

	if(last_mole3 && (mole3==0)){
		if(mole3_on){
			hits++;
			current_score += (stage+1)*MULTIPLIER;
			mole3_on = 0;
			tone_index = 0;
			mole3_counter = mole_counter;
		} else{
			misses++;
			tone_index = 1;
		}
		soundOn |= OUTMOD_4;
		soundTime = 0;
		__bic_SR_register_on_exit(LPM0_bits);
	}

	if(last_mole4 && (mole4==0)){
		if(mole4_on){
			hits++;
			current_score += (stage+1)*MULTIPLIER;
			mole4_on = 0;
			tone_index = 0;
			mole4_counter = mole_counter;
		} else{
			misses++;
			tone_index = 1;
		}
		soundOn |= OUTMOD_4;
		soundTime = 0;
		__bic_SR_register_on_exit(LPM0_bits);
	}

	last_play = play;
	last_mole1 = mole1;
	last_mole2 = mole2;
	last_mole3 = mole3;
	last_mole4 = mole4;

}
ISR_VECTOR(WDT_interval_handler, ".int10")
