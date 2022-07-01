/* --COPYRIGHT--,BSD_EX
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
/* Defines WDT SMCLK interval for sensor measurements*/
#define WDT_meas_setting (DIV_SMCLK_512)
/* Defines WDT ACLK interval for delay between measurement cycles*/
#define WDT_delay_setting (DIV_ACLK_8192)
/* Sensor settings*/
#define KEY_LVL     750                             // Defines threshold for a key press
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

#define LED     BIT0                              // P2.0 LED output
#define MOTOR   BIT7                              // P1.7 Vibration motor output

// Global variables for sensing
unsigned int base_cnt, meas_cnt;
int delta_cnt;
volatile uint16_t pressBuffer = 0;
uint8_t outputEnable = 0;
//uint8_t dutyCounts [7] = {175, 200, 200, 250, 250, 200, 200};
uint8_t dutyCounts [5] = {200, 200, 255, 255, 255};
uint8_t dutyIndex = 0;

/* System Routines*/
void measure_count(void);                           // Measures each capacitive sensor
void startPWM(void);
void stopPWM(void);

int main(void)
{
    unsigned int i;
    WDTCTL = WDTPW + WDTHOLD;                       // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    SFRIE1 |= WDTIE;                                // enable WDT interrupt
    P1DIR = MOTOR;                                  // P1.7 = LED
    P2DIR = LED;                                    // P2.0 = LED
    P1OUT = 0;
    P2OUT = 0;
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
        pressBuffer = (pressBuffer << 1);
        if (pressBuffer < 5)
            outputEnable = 0;
        measure_count();                         // Measure all sensors
        delta_cnt = base_cnt - meas_cnt;         // Calculate delta: c_change
        if (delta_cnt < 0)                       // If negative: result increased
        {                                        // beyond baseline, i.e. cap dec
            base_cnt = (base_cnt+meas_cnt) >> 1; // Re-average quickly
            delta_cnt = 0;                       // Zero out for pos determination
        }
        else if (delta_cnt > KEY_LVL)            // Determine if each key is pressed
        {
            if ((pressBuffer & 0x0006) == 0x0006)    //two previous slots had a press - stop output and reset frame
            {
                pressBuffer = 1;
                outputEnable = 0;
            }
            else
            {
                pressBuffer +=1;                //push
                if (pressBuffer > 0x000F)
                    outputEnable = 1;
//                    P1OUT |= MOTOR;             // key pressed
//                    startPWM();
            }
        }
        /* Handle baseline measurment for a base C increase*/
        else                                    // Only adjust baseline down
        {                                       // if no keys are touched
            base_cnt = base_cnt - 1;            // Adjust baseline down, should be
        }                                       // slow to accomodate for genuine
        if (outputEnable == 1)
            startPWM();
        else
            stopPWM();
        /* Delay to next sample */
        WDTCTL = WDT_delay_setting;                 // WDT, ACLK, interval timer
        __bis_SR_register(LPM3_bits);
    }
}                                                   // End Main

/* Measure count result (capacitance) of each sensor*/
void measure_count(void)
{
//    if (outputEnable == 1)
//        stopPWM();
    TB0CTL = TBSSEL_3 + MC_2;                       // INCLK, cont mode
    TB0CCTL1 = CM_3 + CCIS_2 + CAP;                 // Pos&Neg,GND,Cap
    /*Configure Ports for relaxation oscillator*/
    CAPTIOCTL |= CAPTIOEN + CAPTIOPOSEL0 + CAPTIOPISEL_4; //P1.1
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
    TB0CCR0 = 800-1;                        // PWM Freq (32.768kHz divided by TB0CCR0)
    TB0CCR2 = dutyCounts[dutyIndex++ % 5];                           // CCR2 PWM duty cycle (TB0CCR2/TB0CCR0)
    TB0CCTL2 = OUTMOD_7;                     // CCR2 reset/set
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;  // ACLK, up mode, clear TAR
    P1SEL1 |= MOTOR;
    PM5CTL0 &= ~LOCKLPM5;
}
void stopPWM(void)
{
    TB0CTL = TBSSEL__ACLK | MC__STOP | TBCLR;  // ACLK, stop mode, clear TAR
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
    __bic_SR_register_on_exit(LPM3_bits);           // Exit LPM3 on reti
}
