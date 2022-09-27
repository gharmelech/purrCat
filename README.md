# **purrCat**

This is a novelty PCB Art of a purring cat.

Feel free to use this project or parts of it at will, but please give credit when doing so

While this is an open source project, I am selling kits and assembled units for 10 and 15 USD respectivly.  
Please consider supporting us by [ordering here](https://forms.gle/wS7aRQJ3xb9KGsVB8).

## **Art**
The art was made by Taly Reznik - [@trollworks](https://www.instagram.com/trollworks/?fbclid=IwAR2XS3pt9mpKQIKOb-wqeVHk0iGZXp5bulvpvjSy-UH8qeWS0hPnVhlgy5U)  
Go check out her awesome work!

## **Hardware Description**
The project is based on TI's msp430fr2111 MCU. The current software could fit in (and easily compiled for) msp430fr2110 and could probably be further optimized to fit msp430fr2100, but going with the 2111 model leaves a lot of room for future expension with just a small price difference.

Purring is achived with a round radial vibration motor, which is low-side controlled with an nmos transistor.  
In addition, the eyes light up using revrese-mounted LEDs (might be worth while to change it to a side view LED in the future).

Petting is detected using capcitive touch.  
There are 4 touch pads on the PCB, but only 1 is in use in the current FW.

### **Some FW details**

#### **Picky Cat!**
I wanted to make the cat as realistic as possible, so it only likes petting between 0.5 and 4 Hz.  
(*Since I'm not doing swipe detection, it would also work with tapping. I might change it in a future version*)  
This is achived using a 2s sample buffer with 250ms slots. if the pad is touched for more than 3 slots in a row, the buffer zeros.  
Furthermore, purring only starts after at lest 1s

#### **Realistic Purring**
I have programmed a simple PWM function to make the vibration feel like real purring.  
It has dynamic change to the duty cycle, this creates the subtle ramp-up and ramp-down typical of a cat's purring.
I tried a sinusoidal PWM function, but it didn't have the right feel to it.

#### **Power Saving**
The FW has 2 destinctoperating modes (though not utlizing a state machine, that might be a worthwhile refactoring later on):
Active and Idle.

In Active mode the sampling window is every 250ms, this leads to current consumption of around 20uA (or 60uW at the nominal 3V)  
    --> only this mode leads to purring (during purring, the current consumption is about 20mA - 60mW at nonminal 3V).  
After 30 seconds without petting, we change to Idle mode.  
In this state, we sample every 1 second. if a touch is detected we move back to Active mode.  
In idle mode the current consumption is around 1.5uA (or 4.5uW at nominal 3V).

The final design uses a cr2032 lithium cell which is about 240mAh.  
Given the inefficiancies and the fact this board is over the current spec for this type of battery,  
I estimate a batterty life of roughly 8 hours of purring or about 6 years of idle.

#### **Programming and flashing**
The MCU is programmed using Code Composer Studio IDE.  
For software flashing I use a TI dev board LAUNCHPAD MSP-EXP430FR2433, which is the lowest cost programmer option available, though I assume you could make a flashing device out of an arduino.  
I left 3x2mm pitch pads for programming right near the MCU.  
They are arranged (from top to bottom) GND-SBWTCK-SBWTDIO, an additional connactio to VCC is required and could be obtained from the battery clip.

### **Parts List**
- MCU: msp430fr2111
- LED: 2x 0805 LED (reverse mounted, color of your choice)
- Capacitors: 2x0805 10uF MLCC (1 for the MCU and 1 for the Motor), 1x0603 100nF MLCC (for the MCU)
- Resistors: 3x0603 1k Ohm (current limiting for the transistor and the LEDs), 1x0603 100k Ohm (pulldown for the transistor, could be omitted as the driver pin is push-pull)
- Diode: 1x0603 schottky diode (I used Kyocera AVX SD0603S040S0R2)
- Transistor: 1xSOT3 NMOS Transistor (I used Nexperia NXV55UNR)
- Battery clip: 1x Keystone 3034 (cr2032 battery holder), note that not all battery holders can fit in the PCBs footprint)
- Vibration motor: 1x 10mm round radial vibration motor(or smaller), note that there are no pads for soldering it. one lead should be connected to vcc(I used the cap for it) and the other to the transistor drain pin (I made a bigger pad to accomidate this)
