/***************************************************************************************
 *  record_button.c
 *
 *  This program records button pushes and plays them back on the green on-board LED.
 * 	If in recording mode, the red LED is lit.  If in playback mode, the red LED is off.
 *
 *  This version uses the Watchdog Timer ("WDT") in interval mode.
 *  The default system clock is about 1.1MHz.  The WDT is configured to be driven by this
 *  clock and produce an interrupt every 8K ticks ==> interrupt interval = 8K/1.1Mhz ~ 7.5ms
 *  When the WDT interrupt occurs, the WDT interrupt handler is called.
 *
 *
 *  Initially the green LED is off and the red LED is on.
 *
 *  NOTE: Between interrupts the CPU is OFF!
 ***************************************************************************************/
#include <msp430g2553.h>

// Bit masks for port 1
#define RED 0x01
#define GREEN 0x40
#define BUTTON 0x08

#define PUSHES 60

// Global state variables
volatile unsigned char last_button; // the state of the button bit at the last interrupt
volatile unsigned char inPlayback; //boolean-esque variable to determine if the state is record(0) or playback(1)
volatile unsigned int counter; //count the number of watchdog interrupts per button push or non-push
volatile int arrayIncr; //counter variable to increment throughthe storage array recorded[PUSHES]
volatile unsigned int recorded[PUSHES]; //array recording counter variable to know how long button pushed (or not pushed) for
volatile unsigned char buttonDown; //state variable to know if button is being held down
volatile unsigned char buttonUp; //state variable to know if button isn't being pushed
volatile unsigned int playbackArrayIterator; //iterator variable to loop through the recorded array during playback
volatile unsigned int playbackDownCounter; //counter variable for playback so the watchdog interupt handler knows when to move to next button push or not push
volatile unsigned int pushesRecorded; //holds the number of pushes/not-pushes of the button during a single instance of the recording mode
volatile unsigned int i;

void main(void) {
	  // setup the watchdog timer as an interval timer
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

	  P1DIR |= RED+GREEN;					// Set RED and GREEN to output direction
	  P1DIR &= ~BUTTON;						// make sure BUTTON bit is an input
	  
	  counter = 0;
	  arrayIncr = -1;
	  playbackArrayIterator = 0;
	  playbackDownCounter = 0;
	  pushesRecorded = 0;
	  inPlayback = 0; //initialize state to recording mode
	  P1OUT |= RED;  //initialize Red on to start in record mode
	  P1OUT &= ~GREEN;

	  P1OUT |= BUTTON;
	  P1REN |= BUTTON;						// Activate pullup resistors on Button Pin
	  
	  _bis_SR_register(GIE+LPM0_bits);  // enable interrupts and also turn the CPU off!
}

// ===== Watchdog Timer Interrupt Handler =====
// This event handler is called to handle the watchdog timer interrupt,
//    which is occurring regularly at intervals of about 8K/1.1MHz ~= 7.4ms.

interrupt void WDT_interval_handler(){
  	unsigned char b;
  	b= (P1IN & BUTTON);  // read the BUTTON bit

  	if (inPlayback == 0) //record mode
  	{
  		if (last_button && (b==0)) //button pushed
  		{
  			buttonDown = 1;
  			buttonUp = 0;
  			P1OUT ^= GREEN;

  			if (arrayIncr >= 0) //first recorded thing should be a button push, not empty space
  			{
  				recorded[arrayIncr] = counter;
  				counter = 0; //reset counter variable for next push/non-push 
  				arrayIncr++; //increment to next space in array
  			}

  			counter++;

  			if (arrayIncr < 0)
  			{
  				arrayIncr++;
  			}
  		}
  		else if (((last_button==0) && b) && ((arrayIncr >= 0)||(counter>0)))//button released
  		{
  			buttonDown = 0;
  			buttonUp = 1;
  			counter++;
  			recorded[arrayIncr] = counter;
  			P1OUT ^= GREEN;
  			counter = 0; //reset counter variable for next push/non-push
  			arrayIncr++; //increment to next space in array
  		}
  		else if ((buttonDown == 1) || (buttonUp == 1)) //button is staying in one state (pressed or not pressed)
  		{
  			counter++;//increment counter

  			if ((counter >= 676) && (buttonDown == 0) && (arrayIncr >= 0)) //if we've gone through around 5 seconds without button push
  			{
  				pushesRecorded = arrayIncr; //this adds a blank space to the end
  				recorded[arrayIncr] = 100;
  				counter = 0;
  				P1OUT &= ~RED;//turn red LED off to indicate end of record mode/start of playback mode
  				inPlayback = 1;
  			}

  		}
  		else
  		{
  			counter = 0;
  		}


  	}
  	else //playback mode
  	{
  		//in playback mode, we need to start with the green LED on.  It will remain on for the length of time in the first
  		//array cell.  Then it will go off for the length of time in the next cell, etc.  When the array has been exhausted,
  		//return to playback mode.

  		if ((playbackArrayIterator == 0) || (--playbackDownCounter == 0))
  		{
  			playbackDownCounter = recorded[playbackArrayIterator];
  			playbackArrayIterator++;
  			P1OUT ^= GREEN;
  		}

  		if ((playbackArrayIterator-2) == pushesRecorded) //end of message being played
  		{
  			//return all values to zero for next record mode
  				for (i=0; i<PUSHES; i++)
  				{
  					recorded[i] = 0;
  				}

  				counter = 0;
	  			arrayIncr = -1;
	  			playbackArrayIterator = 0;
	  			playbackDownCounter = 0;
	  			pushesRecorded = 0;
	  			buttonDown = 0;
	  			buttonUp = 0;

  				P1OUT |= RED;//turn red LED on to indicate entering record mode
  				P1OUT &= ~GREEN; //make sure green turned off
	  			inPlayback = 0; //return to record mode
  		}

  	}
  	last_button=b;    // remember button reading for next time.
}
// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")
