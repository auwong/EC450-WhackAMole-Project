#include "msp430g2553.h"
								//P1.1 Reserved for UART / LCD Panel
#define TA1_BIT 0x02			//P2.1 - Theoretically for sound
#define MOLE_LED1 0x01			//P2.0
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
volatile unsigned char countOn=0;
volatile unsigned char soundTime=0;
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

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++// Main
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

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

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++// Init buttons and timer
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

void init_buttons(){
	P1DIR |= MOLE_LED2+MOLE_LED3+MOLE_LED4;		//output LEDs
	P2DIR |= MOLE_LED1;
	P1OUT &= ~(MOLE_LED2+MOLE_LED3+MOLE_LED4);	//Turn off LEDs
	P2OUT &= ~(MOLE_LED1);

	P1OUT |= MOLE_BUTTON1+SR_BUTTON; // pullup
	P1REN |= MOLE_BUTTON1+SR_BUTTON; // enable resistor
	P2OUT |= MOLE_BUTTON2+MOLE_BUTTON3+MOLE_BUTTON4;
	P2REN |= MOLE_BUTTON2+MOLE_BUTTON3+MOLE_BUTTON4;
}

void init_timer(){ 			// initialization and start of timer
	TA1CTL |=TACLR; // reset clock
	TA1CTL =TASSEL_2+ID_0+MC_1; // clock source = SMCLK, clock divider=1, UP mode,
	TA1CCTL0=soundOn+CCIE; // compare mode, outmod=sound, interrupt CCR1 on
	TA1CCR0 = 1000;
	P2SEL|=TA1_BIT; // connect timer output to pin
	P2DIR|=TA1_BIT;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++// Random Generator
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

unsigned random_gen(void)
{
  unsigned int i, j;
  unsigned int result = 0;

  // setup Timer_A
  BCSCTL3 = (~LFXT1S_3 & BCSCTL3) | LFXT1S_2; // Source ACLK from VLO
  TA0CCTL0 = CM_1 + CCIS_1 + CAP;
  TA0CTL |= TASSEL_2 + MC_2;

  for(i=0 ; i<16 ; i++)
  {
    // shift left result
    result <<= 1;
    char k = 0;

    for (j=0 ; j<5; j++){

		// wait until Capture flag is set
		while(!(TA0CCTL0 & CCIFG));

		// clear flag
		TA0CCTL0 &= ~CCIFG;

		// check LSB
		if(TA0CCR0 & 0x01)
		{
		  k++;
		}
	}
    if (k>=3)
    	result |= 0x01;
  }

  return result;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++// Sound handler
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

void interrupt sound_handler(){
	if (soundOn && soundTime>=20){
		soundOn ^= OUTMOD_4;
		soundTime = 0;
	}
	TA1CCTL1 = CCIE + soundOn; //  update control register with current soundOn
}
ISR_VECTOR(sound_handler,".int13") // declare interrupt vector

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++// WDT handler
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

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
	
	/****************** Sound Control *******************/
	if (soundOn!=0)
		soundTime++;

	/****************** State Control *******************/

	if(state == 'p'){									/////Play state
		if(--next_mole==0){
			mole_interval = 100 - (stage*20);
			rand = random_gen();
			rand_on = (rand % 4) + 1;
			switch(rand_on){		// picks at random the next mole
				case 1:{
					if(!mole1_on){
						mole1_on = 1;
						mole1_count = mole_interval;
					}
				}break;
				case 2:{
					if(!mole2_on){
						mole2_on = 1;
						mole2_count = mole_interval;
					}
					} break;
				case 3:{
					if(!mole3_on){
						mole3_on = 1;
						mole3_count = mole_interval;
					}
					} break;
				case 4:{
					if(!mole4_on){
						mole4_on = 1;
						mole4_count = mole_interval;
					}
					} break;
			}
			next_mole = mole_interval;
		}
		
		if(mole1_on){
			P2OUT |= MOLE_LED1;
			if(--mole1_count==0){
				//do stuff
				misses++;
				mole1_on = 0;
			}
		} else{
			P2OUT &= ~MOLE_LED1;
			mole1_count = 0;
		}
		
		if(mole2_on){
			P1OUT |= MOLE_LED2;
			if(--mole2_count==0){
				// do stuff
				misses++;
				mole2_on = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED2;
			mole2_count = 0;
		}
		
		if(mole3_on){
			P1OUT |= MOLE_LED3;
			if(--mole3_count==0){
				// do stuff
				misses++;
				mole3_on = 0;
			}
		} else{
			P1OUT &= ~MOLE_LED3;
			mole3_count = 0;
		}
		
		if(mole4_on){
			P1OUT |= MOLE_LED4;
			if(--mole4_count==0){
				// do stuff
				misses++;
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
	else if(state == 'r'){									/////////Reseted State
		// stays paused until play is pressed, dunno if anything need to happen in here
		misses = 0;
		hits = 0;
		stage = 0;
		mole_interval = 300;
		mole_counter = 0;
		mole1_on = 0;
		mole2_on = 0;
		mole3_on = 0;
		mole4_on = 0;
		P1OUT &= ~(MOLE_LED2+MOLE_LED3+MOLE_LED4);	//Turn off LEDs
		P2OUT &= ~MOLE_LED1;
		state = 'r';
	}
	
	/****************** Button control *******************/

	if(last_play && (play==0)){
		if(state == 'r'){
			state = 'p';
			soundOn ^= OUTMOD_4;
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
			P1OUT &= ~(MOLE_LED2+MOLE_LED3+MOLE_LED4);	//Turn off LEDs
			P2OUT &= ~MOLE_LED1;
			state = 'r';
		}
	}
	last_play = play;
	
	if(last_mole1 && (mole1==0)){
		if(mole1_on){
			hits++;
			mole1_on = 0;
		} else{
			misses++;
		}
		//P2OUT ^= MOLE_LED1;			// debug button operation
		soundOn ^= OUTMOD_4;
	}
	last_mole1 = mole1;
	
	if(last_mole2 && (mole2==0)){
		if(mole2_on){
			hits++;
			mole2_on = 0;
		} else{
			misses++;
		}
		//P1OUT ^= MOLE_LED2;			// debug button operation
		soundOn ^= OUTMOD_4;
	}
	last_mole2 = mole2;
	
	if(last_mole3 && (mole3==0)){
		if(mole3_on){
			hits++;
			mole3_on = 0;
		} else{
			misses++;
		}
		//P1OUT ^= MOLE_LED3;			// debug button operation
		soundOn ^= OUTMOD_4;
	}
	last_mole3 = mole3;
	
	if(last_mole4 && (mole4==0)){
		if(mole4_on){
			hits++;
			mole4_on = 0;
		} else{
			misses++;
		}
		//P1OUT ^= MOLE_LED4;			// debug button operation
		soundOn ^= OUTMOD_4;
	}
	last_mole4 = mole4;

}
ISR_VECTOR(WDT_interval_handler, ".int10")
