#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>


/**
 * main.c
 */
	//globals
volatile bool pressDetected = false;
volatile uint16_t pressBuffer = 0;
void setRTC(void);
void startOUT(void);
void stopOUT(void);
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	//GPIO setup
	P1DIR = 0xFF;//set P1 pins as output
	P2DIR = 0xFF;//set P2 pins as output except switch pin
	P2DIR &= ~BIT3;
	PM5CTL0 &= ~LOCKLPM5;
	P2OUT = BIT3; //all P2 pins output low, except switch pin which is set to pullup
	P2REN = BIT3; //enable pullup on switch pin
	P1OUT = ~(BIT0 | BIT1);//all pins low except LEDs
    TA0CCR0 = 1600-1;                         // PWM Freq (32.768kHz divided by TA0CCR0)
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR1 = 400;                            // CCR1 PWM duty cycle (TA0CCR1/TA0CCR0)
	setRTC();
//	CAPTIOCTL = 0;

	__enable_interrupt();

	    while (1)
	    {
	        if (!(P2IN & BIT3))//key press
	        {
	            pressDetected = true;
	            if (pressBuffer & BIT1)//previous slot had a press - stop output and reset frame
	            {
	                pressBuffer = 1;
	                stopOUT();
	            }
	            else
	            {
	                pressBuffer +=1;//push
	                if (pressBuffer >0x8000)//there's at least one more press detection on the buffer and it isn't the previous slot
	                    startOUT();
	            }
	            while (!(P2IN & BIT3));
	            {
//	                if (!pressDetected) break;
	            }
	            while (pressDetected);
	        }
//	        else if (pressBuffer < 2)
//	            P1OUT &= (~BIT0);
	    }
	return 0;
}

void startOUT(void)
{
    TA0CTL = TASSEL__ACLK | MC__UP | TACLR;  // ACLK, up mode, clear TAR
    P1SEL1 |= BIT1;
    PM5CTL0 &= ~LOCKLPM5;
}
void stopOUT(void)
{
    TA0CTL = TASSEL__ACLK | MC__STOP | TACLR;  // ACLK, stop mode, clear TAR
    P1SEL1 &= ~(BIT1);
}

void setRTC(void)
{
    RTCMOD = 125;
    RTCCTL = RTCSS__VLOCLK | RTCSR | RTCPS__10 | RTCIE;
}
#pragma vector=RTC_VECTOR
__interrupt
void RTC_ISR(void)
{
//    P1OUT ^= BIT0;
    pressDetected = false;
    pressBuffer = (pressBuffer<<1);//advance the buffer
    if (pressBuffer < 2)
        {
        stopOUT();
        setRTC();
        }
    setRTC();
}
