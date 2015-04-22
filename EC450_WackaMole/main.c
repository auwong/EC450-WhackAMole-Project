#include "msp430g2553.h"

#define TA0_BIT 0x01			//P1.0 - on-board red LED
#define MOLE_LED1 0x02			//P1.1
#define MOLE_LED2 0x04			//P1.2
#define MOLE_LED3 0x08			//P1.3
#define MOLE_LED4 0x10			//P1.4
#define SR_BUTTON 0x40			//P1.6 - on-board green LED
#define MOLE_BUTTON1 0x80		//P1.7
#define MOLE_BUTTON2 0x20		//P2.5
#define MOLE_BUTTON3 0x10		//P2.4
#define MOLE_BUTTON4 0x08		//P2.3


////Variables
volatile unsigned soundOn=0; // state of sound: 0 or OUTMOD_4 (0x0080)
volatile unsigned char state=0;
volatile unsigned int misses=0; // how many times the player has missed a mole
volatile unsigned int hits = 0;	// how many times the player hits a mole
volatile unsigned int stage=0;  // current stage which will set the speed of the moles (higher stage means faster moles)
volatile unsigned int next_mole=0;	// time before the next mole will be turned on
volatile unsigned int mole_counter=0;	// counts down to determine how long the mole should be up
volatile unsigned int mole_interval=0;	// sets how long the mole stays up

volatile unsigned int mole1_count=0;
volatile unsigned int mole2_count=0;
volatile unsigned int mole3_count=0;
volatile unsigned int mole4_count=0;

volatile unsigned int mole1_on=0;	// set to 1 if mole1 is up, 0 if off
volatile unsigned int mole2_on=0;	// set to 1 if mole2 is up, 0 if off
volatile unsigned int mole3_on=0;	// set to 1 if mole3 is up, 0 if off
volatile unsigned int mole4_on=0;	// set to 1 if mole4 is up, 0 if off
volatile unsigned int rand=0;		// not sure what to do with this yet...
volatile unsigned int rand_on=1;

volatile unsigned char last_mole1;
volatile unsigned char last_mole2;
volatile unsigned char last_mole3;
volatile unsigned char last_mole4;
volatile unsigned char last_play;


////Initialize functions
void init_timer(void); // routine to setup the timer
void init_buttons(void);	//setup buttons and LEDs

///////////////////////////////////////////////////////////// Main

void main() {
    WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 0 + 1); // initialize watch dog timer

    IE1 |= WDTIE;


    BCSCTL1 = CALBC1_1MHZ; // 1Mhz calibration for clock
	DCOCTL = CALDCO_1MHZ;

	mole_interval = 100;
	next_mole = 100;
	rand = 0;
	state = 'r';
	init_timer();
	init_buttons();
	
	_bis_SR_register(GIE+LPM0_bits);// enable general interrupts and power down CPU
 }

/////////////////////////////////////// Init buttons and timer

void init_buttons(){
	P1DIR |= MOLE_LED1+MOLE_LED2+MOLE_LED3+MOLE_LED4;		//output LEDs
	P1OUT &= ~(MOLE_LED1+MOLE_LED2+MOLE_LED3+MOLE_LED4);	//Turn off LEDs
	P1OUT |= MOLE_BUTTON1+SR_BUTTON; // pullup
	P1REN |= MOLE_BUTTON1+SR_BUTTON; // enable resistor
	P2OUT |= MOLE_BUTTON2+MOLE_BUTTON3+MOLE_BUTTON4;
	P2REN |= MOLE_BUTTON2+MOLE_BUTTON3+MOLE_BUTTON4;
}

void init_timer(){ 			// initialization and start of timer
	TA0CTL |= TACLR;             // reset clock
	TA0CTL = TASSEL1+ID_0+MC_1;  // clock source = SMCLK
	                             // clock divider=1
	                             // UP mode
	                             // timer A interrupt off
	TA0CCTL0=soundOn+CCIE;  // compare mode, output 0, no interrupt enabled
	TA0CCR0 = TAR; 			// in up mode TAR=0... TACCRO-1
	P1SEL|=TA0_BIT; 		// connect timer output to pin
	P1DIR|=TA0_BIT;
}

/////////////////////////////////////////// Interrupts

void interrupt sound_handler(){
	TACCR0 += 2000; // advance 'alarm' time
	TA0CCTL0 = CCIE + soundOn; //  update control register with current soundOn
}
ISR_VECTOR(sound_handler,".int09") // declare interrupt vector

interrupt void WDT_interval_handler(){
	unsigned char mole1;
	unsigned char mole2;
	unsigned char mole3;
	unsigned char mole4;
	unsigned char play;
	//unsigned char reset;
	
	mole1 = (P1IN & MOLE_BUTTON1);
	mole2 = (P2IN & MOLE_BUTTON2);
	mole3 = (P2IN & MOLE_BUTTON3);
	mole4 = (P2IN & MOLE_BUTTON4);
	play = (P1IN & SR_BUTTON);		// start/reset button
	//reset = (P1IN & rest_button); //need reset button
	
	if(state == 'p'){
		// play the thing here
		if(--next_mole==0){
			mole_interval = 100 - (stage*20);
			//int rand_on = (rand() % 4) + 1;
			rand_on = rand_on + 1;
			if (rand_on > 4) rand_on = 1;
			switch(rand_on){
				case 1:{
					mole1_on = 1;
					mole1_count = mole_interval;
					} break;
				case 2:{
					mole2_on = 1;
					mole2_count = mole_interval;
					} break;
				case 3:{
					mole3_on = 1;
					mole3_count = mole_interval;
					} break;
				case 4:{
					mole4_on = 1;
					mole4_count = mole_interval;
					} break;
			}
			next_mole = mole_interval;
		}
		
		if(mole1_on==1){
			P1OUT |= MOLE_LED1;
			if(--mole1_count==0){
				//do stuff
				mole1_on = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED1;
			mole1_count = 0;
		}
		
		if(mole2_on==1){
			P1OUT |= MOLE_LED2;
			if(--mole2_count==0){
				// do stuff
				mole2_on = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED2;
			mole2_count = 0;
		}
		
		if(mole3_on==1){
			P1OUT |= MOLE_LED3;
			if(--mole3_count==0){
				// do stuff
				mole3_on = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED3;
			mole3_count = 0;
		}
		
		if(mole4_on==1){
			P1OUT |= MOLE_LED4;
			if(--mole4_count==0){
				// do stuff
				mole4_on = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED4;
			mole4_count = 0;
		}
		
		if(hits==10){
			stage++;	// increases the stage every 10 hits
			hits = 0;
		}
		
		if(misses==10){
			state = 'r';	// resets the game if the player misses 10 moles :(
		}
	}
	else if(state == 'r'){
		// stays paused until play is pressed, dunno if anything need to happen in here
	}
	
	if(last_play && (play==0)){
		if(state == 'r'){
			state = 'p';
		} else if(state == 'p'){
			// do some more resetting stuff in here
			misses = 0;
			hits = 0;
			stage = 0;
			mole_interval = 300;
			mole_counter = 0;
			mole1_on = 0;
			mole2_on = 0;
			mole3_on = 0;
			mole4_on = 0;
			state = 'r';
		}
	}
	last_play = play;
	
	/*if(last_reset && (reset==0)){

	}
	last_reset = reset;*/
	
	if(last_mole1 && (mole1==0)){
		P1OUT ^= MOLE_LED1;			//temp test
	}
	last_mole1 = mole1;
	
	if(last_mole2 && (mole2==0)){
		P1OUT ^= MOLE_LED2;			//temp test
	}
	last_mole2 = mole2;
	
	if(last_mole3 && (mole3==0)){
		P1OUT ^= MOLE_LED3;			//temp test
	}
	last_mole3 = mole3;
	
	if(last_mole4 && (mole4==0)){
		P1OUT ^= MOLE_LED4;			//temp test
	}
	last_mole4 = mole4;
}
ISR_VECTOR(WDT_interval_handler, ".int10")
