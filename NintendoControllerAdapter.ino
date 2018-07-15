//AUTHOR: Jacob Arsenault
//PROJECT: Nintendo Controller Adapter
//VERSION: 0.1
//DATE: July 12, 2018
//
//
//DESCRIPTION
/*
 *A basic inteface for NES, SNES and N64 controllers (GC to come).  Reports  1-3 controllers as a HID compliant device.
*/
//REFERENCES
/* 
 *Joystick Library by Matthew Heironimus.
*/

/*
TODO Find better way to probe for controllers. In current implementation controllers will not be connected if the 'A' button is held down during read sequence.
TODO Setup function to read n64 controller data
TODO Implement hotswap funcionality to controllers via RESET flag
*/


#include "Joystick.h"

//Define constants
#define BC_NES 8	//NES button count
#define BC_SNES 12	//SNES button count
#define BC_N64 13	//N64 button count
#define X_N64 4		//N64 X axis bit size
#define Y_N64 4 	//N64 Y axis bit size
#define DATA_NES 2	//NES data pin
#define DATA_SNES 3	//SNES data pin
#define DATA_N64 4	//N64 data pin
#define LATCH 5		//Common controller latch signal pin
#define CLOCK 6		//Common controller clock signal pin

//Setup global variables
bool BOOTUP = 1; 	//Bootup flag
bool RESET = 0;		//Reset flag
byte ACTIVE_CONTROLLERS = 0x00;	//Bits 1-3 describe active status of NES/SNES/N64 controllers

//Declare 3 joystick objects
Joystick_ nesCtrl(0x03, JOYSTICK_TYPE_GAMEPAD, BC_NES, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ snesCtrl(0x03, JOYSTICK_TYPE_GAMEPAD, BC_SNES, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ n64Ctrl(0x03, JOYSTICK_TYPE_GAMEPAD, BC_N64, 0, true, true, false, false, false, false, false, false, false, false, false);

void connectControllers(void)
{
//Pulse the latch pin on each controller and listen for response
//Upon response, set active controller bits and initialize joysticks accordingly
	digitalWrite(LATCH, 1);
	digitalWrite(LATCH, 0);
	if (digitalRead(DATA_NES))
	{
		bitSet(ACTIVE_CONTROLLERS, 1);
		nesCtrl.begin(0);
	}
	if (digitalRead(DATA_SNES))
	{	
		bitSet(ACTIVE_CONTROLLERS, 2);
		snesCtrl.begin(0);
	
	}
	if (digitalRead(DATA_N64))
	{
		bitSet(ACTIVE_CONTROLLERS, 3);
		n64Ctrl.begin(0);
	}
}

void disconnectControllers(void)
{
//Disconnect all controllers and return active controller byte to 0x00 (inactive)
	if (bitRead(ACTIVE_CONTROLLERS, 1)==1)
	{
		nesCtrl.end(0);
	}
	if (bitRead(ACTIVE_CONTROLLERS, 2)==1)
	{
		snesCtrl.end(0);
	}	
	if (bitRead(ACTIVE_CONTROLLERS, 3)==1)
	{
		n64Ctrl.end(0);
	}
	ACTIVE_CONTROLLERS = 0x00;
}

void nesRead(bool FLAG)
{
//If flag is true, controller is inactive and will exit
	if(FLAG)
		return;

	int buttons[BC_NES] = {0};
	int previousState[BC_NES] = {0};
	int COUNT = 0;

// Latch current button status on controllers
	digitalWrite(LATCH, 1);
	digitalWrite(LATCH, 0);
	buttons[COUNT] = !digitalRead(DATA_NES);
// Pulse the clock then read the remaining button data  
	for (COUNT=1; COUNT<BC_NES; COUNT++)
	{
		digitalWrite(CLOCK, 1);
		digitalWrite(CLOCK, 0);
		buttons[COUNT] = !digitalRead(DATA_NES);
	}
// Compare buttons states and fill button report  
	for (COUNT=0; COUNT<BC_NES; COUNT++)
	{
		if (buttons[COUNT] != previousState[COUNT]){
			nesCtrl.setButton(COUNT, buttons[COUNT]); 
			previousState[COUNT] = buttons[COUNT];
		}
	}
// Send gamepad report
	nesCtrl.sendState();
}

void snesRead(bool FLAG)
{
//If flag is true, controller is inactive and will exit
	if(FLAG)
		return;

	int buttons[BC_SNES] = {0};
	int previousState[BC_SNES] = {0};
	int COUNT = 0;

// Latch current button status on controllers
	digitalWrite(LATCH, 1);
	digitalWrite(LATCH, 0);
	buttons[COUNT] = !digitalRead(DATA_SNES);
// Pulse the clock then read the remaining button data  
	for (COUNT=1; COUNT<BC_SNES; COUNT++)
	{
		digitalWrite(CLOCK, 1);
		digitalWrite(CLOCK, 0);
		buttons[COUNT] = !digitalRead(DATA_SNES);
	}
// Compare buttons states and fill button report  
	for (COUNT=0; COUNT<BC_SNES; COUNT++)
	{
		if (buttons[COUNT] != previousState[COUNT]){
			snesCtrl.setButton(COUNT, buttons[COUNT]); 
			previousState[COUNT] = buttons[COUNT];
		}
	}
// Send gamepad report
	snesCtrl.sendState();
}

void n64Read(bool FLAG)
{
// If flag is true, controller is inactive and will exit
	if (FLAG)
		return;

// Send gamepad report
	n64Ctrl.sendState();
}

void setup() 
{
//Initialize gpio pin modes
    if (BOOTUP)
    {
        pinMode(DATA_NES, INPUT);
        pinMode(DATA_SNES, INPUT);
        pinMode(DATA_N64, INPUT);
        pinMode(CLOCK, OUTPUT);
        pinMode(LATCH, OUTPUT);
        digitalWrite(CLOCK, 0);
        digitalWrite(LATCH, 0);
        BOOTUP = 0;   //Turn off boot flag
    }
    
// Refresh connection to controllers
   	disconnectControllers();
	connectControllers();

}

void loop()
{
// If reset flag is set, turn flag off and run setup again
	if (RESET)
	{	
		RESET = 0;
		setup();	
	}
// Read active controllers
	nesRead(bitRead(ACTIVE_CONTROLLERS, 1));
	snesRead(bitRead(ACTIVE_CONTROLLERS, 2));
	n64Read(bitRead(ACTIVE_CONTROLLERS, 3));
// Delay to run at roughly 60Hz
	delay(15);
}


