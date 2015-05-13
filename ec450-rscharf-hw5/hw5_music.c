asm(" .length 10000");
asm(" .width 132");
// MAR 2015
// EC 450 HW 5 Reva Scharf
// Music Player
//-----------------------
#include "msp430g2553.h"

#define B0  16189
#define C1  15280
#define CS1 14422
#define D1  13613
#define DS1 12849
#define E1  12128
#define F1  11447
#define FS1 10805
#define G1  10198
#define GS1 9626
#define A1  9086
#define AS1 8576
#define B1  8094
#define C2  7640
#define CS2 7211
#define D2  6806
#define DS2 6424
#define E2  6064
#define F2  5724
#define FS2 5402
#define G2  5099
#define GS2 4813
#define A2  4543
#define AS2 4288
#define B2  4047
#define C3  3820
#define CS3 3606
#define D3  3403
#define DS3 3212
#define E3  3032
#define F3  2862
#define FS3 2701
#define G3  2550
#define GS3 2406
#define A3  2271
#define AS3 2144
#define B3  2024
#define C4  1910
#define CS4 1803
#define D4  1702
#define DS4 1606
#define E4  1516
#define F4  1431
#define FS4 1351
#define G4  1275
#define GS4 1203
#define A4  1136
#define AS4 1072
#define B4  1012
#define C5  955
#define CS5 901
#define D5  851
#define DS5 803
#define E5  758
#define F5  715
#define FS5 675
#define G5  637
#define GS5 602
#define A5  568
#define AS5 536
#define B5  506
#define C6  478
#define CS6 451
#define D6  425
#define DS6 402
#define E6  379
#define F6  358
#define FS6 338
#define G6  319
#define GS6 301
#define A6  284
#define AS6 268
#define B6  253
#define C7  239
#define CS7 225
#define D7  213
#define DS7 201
#define E7  189
#define F7  179
#define FS7 169
#define G7  159
#define GS7 150
#define A7  142
#define AS7 134
#define B7  126
#define C8  119
#define CS8 113
#define D8  106
#define DS8 100

#define R	20 //rest
#define WN 400 //whole note
#define DHN 300//dotted half note
#define HN 200//half note
#define DQN 150//dotted quarter note
#define QN 100//quarter note
#define EN 50//eighth note
#define SN 25//sixteenth note

//-----------------------
// define the bit mask (within P1) corresponding to output TA0
#define TA0_BIT 0x02

//define bit masks for red and green on board LEDs
#define RED 0x01
#define GREEN 0x40

// define the port and location for the buttons to be used
// specific bit for the button
#define SPR_BUTTON 0x20
#define ACCEL_BUTTON 0x80
#define RIT_BUTTON 0x10
#define SEL_BUTTON 0x04
//----------------------------------

// Some global variables (mainly to look at in the debugger)
volatile unsigned char last_spr_button;
volatile unsigned char last_rit_button;
volatile unsigned char last_accel_button;
volatile unsigned char last_sel_button;
volatile unsigned char spr_down;
volatile unsigned int pause_time_counter;
volatile unsigned halfPeriod; // half period count for the timer
volatile unsigned long intcount=0; // number of times the interrupt has occurred
volatile unsigned soundOn=0; // state of sound: 0 or OUTMOD_4 (0x0080)
volatile unsigned char song_selection = 0;  //0 - Joy to the World // 1 - other song
volatile int songIterator = 0;  //used to iterate through values of Melody/Durations arrays.  set back to 0 on reset.
volatile unsigned int downCounter=0;
volatile unsigned int noteSpaceCounter = 0;
volatile float speed;
volatile unsigned char songFinish = 0;  //0-not finished 1-finished
volatile unsigned char paused = 1; //0=not paused 1=paused
volatile unsigned char newPlay = 1;  //0=continuing 1=just started 1st note of song
volatile unsigned char inNoteSpace = 0;

volatile unsigned minHP=100;  // minimum half period
volatile unsigned maxHP=2000; // maximum half period
volatile int deltaHP=-1; // step in half period per half period

int songLen = 0;
const int JoyToWorld_Melody[] = {C5,C5,B4,A4,G4,F4,E4,D4,C4,G4,A4,A4,B4,B4,C5,C5,C5,B4,A4,G4,G4,F4,E4,C5,C5,B4,A4,G4,G4,F4,E4,E4,E4,E4,E4,E4,F4,G4,F4,E4,D4,D4,D4,D4,E4,F4,E4,D4,E4,C5,A4,G4,F4,E4,F4,E4,D4,C4};
const int JoyToWorld_Durations[] = {HN,HN,DQN,EN,DHN,QN,HN,HN,DHN,QN,DHN,QN,DHN,QN,DHN,QN,QN,QN,QN,QN,DQN,EN,QN,QN,QN,QN,QN,QN,DQN,EN,QN,QN,QN,QN,QN,EN,EN,DHN,EN,EN,QN,QN,QN,EN,EN,DHN,EN,EN,QN,HN,QN,DQN,EN,QN,QN,HN,HN,WN};
const int JoyLen = (sizeof(JoyToWorld_Melody)/2);

const int Jeopardy_Melody[] = {G5,G5,C6,G5,C5,C5,G5,C6,G5,R,G5,C6,G5,C6,E6,D6,C6,B5,A5,GS5,G5,C6,G5,C5,C5,G5,C6,G5,C6,R,A5,G5,F5,E5,D5,C5,R,AS4,DS5,AS4,DS4,AS4,DS5,AS4,AS4,DS5,AS4,DS5,G5,R,F5,DS5,D5,C5,B4,AS4,DS5,AS4,DS4,AS4,DS5,AS4,DS5,R,C5,AS4,GS4,G4,R,F4,DS4,AS3,DS2};
const int Jeopardy_Durations[] = {QN,QN,QN,QN,EN,EN,QN,QN,QN,QN,QN,QN,QN,QN,DQN,EN,EN,EN,EN,EN,QN,QN,QN,EN,EN,QN,QN,HN,QN,EN,EN,QN,QN,QN,QN,QN,QN,QN,QN,QN,QN,QN,QN,HN,QN,QN,QN,QN,QN,EN,EN,EN,EN,EN,EN,QN,QN,QN,QN,QN,QN,HN,QN,EN,EN,QN,QN,QN,QN,QN,QN,QN,QN,HN};
const int JeopardyLen = (sizeof(Jeopardy_Melody)/2);

//ignore this.  I started putting in a third song, but ran out of time.
//const int CannonD_Melody[] = {FS5,E5,D5,CS5,B5,A5,B5,CS5,D5,CS5,B4,A4,G4,FS4,G4,E4,FS5,E5,D5,CS5,B4,A4,B4,CS5,A4,D5,E5,CS5,D5,FS5,A5,A4,B4,G4,A4,A4,G4,DG,CS5,A4, //PAGE 1
	//						  D5,CS5,D5,FS4,A4,A4,CS5,D5,D5,FS5,A5,A5,B5,G5,FS5,E5,G5,FS5,E5,D5,CS5,B4,A4,D5,CS5,D5,CS5,D5,CS5,D5,FS4,A4,CS5,D5,FS5,A5,A5,B5,G5,FS5,E5,G5,FS5,E5,D5,CS5,B4,A4,D5,CS5,D5,CS5,D5,FS5,G5,A5,FS5,G5,A5,A4,B4,CS5,D5,E5,FS5,G5,FS5,D5,E5,FS5,FS4,G4,A4,B4,A4,G4,A4,FS4,G4,A4,D5,B4,A4,G4,FS4,E4,FS4,E4,D4,E4,FS4,G4,A4,B4,D5,B4,A4,B4,E5,D5,CS5,B5,BS5,D5,D5,FS5,G5,E5, //PAGE 2
		//					  };


void init_timer(void); // routine to setup the timer
void init_buttons(void); // routine to setup the button

// ++++++++++++++++++++++++++
void main(){
	//WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
	 WDTCTL =(WDTPW + // (bits 15-8) password
		                   // bit 7=0 => watchdog timer on
		                   // bit 6=0 => NMI on rising edge (not used here)
		                   // bit 5=0 => RST/NMI pin does a reset (not used here)
		           WDTTMSEL + // (bit 4) select interval timer mode
		           WDTCNTCL +  // (bit 3) clear watchdog timer counter
		  		          0 // bit 2=0 => SMCLK is the source
		  		          +1 // bits 1-0 = 01 => source/8K
		  		   );
		  IE1 |= WDTIE;		// enable the WDT interrupt (in the system interrupt register IE1)

	BCSCTL1 = CALBC1_1MHZ; // 1Mhz calibration for clock
	DCOCTL = CALDCO_1MHZ;

	P1DIR |= GREEN;
	P1OUT &= ~GREEN;

	P1DIR |= RED;
	P1OUT &= ~RED;

	halfPeriod=maxHP; // initial half-period at lowest frequency
	init_timer(); // initialize timer
	init_buttons(); // initialize the button
	spr_down = 0;
	speed = 4;  //initialized for joy to the world
	//soundOn = 0;
	_bis_SR_register(GIE+LPM0_bits);// enable general interrupts and power down CPU
}

// +++++++++++++++++++++++++++
// Sound Production System
void init_timer(){ // initialization and start of timer
	TA0CTL |=TACLR; // reset clock
	TA0CTL =TASSEL1+ID_0+MC_1; // clock source = SMCLK, clock divider=1, continuous mode,
	TA0CCTL0=soundOn+CCIE; // compare mode, outmod=sound, interrupt CCR1 on
	TA0CCR0 = halfPeriod; // time for first alarm
	P1SEL|=TA0_BIT; // connect timer output to pin
	P1DIR|=TA0_BIT;
	soundOn = 0;
}

// +++++++++++++++++++++++++++
void interrupt sound_handler(){
	if (song_selection == 0)//joy to world
		{
			TACCR0 = JoyToWorld_Melody[songIterator-1];
		}
	else
		{
			TACCR0 = Jeopardy_Melody[songIterator-1];
		}

	TA0CCTL0 = CCIE + soundOn; //  update control register with current soundOn
	//++intcount; // advance debug counter
}
ISR_VECTOR(sound_handler,".int09") // declare interrupt vector

// +++++++++++++++++++++++++++
// Button input System
// Button toggles the state of the sound (on or off)
// action will be interrupt driven on a downgoing signal on the pin
// no debouncing (to see how this goes)

void init_buttons(){
	//init start,pause,reset button
	P1OUT |= SPR_BUTTON; // pullup
	P1REN |= SPR_BUTTON; // enable resistor

	//init accelerando (speed up) button
	P1OUT |= ACCEL_BUTTON; // pullup
	P1REN |= ACCEL_BUTTON; // enable resistor


	//init ritardando (slow down) button
	P1OUT |= RIT_BUTTON; // pullup
	P1REN |= RIT_BUTTON; // enable resistor

	//init select song button
	P1OUT |= SEL_BUTTON; // pullup
	P1REN |= SEL_BUTTON; // enable resistor
}


interrupt void WDT_interval_handler(){
  	unsigned char b_spr;
  	unsigned char b_rit;
  	unsigned char b_accel;
  	unsigned char b_sel;
  	b_rit= (P1IN & RIT_BUTTON);
  	b_accel = (P1IN & ACCEL_BUTTON);
  	b_sel = (P1IN & SEL_BUTTON);
  	b_spr= (P1IN & SPR_BUTTON);  // read the BUTTON bit

//TIMING NOTES
  		if (downCounter == 0)
  		{
  			if(noteSpaceCounter == 0)
  			{
  				soundOn = 0;
  			}
  			if(noteSpaceCounter++ == 5)
  			{
  				noteSpaceCounter = 0;
  				inNoteSpace = 0;

  				if (song_selection == 0) //joy
  				{
  					downCounter = JoyToWorld_Durations[songIterator] / speed;
  					songLen = JoyLen+1;
  					P1OUT |= GREEN;
  					P1OUT &= ~RED;
  				}
  				else
  				{
  					downCounter = Jeopardy_Durations[songIterator] / speed;
  					songLen = JeopardyLen + 1;
  					P1OUT |= RED;
  					P1OUT &=~GREEN;
  				}

  				if (newPlay == 0){
  				soundOn = OUTMOD_4;
  				}
  				songIterator++;
  				if (songIterator == songLen)
  				{
  					soundOn = 0;
  					newPlay = 1;
  					//songIterator = 1;
  					songFinish=1;
  					paused = 1;
  				}
  			}
  		}
  		else if (paused == 0)
  		{
  			downCounter--;
  		}

  	//BUTTON HANDLING
  	  	//handles start, pause, reset button
  	  	if (last_spr_button && (b_spr==0)){ // has the button bit gone from high to low

  	  		newPlay = 0;
  			spr_down = 1; //button is pushed down
  			pause_time_counter++;

  	  		if (paused==0) //not paused
  	  		{
  	  			if (song_selection == 0)
  	  			{
  	  				P1OUT |= GREEN;
  	  			}
  	  			else
  	  			{
  	  				P1OUT |= RED;
  	  			}
  	  			if ((songFinish==0) && (inNoteSpace==0))
  	  			{
  	  				soundOn^=OUTMOD_4;
  	  				paused=1;
  	  			}
  	  			else if ((songFinish==0) && (inNoteSpace==1))
  	  			{
  	  				soundOn=0;
  	  				paused=1;
  	  			}
  	  			else
  	  			{
  	  				soundOn=0;
  	  				paused=1;
  	  			}
  	  		}
  	  		else //paused
  	  		{
  	  			if (inNoteSpace == 1)
  	  			{
  	  				soundOn = 0;
  	  			}
  	  			paused=0;
  	  			P1OUT &= ~GREEN;
  	  			P1OUT &= ~RED;
  	  		}
  	  	}
  		else if (((last_spr_button==0) && b_spr) && (pause_time_counter > 0)) //button released
  		{
  			spr_down = 0;
  			pause_time_counter = 0;
  		}
  		else if (spr_down == 1) //held button down
  		{
  			pause_time_counter++;

  			if ((pause_time_counter >= 201) && (spr_down == 1))
  			{
  				//reset to beginning of song
  				songIterator = 1;
  				pause_time_counter = 0; //reset var
  				newPlay = 1;
  				if (song_selection == 0){
  				  	  			speed = 4;
  				  	  		}
  				  	  		else{
  				  	  			speed = 2;
  				  	  		}
  			}
  		}

  	  	if (last_rit_button && (b_rit==0)){ // has the button bit gone from high to low
  	  		if (speed > .5)
  	  		{
  	  			speed -= .5;
  	  		}
  	  	}

  	  	if (last_accel_button && (b_accel==0)){ // has the button bit gone from high to low
  	  		speed += .5;
  	  	}

  	  	if (last_sel_button && (b_sel==0)){ // has the button bit gone from high to low
  	  		if (song_selection == 0)
  	  		{
  	  			songIterator = 1;
  	  			song_selection = 1;
  	  			speed = 2; //arbitrary.  change later when song known
  	  		}
  	  		else
  	  		{
  	  			songIterator = 1;
  	  			song_selection = 0;
  	  			speed = 4;
  	  		}
  	  	}
  	last_spr_button=b_spr;    // remember button reading for next time.
	last_rit_button = b_rit;
	last_accel_button = b_accel;
	last_sel_button = b_sel;
}
// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")
// +++++++++++++++++++++++++++
