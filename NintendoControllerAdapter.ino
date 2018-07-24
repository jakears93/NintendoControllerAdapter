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
//DEPENDENCIES
/* 
https://github.com/MHeironimus/ArduinoJoystickLibrary 
delay_x.h
*/

/*
TODO Find better way to probe for controllers. In current implementation controllers will not be connected if the 'A' button is held down during read sequence.
TODO Implement timers in main loop and n64 data reads to accurately read signals and send reports at equal intervals
*/


#include "Joystick.h"

//Define constants
#define BC_NES 8	//NES button count
#define BC_SNES 12	//SNES button count
#define BC_N64 14	//N64 button count
#define XAXIS_N64 4	//N64 X axis bit size
#define YAXIS_N64 4 	//N64 Y axis bit size
#define DATA_NES_PIN 2	//NES data pin
#define DATA_SNES_PIN 3	//SNES data pin
#define DATA_N64_PIN 4	//N64 data pin
#define LATCH_PIN 5	//Common controller latch signal pin
#define CLOCK_PIN 6	//Common controller clock signal pin
#define RESET_PIN 0	//Reset flag pin
#define SIZE_OF_N64_SIGNAL 32 //Number of bits in the N64 data response

//Setup global variables
bool BOOTUP = 1;	 	//Bootup flag
bool RESET = 0;			//Reset flag
byte ACTIVE_CONTROLLERS = 0x00;	//Bits 1-3 describe active status of NES/SNES/N64 controllers
bool N64InputSignal[SIZE_OF_N64_SIGNAL] = {0};      //N64 controller data
  
//Declare 3 joystick objects
Joystick_ nesCtrl(0x03, JOYSTICK_TYPE_GAMEPAD, BC_NES, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ snesCtrl(0x03, JOYSTICK_TYPE_GAMEPAD, BC_SNES, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ n64Ctrl(0x03, JOYSTICK_TYPE_MULTI_AXIS, BC_N64, 0, true, true, false, false, false, false, false, false, false, false, false);

//Define functions
byte bitsToByte(bool bits[SIZE_OF_N64_SIGNAL], int index)
{
  byte value = 0x00;
  int COUNT = 0;
  for(COUNT; COUNT<8; COUNT++)
  {
    if(bits[index])
    {
      bitSet(value, COUNT);
    }
    index++;
  }
  return value*4;   // Scaled x4 for because joystick library range is set to 10bit resolution
}

void N64_SIGNAL_HIGH() __attribute__((always_inline));
void N64_SIGNAL_HIGH()
{
 _delay_us(1);          //Hold data low for 1 microseconds   
 PORTD ^= B00000100;    //Toggle N64 data high
 _delay_us(3);          //Hold data high for 3 microseconds
 PORTD ^= B00000100;    //Toggle N64 data low
}

void N64_SIGNAL_LOW() __attribute__((always_inline));
void N64_SIGNAL_LOW()
{
 _delay_us(3);          //Hold data low for 3 microseconds   
 PORTD ^= B00000100;    //Toggle N64 data high
 _delay_us(1);          //Hold data high for 1 microseconds
 PORTD ^= B00000100;    //Toggle N64 data low
}

void connectControllers(void)
{
//Initialize variable to store response from N64 controller
  bool response = 0;
//Pulse the latch pin on each controller and listen for response
//Upon response, set active controller bits and initialize joysticks accordingly
	digitalWrite(LATCH_PIN, 1);
	digitalWrite(LATCH_PIN, 0);
	if (digitalRead(DATA_NES_PIN)) 		//Fails if A button is pressed during latch
	{
		bitSet(ACTIVE_CONTROLLERS, 1);	
		nesCtrl.begin(0);
	}
	if (digitalRead(DATA_SNES_PIN))		//Fails if A button is pressed during latch
	{	
		bitSet(ACTIVE_CONTROLLERS, 2);
		snesCtrl.begin(0);
	}
 
   // Send intial polling sequence of 9 bits to N64 Controller(0b000000011)
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_HIGH();
  N64_SIGNAL_HIGH();

  DDRD ^= B00000100;      //Toggles N64 data pin to input via direction register

  //changed to 0us to account for time to of toggling pins (toggles 2 times per signal, x 9 signals = 18 + 1 to toggle to input = 19 clock cycles or 19/16 us)
  _delay_us(3);
  response = (PORTD & B00000100);   // Reads and stores only N64 data pin value by using logical AND expression
  DDRD ^= B00000100;      //Toggles N64 data pin to output via direction register
	if (response)		
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
		nesCtrl.end();
	}
	if (bitRead(ACTIVE_CONTROLLERS, 2)==1)
	{
		snesCtrl.end();
	}	
	if (bitRead(ACTIVE_CONTROLLERS, 3)==1)
	{
		n64Ctrl.end();
	}
	ACTIVE_CONTROLLERS = 0x00;
}

void nesRead(bool FLAG)
{
//If flag is true, controller is inactive and will exit
	if(FLAG)
		return;

	bool buttons[BC_NES] = {0};
	bool previousState[BC_NES] = {0};
	int COUNT = 0;

// Latch current button status on controllers
	digitalWrite(LATCH_PIN, 1);
	digitalWrite(LATCH_PIN, 0);
	buttons[COUNT] = !digitalRead(DATA_NES_PIN);
// Pulse the clock then read the remaining button data  
	for (COUNT=1; COUNT<BC_NES; COUNT++)
	{
		digitalWrite(CLOCK_PIN, 1);
		digitalWrite(CLOCK_PIN, 0);
		buttons[COUNT] = !digitalRead(DATA_NES_PIN);
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
	bool buttons[BC_SNES] = {0};
	bool previousState[BC_SNES] = {0};
	int COUNT = 0;
// Latch current button status on controllers
	digitalWrite(LATCH_PIN, 1);
	digitalWrite(LATCH_PIN, 0);
	buttons[COUNT] = !digitalRead(DATA_SNES_PIN);
// Pulse the clock then read the remaining button data  
	for (COUNT=1; COUNT<BC_SNES; COUNT++)
	{
		digitalWrite(CLOCK_PIN, 1);
		digitalWrite(CLOCK_PIN, 0);
		buttons[COUNT] = !digitalRead(DATA_SNES_PIN);
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

  // Reset controller data  
  int COUNT = 0;
  for(COUNT; COUNT<SIZE_OF_N64_SIGNAL; COUNT++)
  {
       N64InputSignal[COUNT] = 0; 
  }
  // Send intial polling sequence of 9 bits (0b000000011)
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_LOW(); 
  N64_SIGNAL_HIGH();
  N64_SIGNAL_HIGH();

  DDRD ^= B00000100;      //Toggles N64 data pin to input via direction register
  
  //wait 1us then read bit 0, wait 4us read bit x (x++, x<32), loop isnt used to optimize speed
  //changed to 0us to account for time to of toggling pins (toggles 2 times per signal, x 9 signals = 18 + 1 to toggle to input = 19 clock cycles or 19/16 us)
  //dont need to account for reading signal as last 3us of 4us signal can be read for an accurate value and phase shift shouldnt exceed 32/16MHz = 2us
  
  //_delay_us(1);
  N64InputSignal[0] = (PORTD & B00000100);   // Reads and stores only N64 data pin value by using logical AND expression
  _delay_us(4);
  N64InputSignal[1] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[2] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[3] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[4] = (PORTD & B00000100); 
  _delay_us(4);
  N64InputSignal[5] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[6] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[7] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[8] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[9] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[10] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[11] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[12] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[13] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[14] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[15] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[16] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[17] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[18] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[19] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[20] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[21] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[22] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[23] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[24] = (PORTD & B00000100);
  _delay_us(4);
  N64InputSignal[25] = (PORTD & B00000100); 
  _delay_us(4);
  N64InputSignal[26] = (PORTD & B00000100); 
  _delay_us(4);
  N64InputSignal[27] = (PORTD & B00000100); 
  _delay_us(4);
  N64InputSignal[28] = (PORTD & B00000100); 
  _delay_us(4);
  N64InputSignal[29] = (PORTD & B00000100);  
  _delay_us(4);
  N64InputSignal[30] = (PORTD & B00000100);   
  _delay_us(4);
  N64InputSignal[31] = (PORTD & B00000100);   
  DDRD ^= B00000100;                         //Returns N64 data pin to output for next function call

// Fill gamepad report
  COUNT = 0;
  byte xAxis = 0;
  byte yAxis = 0;
  bool previousState[BC_N64+2];
  int buttonNum = 0;

  // Update button status if change has occured, skip reading bits 8 and 9 as they do not have any significance
  for (COUNT=0; COUNT<BC_N64+2; COUNT++)
  {
    buttonNum++;
    if (COUNT == 8 || COUNT == 9)
    {
      buttonNum--;  
    }
    else if (N64InputSignal[buttonNum] != previousState[buttonNum]){
      n64Ctrl.setButton(buttonNum, N64InputSignal[buttonNum]); 
      previousState[buttonNum] = N64InputSignal[buttonNum];
    }
  }
  // Update joystick status if change has occured
  xAxis = bitsToByte(N64InputSignal, 16);   //bits 16-23
  yAxis = bitsToByte(N64InputSignal, 24);   //bits 24-31
  n64Ctrl.setXAxis(xAxis);
  n64Ctrl.setYAxis(yAxis);
// Send gamepad report
	n64Ctrl.sendState();
}

//Setup Entry Point
void setup() 
{
//Initialize gpio pin modes
    if (BOOTUP)
    {
        pinMode(DATA_NES_PIN, INPUT);
        pinMode(DATA_SNES_PIN, INPUT);
        pinMode(DATA_N64_PIN, OUTPUT);
        pinMode(CLOCK_PIN, OUTPUT);
        pinMode(LATCH_PIN, OUTPUT);
        digitalWrite(CLOCK_PIN, 0);
        digitalWrite(LATCH_PIN, 0);
        digitalWrite(DATA_N64_PIN, 0);
        BOOTUP = 0;   //Turn off boot flag
    }
    
// Refresh connection to controllers
  disconnectControllers();
	connectControllers();

}

// Main Loop
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
// Read reset pin for flag
	if (!digitalRead(RESET_PIN))
		RESET = 1;
// Delay to run between roughly 60-100Hz per read/send
	delay(10);
}


