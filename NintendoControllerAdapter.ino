#include "Joystick.h"
#include "delay_x.h"

#define NES_BC 8
#define NES_DATA 2
#define NES_LATCH 3
#define NES_CLOCK 4
#define SNES_BC 12
#define SNES_DATA 5
#define SNES_LATCH 3
#define SNES_CLOCK 4
#define N64_BC 14
#define N64_X_AXIS_INDEX 16
#define N64_Y_AXIS_INDEX 24
#define N64_DATA_SIZE 32
#define N64_DATA_IN 7
#define N64_DATA_OUT 6

bool nesData[NES_BC];
bool snesData[SNES_BC];
int n64Data[N64_DATA_SIZE];
int previousNesData[NES_BC];
int previousSnesData[SNES_BC];
int previousN64Data[N64_DATA_SIZE];
byte ACTIVE_CONTROLLERS = 0x00;

Joystick_ nesCtrl(0x03, JOYSTICK_TYPE_GAMEPAD, NES_BC, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ snesCtrl(0x03, JOYSTICK_TYPE_GAMEPAD, SNES_BC, 0, false, false, false, false, false, false, false, false, false, false, false);
Joystick_ n64Ctrl(0x03, JOYSTICK_TYPE_MULTI_AXIS, N64_BC, 0, true, true, false, false, false, false, false, false, false, false, false);

//DEBUG USE ONLY
void printN64Data(){
  int COUNT = 0;  
  for (COUNT; COUNT<N64_DATA_SIZE; COUNT++){
    Serial.print("Button ");
    Serial.print(COUNT);
    Serial.print(":");
    Serial.println(n64Data[COUNT]);
  }
}

//ENTRY POINT
void setup()
{
  int COUNT = 0;
  pinMode(NES_DATA, INPUT);  
  pinMode(NES_CLOCK, OUTPUT);
  pinMode(NES_LATCH, OUTPUT);
  pinMode(SNES_DATA, INPUT);  
  pinMode(SNES_CLOCK, OUTPUT);
  pinMode(SNES_LATCH, OUTPUT);
  pinMode(N64_DATA_IN, INPUT);
  pinMode(N64_DATA_OUT, OUTPUT);
  digitalWrite(NES_CLOCK, 0);
  digitalWrite(NES_LATCH, 0);
  digitalWrite(SNES_CLOCK, 0);
  digitalWrite(SNES_LATCH, 0);
  digitalWrite(N64_DATA_OUT, 0);
  for(COUNT; COUNT<N64_DATA_SIZE; COUNT++){
    previousN64Data[COUNT] = 1;
  }
  COUNT = 0;
  for(COUNT; COUNT<NES_BC; COUNT++){
    previousNesData[COUNT] = 1;
  }
    COUNT = 0;
  for(COUNT; COUNT<SNES_BC; COUNT++){
    previousSnesData[COUNT] = 1;
  }
  connectControllers();
}

// Main Loop
void loop()
{ 
  updateNesController();
  updateSnesController();
  updateN64Controller();
}


//FUNCTION DEFINITIONS
void connectControllers(void)
{
//Pulse each controller and listen for response
//Upon response, set active controller bits and initialize joysticks accordingly
  
//Initialize variable to store response from N64 controller
  bool response = 0;

// Probe snes controller
  digitalWrite(NES_LATCH, HIGH);
  digitalWrite(NES_CLOCK, LOW);
  if (digitalRead(NES_DATA))    //Fails if A button is pressed during latch
  {
    bitSet(ACTIVE_CONTROLLERS, 1);  
    nesCtrl.begin(0);
  }
// Probe snes controller
  digitalWrite(SNES_LATCH, HIGH);
  digitalWrite(SNES_CLOCK, LOW);
  if (digitalRead(SNES_DATA))   //Fails if A button is pressed during latch
  { 
    bitSet(ACTIVE_CONTROLLERS, 2);
    snesCtrl.begin(0);
  }
 
// Send intial polling sequence of 9 bits to N64 Controller(0b000000011)
  pollN64Controller();
  response = (PIND & B10000000);   // Reads and stores only N64 data-in pin value by using logical AND expression
  if (response)   
  {
    bitSet(ACTIVE_CONTROLLERS, 3);
    n64Ctrl.begin(0);
  }
}

void updateNesController()
{
//If flag is true, controller is inactive and will exit
  if(!bitRead(ACTIVE_CONTROLLERS, 1))
    return;
    
  int COUNT = 0;
// Latch current button status on controllers
  digitalWrite(NES_LATCH, 1);
  digitalWrite(NES_LATCH, 0);
  nesData[COUNT] = !digitalRead(NES_DATA);
// Pulse the clock then read the remaining button data  
  for (COUNT=1; COUNT<NES_BC; COUNT++)
  {
    digitalWrite(NES_CLOCK, 1);
    digitalWrite(NES_CLOCK, 0);
    snesData[COUNT] = !digitalRead(NES_DATA);
  }
// Compare buttons states and fill button report  
  for (COUNT=0; COUNT<NES_BC; COUNT++)
  {
    if (nesData[COUNT] != previousNesData[COUNT]){
      nesCtrl.setButton(COUNT, nesData[COUNT]); 
      previousNesData[COUNT] = nesData[COUNT];
    }
  }
// Send gamepad report
  nesCtrl.sendState();
}

void updateSnesController()
{
//If flag is true, controller is inactive and will exit
  if(!bitRead(ACTIVE_CONTROLLERS, 2))
    return;
    
  int COUNT = 0;
// Latch current button status on controllers
  digitalWrite(SNES_LATCH, 1);
  digitalWrite(SNES_LATCH, 0);
  snesData[COUNT] = !digitalRead(SNES_DATA);
// Pulse the clock then read the remaining button data  
  for (COUNT=1; COUNT<SNES_BC; COUNT++)
  {
    digitalWrite(SNES_CLOCK, 1);
    digitalWrite(SNES_CLOCK, 0);
    snesData[COUNT] = !digitalRead(SNES_DATA);
  }
// Compare buttons states and fill button report  
  for (COUNT=0; COUNT<SNES_BC; COUNT++)
  {
    if (snesData[COUNT] != previousSnesData[COUNT]){
      snesCtrl.setButton(COUNT, snesData[COUNT]); 
      previousSnesData[COUNT] = snesData[COUNT];
    }
  }
// Send gamepad report
  snesCtrl.sendState();
}

void pollN64Controller(){
  lowSignal();
  lowSignal();
  lowSignal();
  lowSignal();
  lowSignal();
  lowSignal();
  lowSignal();
  highSignal();
  highSignal();
}

void storeN64Data(){
  int COUNT = 0;
// Manually store value of D register in each value of data[32] without loop to save delay of counter variable iteration
  n64Data[0] = PIND;
  _delay_us(4);
  n64Data[1] = PIND;
  _delay_us(4);
  n64Data[2] = PIND;
  _delay_us(4);
  n64Data[3] = PIND;
  _delay_us(4);
  n64Data[4] = PIND;
  _delay_us(4);

  n64Data[5] = PIND;
  _delay_us(4);
  n64Data[6] = PIND;
  _delay_us(4);
  n64Data[7] = PIND;
  _delay_us(4);
  n64Data[8] = PIND;
  _delay_us(4);
  n64Data[9] = PIND;
  _delay_us(4);

  n64Data[10] = PIND;
  _delay_us(4);
  n64Data[11] = PIND;
  _delay_us(4);
  n64Data[12] = PIND;
  _delay_us(4);
  n64Data[13] = PIND;
  _delay_us(4);
  n64Data[14] = PIND;
  _delay_us(4);

  n64Data[15] = PIND;
  _delay_us(4);
  n64Data[16] = PIND;
  _delay_us(4);
  n64Data[17] = PIND;
  _delay_us(4);
  n64Data[18] = PIND;
  _delay_us(4);
  n64Data[19] = PIND;
  _delay_us(4);

  n64Data[20] = PIND;
  _delay_us(4);
  n64Data[21] = PIND;
  _delay_us(4);
  n64Data[22] = PIND;
  _delay_us(4);
  n64Data[23] = PIND;
  _delay_us(4);
  n64Data[24] = PIND;
  _delay_us(4);

  n64Data[25] = PIND;
  _delay_us(4);
  n64Data[26] = PIND;
  _delay_us(4);
  n64Data[27] = PIND;
  _delay_us(4);
  n64Data[28] = PIND;
  _delay_us(4);
  n64Data[29] = PIND;
  _delay_us(4);

  n64Data[30] = PIND;
  _delay_us(4);
  n64Data[31] = PIND;
  _delay_us(4);

// Sets byte to 0 (pressed/high) or 1 (notPressed/low) according to data (N64 Controller reads active low so values are swapped)
// Not time sensitive so a for-loop can be used again
  for(COUNT; COUNT<N64_DATA_SIZE; COUNT++){
    n64Data[COUNT] &= B10000000;
    if((n64Data[COUNT] & 0x80) == 0x80){
      n64Data[COUNT] = 0;
    }
    else{
      n64Data[COUNT] = 1;
    }
  }
}

void updateN64Controller(){
//If flag is true, controller is inactive and will exit
  if(!bitRead(ACTIVE_CONTROLLERS, 3))
    return;
    
//Grab data from controller
  cli();  //Disable interupts as N64 signals are microsecond sensitive
    pollN64Controller();
    storeN64Data();
  sei();  //Renable inteurpts
    
// Fill gamepad report
  int COUNT = 0;
  int xAxis = 0;
  int yAxis = 0;
  int joystickButtonMapping = 0;

  // Update button status if change has occured, skip reading bits 8 and 9 as they do not have any significance
  for (COUNT=0; COUNT<N64_DATA_SIZE; COUNT++)
  {
    joystickButtonMapping++;
    if (COUNT == 8 || COUNT == 9) //These two bits don't account for any buttons or joysticks, ignore them
    {
      joystickButtonMapping--;  
    }
    else if (n64Data[COUNT] != previousN64Data[COUNT]){
      n64Ctrl.setButton(joystickButtonMapping, n64Data[COUNT]); 
      previousN64Data[COUNT] = n64Data[COUNT];
    }
  }
  // Update joystick status if change has occured
  xAxis = bitsToInt(N64_X_AXIS_INDEX);   
  yAxis = bitsToInt(N64_Y_AXIS_INDEX);   
  n64Ctrl.setXAxis(xAxis);
  n64Ctrl.setYAxis(yAxis);
// Send gamepad report
  n64Ctrl.sendState();
}

void lowSignal(){
  PORTD |= 0x40;  //Set pin 6 (n64 data-out) high (B0100 0000)
  _delay_us(0.85);
  PORTD &= 0xBF;  //Set pin 6 low (B1011 1111)
  _delay_us(2.5);
}

void highSignal(){ 
  PORTD |= 0x40;  //Set pin 6 high (B0100 0000)
  _delay_us(2.5);
  PORTD &= 0xBF;  //Set pin 6 low (B1011 1111)
  _delay_us(0.85);
}

int bitsToInt(int index)
{
  int value = 0;
  int COUNT = 0;
  for(COUNT; COUNT<8; COUNT++)
  {
    if(n64Data[index])
    {
      bitSet(value, COUNT);
    }
    index++;
  }
  return value*2*2;   // Scaled x4 for because joystick library range is set to 10bit resolution
}


