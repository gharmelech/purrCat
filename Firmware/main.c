/*purrCat - a purring cat PCB by Gal Harmelech
 * I wanted to make this cat as picky as a real cat, so it does not purr for any type of petting:
 * The main principle is a 2 seconds buffer with 250 ms slots, this enables identifying "petting" between 0.5 to 4 Hz
 * If being petted with an accaptable pattern for more than 1 second - It will start purring.
 * For the Purring there is a 40 Hz PWM function with 0.25 to 0.33 duty cycle dynamicaly changing to give the small ramp-up and down in a cat's purring
 * The eyes just light up whenever it is purring, but more sophisticated schemes are possiable as the eyes are wired to the seconed timer output
 * Feel free to improve and expend this software, I'm curious to see what the community could do with it!
 * My one request is that you do not remove my name from it
 * Have Fun!
 * 
 * Some parts of the software are based on TI's example code, for those parts the followind is aplicable:
 * 
 * --COPYRIGHT--,BSD_EX
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

/*Time constants*/
#define WDT_meas_setting (DIV_SMCLK_512)            // Defines WDT SMCLK interval for sensor measurements with duration of 0.5 ms
#define WAIT_PERIOD_FACTOR (5)                      // actual delay is given by ((Factor)*256)/10 (ms)

/*Idle Mode*/
//#define USE_IDLEMODE
#ifdef USE_IDLEMODE
#define ACTIVE_TIME 30                              // Time to stay active before entering idle mode (sec)
#define IDLE_LIMIT (ACTIVE_TIME<<2)
#define IDLE_SAMPLE_PERIOD 1500                     // Time between samples when in idle mode (ms)
#define IDLE_SAMPLE_PERIOD_FACTOR ((int)((10 * IDLE_SAMPLE_PERIOD) / 256))
#endif

/* Sensor settings*/
#define KEY_LVL     (750)                           // Defines threshold for a key press
/*Set to ~ half the max delta expected*/

/* Definitions for use with the WDT settings*/
#define DIV_ACLK_32768  (WDT_ADLY_1000)             // ACLK/32768
#define DIV_ACLK_8192   (WDT_ADLY_250)              // ACLK/8192
#define DIV_ACLK_512    (WDT_ADLY_16)               // ACLK/512
#define DIV_ACLK_64     (WDT_ADLY_1_9)              // ACLK/64
#define DIV_SMCLK_32768 (WDT_MDLY_32)               // SMCLK/32768
#define DIV_SMCLK_8192  (WDT_MDLY_8)                // SMCLK/8192
#define DIV_SMCLK_512   (WDT_MDLY_0_5)              // SMCLK/512
#define DIV_SMCLK_64    (WDT_MDLY_0_064)            // SMCLK/64

/*Outputs*/
#define LED                 (BIT0)                  // P2.0 LED output
#define MOTOR               (BIT7)                  // P1.7 Vibration motor output

/*Touch Pads*/
#define TOUCHPAD_1          (BIT6)                  // P1.6 Touchpad
#define CAPTURE_TOUCHPAD_1   (CAPTIOEN + CAPTIOPOSEL0 + CAPTIOPISEL_6)
#define TOUCHPAD_2          (BIT5)                  // P1.5 Touchpad
#define CAPTURE_TOUCHPAD_2   (CAPTIOEN + CAPTIOPOSEL0 + CAPTIOPISEL_5)
#define TOUCHPAD_3          (BIT4)                  // P1.4 Touchpad
#define CAPTURE_TOUCHPAD_3   (CAPTIOEN + CAPTIOPOSEL0 + CAPTIOPISEL_4)
#define TOUCHPAD_4          (BIT3)                  // P1.3 Touchpad
#define CAPTURE_TOUCHPAD_4   (CAPTIOEN + CAPTIOPOSEL0 + CAPTIOPISEL_3)

// Global variables for sensing
unsigned int base_cnt, meas_cnt;
int delta_cnt;
uint16_t pressBuffer = 0;
uint8_t outputEnable = 0;
uint8_t dutyCounts [] = {200, 200, 255, 255, 255};
uint8_t dutyIndex = 0;
#ifdef USE_IDLEMODE
uint8_t idleCnt = 0;
#endif
/* System Routines*/
void measure_count(void);                           // Measures each capacitive sensor
void startPWM(void);
void stopPWM(void);

int main(void)
{
    unsigned int i;
    WDTCTL = WDTPW + WDTHOLD;                       // Stop watchdog timer
    SFRIE1 |= WDTIE;                                // enable WDT interrupt
    P1DIR = 0xff;
    P2DIR = 0xff;
    P1OUT = 0x0;
    P2OUT = LED;
    PM5CTL0 &= ~LOCKLPM5;
    __bis_SR_register(GIE);                         // Enable interrupts
    measure_count();                                // Establish baseline capacitance
    base_cnt = meas_cnt;
    for(i=15; i>0; i--)                             // Repeat and avg base measurement
    {
        measure_count();
        base_cnt = (meas_cnt+base_cnt)>>1;
    }
    /* Main loop starts here*/
    while (1)
    {
        pressBuffer = (pressBuffer << 1);           // Advance sliding window
        if (pressBuffer == 0)
            outputEnable = 0;
        measure_count();
        delta_cnt = base_cnt - meas_cnt;            // Calculate delta: c_change
        if (delta_cnt < 0)                          // If negative: result increased
        {                                           // beyond baseline, i.e. cap dec
            base_cnt = (base_cnt+meas_cnt) >> 1;    // Re-average quickly
            delta_cnt = 0;                          // Zero out for pos determination
        }
        else if (delta_cnt > KEY_LVL)               // Touch detected
        {
#ifdef USE_IDLEMODE
            idleCnt = 0;
#endif
            if ((pressBuffer & 0x0006) == 0x0006)   // Two previous slots had a press --> stop output and reset buffer
            {
                pressBuffer = 0;
                outputEnable = 0;
            }
            else
            {
                pressBuffer +=1;                    // push
                if (pressBuffer > 0x0F)
                    outputEnable = 1;
            }
        }
        else                                        // No Touch detected
        {
            base_cnt = base_cnt - 1;                // Account for capacitance drift
#ifdef USE_IDLEMODE
            if (idleCnt < IDLE_LIMIT)
                idleCnt++;
#endif
        }
        if (outputEnable == 1)
        {
            startPWM();
            P2OUT &= (~LED);
        }
        else
        {
            stopPWM();
            P2OUT |= LED;
        }
        RTCMOD = WAIT_PERIOD_FACTOR;                // Active mode: Interrupt and reset happen every 10*256*(1/10KHz) = ~0.25S
#ifdef USE_IDLEMODE
        if (idleCnt == IDLE_LIMIT)                  // Idle mode, sample every ~1.5 second to conserve power
            RTCMOD = IDLE_SAMPLE_PERIOD_FACTOR-1;   // Interrupt and reset happen every 59*256*(1/10KHz) = ~1.5S
#endif
        RTCCTL |= RTCSS__VLOCLK | RTCSR |RTCPS__256;
        RTCCTL |= RTCIE;
        __bis_SR_register(LPM4_bits);
    }
}                                                   // End Main

/* Measure count result (capacitance)*/
void measure_count(void)
{
    TB0CTL = TBSSEL_3 + MC_2;                       // INCLK, cont mode
    TB0CCTL1 = CM_3 + CCIS_2 + CAP;                 // Pos&Neg,GND,Cap
    /*Configure Ports for relaxation oscillator*/
    CAPTIOCTL |= CAPTURE_TOUCHPAD_3;                 //Only using touchpad #3, in future could create more elaborate functions using the other pads
    /*Setup Gate Timer*/
    WDTCTL = WDT_meas_setting;                      // WDT, ACLK, interval timer
    TB0CTL |= TBCLR;                                // Clear Timer_B TBR
    __bis_SR_register(LPM0_bits+GIE);               // Wait for WDT interrupt
    TB0CCTL1 ^= CCIS0;                              // Create SW capture of CCR1
    meas_cnt = TB0CCR1;                             // Save result
    WDTCTL = WDTPW + WDTHOLD;                       // Stop watchdog timer
    CAPTIOCTL = 0;
}
void startPWM(void)
{
    TB0CCR0 = 900-1;                                // PWM Freq 32.768kHz divided by TB0CCR0
    dutyIndex++;
    if (dutyIndex > 4)
        dutyIndex = 0;
    TB0CCR2 = dutyCounts[(uint8_t)(dutyIndex)]; // CCR2 PWM duty cycle TB0CCR2/TB0CCR0
    TB0CCTL2 = OUTMOD_7;                            // CCR2 reset/set
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;         // ACLK, up mode, clear TAR
    P1SEL1 |= MOTOR;                                // Configure Motor output to Timer B 0.2
    PM5CTL0 &= ~LOCKLPM5;
}
void stopPWM(void)
{
    TB0CTL = TBSSEL__ACLK | MC__STOP | TBCLR;       // ACLK, stop mode, clear TAR
    P1SEL1 &= ~(MOTOR);
}

/* Watchdog Timer interrupt service routine*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
    TB0CCTL1 ^= CCIS0;                              // Create SW capture of CCR1
    __bic_SR_register_on_exit(LPM4_bits);           // Exit LPM4 on reti
}

/*RTC ISR*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(RTC_VECTOR))) RTC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    TB0CCTL1 ^= CCIS0;                              // Create SW capture of CCR1
    RTCCTL = RTCSS__DISABLED;
    __bic_SR_register_on_exit(LPM4_bits);
}
