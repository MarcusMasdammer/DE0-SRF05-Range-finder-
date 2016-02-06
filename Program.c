/*
 * 	This Program works with an NIOS processor, running on the ALTERA DE0 development board and
 * 	a SRF05 Ultrasonic Range Finder.
 * 	
 * 	When triggered a result in Centimetres(Max of 9.999) or Meters(Max of 5.200m) 
 * 	of the measured distance will be displayed on the DE0 development boards Seven segment displays.  
 *	
 *	When button 2 is pressed and Load switch is off(0) it resets any save slot that is on(1) as well as the
 *	recorded highest and lowest value. When Load switch is on(1) and button 2 is pressed the highest and lowest recorded values   
 *	
 *	<Switch Functions>
 *	Switch 0 = Constant Read(CR)	//When switch is on(1) constantly read distance.
 *	Switch 1 = CM or M(CM_M)		//When switch is off(0) display distance  in CM.MM, when switch is on(1) display distance M.MM.
 *									//note: saves for Meter values and CM values are independent and is toggled using this switch.
 *	Switch 2 = LED					//When switch is on(1) switch LEDs on.
 *	Switch 3 = Decimal Point(DP)	//When switch is on(1) move decimal point one segment to the right.
 *	Switch 4 = Load/Save(LOAD)		//When switch is on(1) Load state, when switch is off(0) Save/Reset state
 *	Switch 5 = Save slot 1(SV_1)	//When switch is on(1) Save state: Save the value taken with button 1 to save slot 1
 *									//reset to 0 if button 2 is pressed. Load State: displays the value saved to slot 1 when button 1 is pressed.
 *	Switch 6 = Save slot 2(SV_2)	//When switch is on(1) Save state: Save the value taken with button 1 to save slot 2
 *									//reset to 0 if button 2 is pressed. Load State: displays the value saved to slot 2 when button 1 is pressed.
 *	Switch 7 = Save slot 3(SV_3)	//When switch is on(1) Save state: Save the value taken with button 1 to save slot 3
 *									//reset to 0 if button 2 is pressed. Load State: displays the value saved to slot 3 when button 1 is pressed.
 *	Switch 8 = Save slot 4(SV_4)	//When switch is on(1) Save state: Save the value taken with button 1 to save slot 4
 *									//reset to 0 if button 2 is pressed. Load State: displays the value saved to slot 4 when button 1 is pressed.
 *	Switch 9 = Save slot 5(SV_5)	//When switch is on(1) Save state: Save the value taken with button 1 to save slot 5
 *									//reset to 0 if button 2 is pressed. Load State: displays the value saved to slot 5 when button 1 is pressed.		
 *	<END>>>	
 *	
 *	Development Environment: ALTERA NIOS II Software on Quartus II version 13.0(Wed Edition)
 *	Hardware: SRF05 Ultrasonic Range Finder, DE0 FPGA Development Board, DE0 Breadboard extension
 *	
 *	@author Marcus Masdammer <masdammer130@hotmail.co.uk>
 *	@copyright 2016
 *	@license GNU GENERAL PUBLIC LICENSE version 2
 *	@Version 1.0.0 		
 */

#include "sys/alt_stdio.h"   			//for the alt_putstr function below.  Outputs to Eclipse console
#include "altera_avalon_pio_regs.h"  	//for the I/O functions in the while loop below
#include "sys/alt_timestamp.h"  		//Timestamp Driver
#include "system.h"

#define setHeaderOuts HEADEROUTPUTS_BASE+0x10  	//HEADEROUTPUTS_BASE is defined in system.h of the _bsp file.  It refers to the base address in the Qsys design
												//the hex offset (in this case 0x10, which is 16 in decimal) gives the number of bytes of offset
												//each register is 32 bits, or 4 bytes
												//so to shift to register 4, which is the outset register, we need 4 * (4 bytes) = 16 bytes
#define clearHeaderOuts HEADEROUTPUTS_BASE+0x14 //to shift to register 5 (the 'outclear' register) we need to shift by 5 * (4 bytes) = 20 bytes, (=0x14 bytes)
												// offset of 5 corresponds to the 'outclear' register of the PIO.

int dig(int counter);							//Defines the dig function
int distance_get(int CM_M,int DP, int LED);		//Defines the Distance Get function
int sseg(int distance,int DP);					//Defines the SSEG function
void timer(int secs);							//Defines the timer function

/*################################################################*/

int  main(void)
{
	alt_putstr("Project 1: CM & M Distance Measurement - Marcus Masdammer");

	int buttons; 								//for button states on the DE0 board
	int distance;								//for distance that was calculated
	int settings;								//for switch settings
	int disp;									//for saving the SSEG display value

	int CR;										//Saves the On or Off state of constant read on SW0
	int CM_M;									//Saves the CM or M state on SW1
	int LED;									//Saves the On or Off state of LED on SW2
	int DP;										//Saves the On or Off state of Decimal point on SW3
	int LOAD;									//Saves the Load or Read state on SW4
	int SV_1;									//Saves the On or Off state of save 1 on SW5
	int SV_2;									//Saves the On or Off state of save 2 on SW6
	int SV_3;									//Saves the On or Off state of save 3 on SW7
	int SV_4;									//Saves the On or Off state of save 4 on SW8
	int SV_5;									//Saves the On or Off state of save 5 on SW9

	int hi = 0;									//Highest value
	int low = 10000;							//Lowest value

	int save1 = 0xffffffff;						//Default of Save 1 as blank
	int save2 = 0xffffffff;						//Default of Save 2 as blank
	int save3 = 0xffffffff;						//Default of Save 3 as blank
	int save4 = 0xffffffff;						//Default of Save 4 as blank
	int save5 = 0xffffffff;						//Default of Save 5 as blank

	int save1_M = 0xffffffff;					//Default of Save 1 Meter as blank
	int save2_M = 0xffffffff;					//Default of Save 2 Meter as blank
	int save3_M = 0xffffffff;					//Default of Save 3 Meter as blank
	int save4_M = 0xffffffff;					//Default of Save 4 Meter as blank
	int save5_M = 0xffffffff;					//Default of Save 5 Meter as blank

	while(1)//Infinite loop
	{
		buttons=IORD_ALTERA_AVALON_PIO_DATA(PUSHBUTTONS1_2_BASE);		//Read button states

		while((buttons & 0x01)+(buttons & 0x02)==3)						//While none of the buttons are pressed
		{
			buttons=IORD_ALTERA_AVALON_PIO_DATA(PUSHBUTTONS1_2_BASE);	//Read button states
		}

		settings=IORD_ALTERA_AVALON_PIO_DATA(DE0SWITCHES_BASE);			//Read Switch states

		CR = ((settings) - ((settings >> 1) << 1));						//Read Constant read switch state
		CM_M = ((settings >> 1) - ((settings >> 2) << 1));				//Read CM or M switch state
		LED = ((settings >> 2) - ((settings >> 3) << 1));				//Read LED switch state
		DP = ((settings >> 3) - ((settings >> 4) << 1));				//Read Decimal Point switch state
		LOAD = ((settings >> 4) - ((settings >> 5) << 1));				//Read Load switch state
		SV_1 = ((settings >> 5) - ((settings >> 6) << 1));				//Read Save 1 switch state
		SV_2 = ((settings >> 6) - ((settings >> 7) << 1));				//Read Save 2 switch state
		SV_3 = ((settings >> 7) - ((settings >> 8) << 1));				//Read Save 3 switch state
		SV_4 = ((settings >> 8) - ((settings >> 9) << 1));				//Read Save 4 switch state
		SV_5 = ((settings >> 9) - ((settings >> 10) << 1));				//Read Save 5 switch state

		if (LOAD == 0)					//If Load switch is off
		{
			if(buttons == 2)			//If button 1 was pressed
			{
				while(CR == 1)			//While Constant Read Switch is On
				{
					settings=IORD_ALTERA_AVALON_PIO_DATA(DE0SWITCHES_BASE);	//Read Switch states

					CR = ((settings) - ((settings >> 1) << 1));				//Read Constant read switch state
					CM_M = ((settings >> 1) - ((settings >> 2) << 1));		//Read CM or M switch state
					LED = ((settings >> 2) - ((settings >> 3) << 1));		//Read LED switch state
					DP = ((settings >> 3) - ((settings >> 4) << 1));		//Read Decimal Point switch state

					distance = distance_get(CM_M,DP,LED);					//Run Distance Get function and save the return as distance
					if (distance < 10000)									//If distance return is less than 10000
					{
						IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,sseg(distance,DP));	//Display the distance on the SSEG Display

						if((CM_M == 0) & (DP == 0))									//If CM_M and DP switches are Off
						{
							if(distance > hi)										//If Distance is higher than saved hi value
							{
								hi = distance;										//Save the new hi value
							}
							else if(distance < low)									//Else If Distance is lower than saved low value
							{
								low = distance;										//Save the new Low Value
							}
						}
					}
					else
					{
						IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x86Af2fff);			//If greater than 10000 display Err.
					}
				}

				distance = distance_get(CM_M,DP,LED);								//Run Distance Get function and save the return as distance
				if (distance < 10000)												//If distance return is less than 10000
				{
					disp = sseg(distance,DP);										//Display is the return of SSEG function of supplied distance and decimal point
					IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,disp);					//Display the value using disp(return of SSEG function)

					if((DP == 0) & (CM_M == 0))										//If CM_M and DP switches are Off
					{
						if(distance > hi)											//If Distance is higher than saved hi value
						{
							hi = distance;											//Save the new hi value
						}
						else if(distance < low)										//Else If Distance is lower than saved low value
						{
							low = distance;											//Save the new Low Value
						}
					}

				}
				else
				{
					IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x86Af2fff);				//If greater than 10000 display Err.
				}

				if(CM_M == 0)							//If CM mode
				{
					if(SV_1 == 1)						//If Save 1(SW5) is On
					{
						save1 = disp;					//Save the Display to Save state 1
					}
					if(SV_2 == 1)						//If Save 2(SW6) is On
					{
						save2 = disp;					//Save the Display to Save state 2
					}
					if(SV_3 == 1)						//If Save 3(SW7) is On
					{
						save3 = disp;					//Save the Display to Save state 3
					}
					if(SV_4 == 1)						//If Save 4(SW8) is On
					{
						save4 = disp;					//Save the Display to Save state 4
					}
					if(SV_5 == 1)						//If Save 5(SW9) is On
					{
						save5 = disp;					//Save the Display to Save state 5
					}
				}
				else									//Else M mode
				{
					if(SV_1 == 1)						//If Save 1(SW5) is On
					{
						save1_M = disp;					//Save the Display to Save state 1 Meters
					}
					if(SV_2 == 1)						//If Save 2(SW6) is On
					{
						save2_M = disp;					//Save the Display to Save state 2 Meters
					}
					if(SV_3 == 1)						//If Save 3(SW7) is On
					{
						save3_M = disp;					//Save the Display to Save state 3 Meters
					}
					if(SV_4 == 1)						//If Save 4(SW8) is On
					{
						save4_M = disp;					//Save the Display to Save state 4 Meters
					}
					if(SV_5 == 1)						//If Save 5(SW9) is On
					{
						save5_M = disp;					//Save the Display to Save state 5 Meters
					}
				}
			}
			else if(buttons == 1)				//If Button 2 is pressed Reset selected saves and High + Low Values
			{
				hi = 0;							//Hi value reset to 0
				low = 10000 ;					//Low Value reset to 10000

				if(CM_M == 0)					//Reset CM values
				{
					if(SV_1 == 1)				//If Save 1(SW5) is On
					{
						save1 = 0xffffffff;		//Reset Save state 1
					}

					if(SV_2 == 1)				//If Save 2(SW6) is On
					{
						save2 = 0xffffffff;		//Reset Save state 2
					}

					if(SV_3 == 1)				//If Save 3(SW7) is On
					{
						save3 = 0xffffffff;		//Reset Save state 3
					}

					if(SV_4 == 1)				//If Save 4(SW8) is On
					{
						save4 = 0xffffffff;		//Reset Save state 4
					}

					if(SV_5 == 1)				//If Save 5(SW9) is On
					{
						save5 = 0xffffffff;		//Reset Save state 5
					}
				}
				else
				{
					if(SV_1 == 1)						//If Save 1(SW5) is On
					{
						save1_M = 0xffffffff;			//Reset Save state 1 Meters
					}
					if(SV_2 == 1)						//If Save 2(SW6) is On
					{
						save2_M = 0xffffffff;			//Reset Save state 2 Meters
					}
					if(SV_3 == 1)						//If Save 3(SW7) is On
					{
						save3_M = 0xffffffff;			//Reset Save state 3 Meters
					}
					if(SV_4 == 1)						//If Save 4(SW8) is On
					{
						save4_M = 0xffffffff;			//Reset Save state 4 Meters
					}
					if(SV_5 == 1)						//If Save 5(SW9) is On
					{
						save5_M = 0xffffffff;			//Reset Save state 5 Meters
					}
				}
			}

		}
		else if(LOAD == 1)								//Else If Load Switch is 1
		{
			if(buttons == 2)							//If button 1 was pressed
			{
					if(CM_M == 0)
					{

						IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0xC606ffff);				//Display CE on SSEG display
						timer(2);														//wait 2 seconds

						if(SV_1 == 1)													//If Save 1 (SW5) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x12F9ffff);			//Display SV1 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save1);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_2 == 1)													//If Save 2 (SW6) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x12A4ffff);			//Display SV2 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save2);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_3 == 1)													//If Save 3 (SW7) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x12B0ffff);			//Display SV3 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save3);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_4 == 1)													//If Save 4 (SW8) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x1299ffff);			//Display SV4 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save4);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_5 == 1)													//If Save 4 (SW8) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x1292ffff);			//Display SV5 on SSEG display
							timer(2); 													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save5);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}
					}
					else
					{
						IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x2BC606ff);				//Display nCE on SSEG display
						timer(2);														//Wait 2 seconds

						if(SV_1 == 1)													//If Save 1 (SW5) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x12F9ffff);			//Display SV1 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save1_M);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_2 == 1)													//If Save 2 (SW6) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x12A4ffff);			//Display SV2 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save2_M);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_3 == 1)													//If Save 3 (SW7) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x12B0ffff);			//Display SV3 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save3_M);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_4 == 1)													//If Save 4 (SW8) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x1299ffff);			//Display SV4 on SSEG display
							timer(2);													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save4_M);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

						if(SV_5 == 1)													//If Save 4 (SW8) is On
						{
							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x1292ffff);			//Display SV5 on SSEG display
							timer(2); 													//Wait 2 seconds

							IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,save5_M);				//Display the saved value on SSEG display
							timer(3);													//Wait 3 seconds
						}

					}
				}
				else if(buttons == 1)														//Else if Button 2 is pressed on the DE0 Board
				{
					//Displaying the Highest value ######################
					IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0x8B6FFFFF);						//Display hi on the SSEG display
					timer(2);																//Wait 2 seconds
					IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,sseg(hi,DP));						//Display the Hi value on the SSEG display
					timer(3);																//Wait 3 seconds
					//###################################################

					//Displaying the Lowest value #######################
					IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,0xC740FFFF);						//Display LO on the SSEG display
					timer(2);																//Wait 2 seconds
					IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,sseg(low,DP));					//Display the Low value on the SSEG display
					timer(3);																//Wait 3 seconds
					//###################################################
				}
			}

			while((buttons & 0x01)+(buttons & 0x02) != 3) 									//While any of the two buttons are still pressed
			{
				buttons=IORD_ALTERA_AVALON_PIO_DATA(PUSHBUTTONS1_2_BASE);					//Read button state
			}
	}
}

//############################################################################################################################################


//Timer function to run for supplied amount of seconds #########
void timer(int sec)
{
	int TS = 50000000;						//Ticks per second
	int time = 0;							//Time reset

	time=alt_timestamp_start();				//Start Timer
	while(time <= (TS*sec))					//While Timer value is lower than Ticks x required seconds
	{
		time=alt_timestamp();				//read tick amount
	}
	return;									//return to line that called this function
}
//###############################################################################################################################################


//This Function uses the SRF05 Ultrasonic Rangefinder to Calculate distance
int distance_get(int CM_M,int DP, int LED)
{
	int timer = 0;											//Define a Variable for the timers
	char input = 0;											//Defines the input variable
	float constant = 0;										//Defines the constant used to calculate range

	IOWR_ALTERA_AVALON_PIO_DATA(clearHeaderOuts,0x01);		//Turns off output pin 1

	if(DP == 1)												//If DP that was supplied is 1
		{
			constant = 0.34029/10;							//our constant will be divided by 10
		}
	else
		{
			constant = 0.34029;								//else stays the same
		}

	if(CM_M == 1)											//If CM_M value that was supplied is 1
		{
			constant = constant/100;						//Constant will be divided by 100 else do nothing
		}

	timer=alt_timestamp_start();							//Start timer


	while(timer < 2500000) 									//While timer is less than 50ms
	{
		timer=alt_timestamp();								//Read timer
	}

	IOWR_ALTERA_AVALON_PIO_DATA(setHeaderOuts,0x01);		//Turn on Output Signal of pin 1

	timer=alt_timestamp_start();							//Start Timer

	while(timer < 501)										//Wait for 501 ticks(Time for signal to reach the SRF05)
	{
		timer = alt_timestamp();
	}

	IOWR_ALTERA_AVALON_PIO_DATA(clearHeaderOuts,0x01);		//Turn Off output signal on pin 1

	input=IORD_ALTERA_AVALON_PIO_DATA(HEADERINPUTS_BASE);	//Input pins state saved to input

	while(((input)&(0x01)) == 0)							//Wait for no signals
	{
	  input=IORD_ALTERA_AVALON_PIO_DATA(HEADERINPUTS_BASE); //Input pins state saved to input
	}

	timer = alt_timestamp_start();							//Start timer

	input=IORD_ALTERA_AVALON_PIO_DATA(HEADERINPUTS_BASE);	//Read input Pins

	while(((input)&(0x01)) == 1 )							//While no input signal on input pin 1
	{
	  input=IORD_ALTERA_AVALON_PIO_DATA(HEADERINPUTS_BASE); //Read input Pins
	}

	timer = alt_timestamp();								//Save current timer value

	float dis = timer*constant;								//Distance = Timer x Constant

	if(LED == 1)											//If LED setting is On
	{

		if(dis/1000 <= 1)										//If Distance/1000 <= 1
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x000);	//NO LEDs are Lit
		}
		else if(dis/1000 <= 2)									//Else if Distance/1000 <= 2
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x001);	//LED 0 is lit
		}
		else if(dis/1000 <= 3)									//Else if Distance/1000 <= 3
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x003);	//LED 0:1 is lit
		}
		else if(dis/1000 <= 4)									//Else if Distance/1000 <= 4
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x007);	//LED 0:2 is lit
		}
		else if(dis/1000 <= 5)									//Else if Distance/1000 <= 5
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x00F);	//LED 0:3 is lit
		}
		else if(dis/1000 <= 6)									//Else if Distance/1000 <= 6
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x01F);	//LED 0:4 is lit
		}
		else if(dis/1000 <= 7)									//Else if Distance/1000 <= 7
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x03F);	//LED 0:5 is lit
		}
		else if(dis/1000 <= 8)									//Else if Distance/1000 <= 8
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x07F);	//LED 0:6 is lit
		}
		else if(dis/1000 <= 9)									//Else if Distance/1000 <= 9
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x0FF);	//LED 0:7 is lit
		}
		else if(dis/1000 < 10)									//Else if Distance/1000 < 10
		{
			IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0xfff);	//LED 0:8 is lit
		}
	}
	else
	{
		IOWR_ALTERA_AVALON_PIO_DATA(DE0_LEDS_BASE,0x000);		//All LEDs Off
	}
	return dis;													//return to line that called this function
}

//######################################################################################################################

//Returns the hexidecimal value of supplied number for SSEG display
int dig(int counter)
{
    int digit;				//The Variable that will be returned

	if (counter == 0)		//If supplied number is 0
	{
		digit = 0xC0;		//SSEG value for 0
	}
	else if(counter == 1)	//If supplied number is 1
	{
		digit = 0xF9;		//SSEG value for 1
	}
	else if(counter == 2)	//If supplied number is 2
	{
		digit = 0xA4;		//SSEG value for 2
	}
	else if(counter == 3)	//If supplied number is 3
	{
		digit = 0xB0;		//SSEG value for 3
	}
	else if(counter == 4)	//If supplied number is 4
	{
		digit = 0x99;		//SSEG value for 4
	}
	else if(counter == 5)	//If supplied number is 5
	{
		digit = 0x92;		//SSEG value for 5
	}
	else if(counter == 6)	//If supplied number is 6
	{
		digit = 0x82;		//SSEG value for 6
	}
	else if(counter == 7)	//If supplied number is 7
	{
		digit = 0xF8;		//SSEG value for 7
	}
	else if(counter == 8)	//If supplied number is 8
	{
		digit = 0x80;		//SSEG value for 8
	}
	else if(counter == 9)	//If supplied number is 9
	{
		digit = 0x98;		//SSEG value for 9
	}
	else
	{
		digit = 0xC0;		//Else SSEG value for 0
	}
	return digit;			//Return the hex value of the digit
}

//################################################################################################################

//This function generates the display for the SSEG(7-segment) display
int sseg(int distance, int DP)
{
	//These variable saves the individual digits
	 int seg1 = dig(distance %10);			//Right most digit
	 int seg2 = dig((distance/10)%10);		//Following digit
	 int seg3 = dig((distance/100)%10);		//Following digit
	 int seg4 = dig((distance/1000)%10);	//Last digit

	 int sseg;								//the Variable that will be passed back to the line variable

	 if(DP == 0)							//If Decimal point switch is Off
	 {
	 sseg = seg4 - 128;						//Add a decimal point to the Left most segment
	 sseg = sseg << 8;						//shift over 8-bits
	 sseg = sseg + seg3;					//Add the next digit
	 }
	 else
	 {
		 sseg = seg4;						//Else
		 sseg = sseg << 8;
		 seg3 = seg3 - 128;					//Decimal point is added to the next 7-segment display
		 sseg = sseg + seg3;
	 }

	 sseg = sseg << 8;						//Shift over 8-bits
	 sseg = sseg + seg2;					//Add Segment 2 so that it can be displayed
	 sseg = sseg << 8;						//Shift over 8-bits
	 sseg = sseg + seg1;					//Add final segment

	return sseg;							//Return the entire display
}
//#########################################################################################################################
