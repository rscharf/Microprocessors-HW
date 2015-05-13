/* 4-10-2015

SPI SLAVE

This code is for the slave of our HW6 SPI system.  The system involves a master getting input from
a photoresistor and using the ADC to translate it into a digital signal and send it to the slave.
The slave (the code in this document) plays a frequency on a speaker which depends on the value
from the photoresistor that the master gives it.

 Timing and clock.
 MCLK and SMCLK = 8 Mhz
 UCB0BRx interface divisor is a parameterized below.
 WDT divides SMCL by 512 (==> fastest rate gives 1 TX every 64 microseconds)
 Parameter ACTION_INTERVAL controls actual frequency of WDT interrupts that TX
 16 bit Parameter BIT_RATE_DIVISOR controls the SPI bitrate clock
*/

#include "msp430g2553.h"

 /* declarations of functions defined later */
 void init_spi(void);
 void init_wdt(void);

// Global variables and parameters (all volatilel to maintain for debugger)
volatile unsigned long lightHP;

volatile unsigned char data_received= 0; 	// most recent byte received

volatile unsigned char justReceived = 0;

volatile int counter=0;

union {
	volatile int lightValue;
	volatile char bytes[2];
} received;

// Try for a fast send.  One transmission every 64 microseconds
// bitrate = 1 bit every 4 microseconds
#define BIT_RATE_DIVISOR 32

#define TA0_BIT 0x02
const unsigned int LIGHT_RANGE = 1023;
const unsigned int LIGHT_MIN = 0;
const unsigned int LIGHT_MAX = 1023;
const unsigned int SOUND_RANGE = 1900;
const unsigned int SOUND_MIN = 100;
const unsigned int SOUND_MAX = 2000;


void init_wdt(){
	// setup the watchdog timer as an interval timer
	// INTERRUPT NOT YET ENABLED!
  	WDTCTL =(WDTPW +		// (bits 15-8) password
     	                   	// bit 7=0 => watchdog timer on
       	                 	// bit 6=0 => NMI on rising edge (not used here)
                        	// bit 5=0 => RST/NMI pin does a reset (not used here)
           	 WDTTMSEL +     // (bit 4) select interval timer mode
  		     WDTCNTCL  		// (bit 3) clear watchdog timer counter
  		                	// bit 2=0 => SMCLK is the source
  		                	// bits 1-0 = 10=> source/512
 			 );
 }

//----------------------------------------------------------------

// ======== Receive interrupt Handler for UCB0 ==========

void interrupt spi_rx_handler(){
	data_received=UCB0RXBUF; // copy data to global variable

	received.bytes[counter%2] = data_received;
	if (counter%2 == 1)
	{
		justReceived = 1;
	}

	counter++;

	IFG2 &= ~UCB0RXIFG;		 // clear UCB0 RX flag
}
ISR_VECTOR(spi_rx_handler, ".int07")


//Bit positions in P1 for SPI
#define SPI_CLK 0x20
#define SPI_SOMI 0x40
#define SPI_SIMO 0x80

// calculate the lo and hi bytes of the bit rate divisor
#define BRLO (BIT_RATE_DIVISOR &  0xFF)
#define BRHI (BIT_RATE_DIVISOR / 0x100)

void init_spi(){


	UCB0CTL1 = UCSSEL_2+UCSWRST;  		// Reset state machine; SMCLK source;
	UCB0CTL0 = UCCKPH					// Data capture on rising edge
			   							// read data while clock high
										// lsb first, 8 bit mode,
			   +UCMODE_0				// 3-pin SPI mode
			   +UCSYNC;					// sync mode (needed for SPI or I2C)
	UCB0BR0=BRLO;						// set divisor for bit rate
	UCB0BR1=BRHI;
	UCB0CTL1 &= ~UCSWRST;				// enable UCB0 (must do this before setting
										//              interrupt enable and flags)
	IFG2 &= ~UCB0RXIFG;					// clear UCB0 RX flag
	IE2 |= UCB0RXIE;					// enable UCB0 RX interrupt
	// Connect I/O pins to UCB0 SPI
	P1SEL =SPI_CLK+SPI_SOMI+SPI_SIMO;
	P1SEL2=SPI_CLK+SPI_SOMI+SPI_SIMO;

}

void init_timer(){ // initialization and start of timer
	TA0CTL |=TACLR; // reset clock
	TA0CTL =TASSEL_2+ID_3+MC_1; // clock source = SMCLK, clock divider=8, continuous mode,
	TA0CCTL0=CCIE; // compare mode, outmod=sound, interrupt CCR1 on
	TA0CCR0 = 500; // time for first alarm
	P1SEL|=TA0_BIT; // connect timer output to pin
	P1DIR|=TA0_BIT;
}

void interrupt sound_handler(){
	TA0CCTL0 = CCIE + OUTMOD_4;
	if ((counter%2 == 0) && (justReceived == 1))//make sure we have both bytes of the master's data
	{
		//map the range of light values to the range of halfperiods
		lightHP = received.lightValue - LIGHT_MIN;
		lightHP = lightHP * SOUND_RANGE;
		lightHP = lightHP / LIGHT_RANGE;
		lightHP = lightHP + SOUND_MIN;
		TA0CCR0 = lightHP;

		justReceived = 0;
	}
}
ISR_VECTOR(sound_handler,".int09") // declare interrupt vector
/*
 * The main program just initializes everything and leaves the action to
 * the interrupt handlers!
 */

void main(){

	WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;			// 8Mhz calibration for clock
  	DCOCTL  = CALDCO_8MHZ;
//init stuff
  	init_spi();
  	init_wdt();
  	init_timer();
 	_bis_SR_register(GIE+LPM0_bits);
}
