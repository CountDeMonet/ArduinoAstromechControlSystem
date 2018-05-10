//************************** Set speed and turn speeds here************************************//

//set these 3 to whatever speeds work for you. 0-stop, 127-full speed.
const byte DRIVESPEED1 = 50;
//Recommend beginner: 50 to 75, experienced: 100 to 127, I like 100.
const byte DRIVESPEED2 = 100;
//Set to 0 if you only want 2 speeds.
const byte DRIVESPEED3 = 127;

byte drivespeed = DRIVESPEED1;

// the higher this number the faster the droid will spin in place, lower - easier to control.
// Recommend beginner: 40 to 50, experienced: 50 $ up, I like 70
const byte TURNSPEED = 70;
// If using a speed controller for the dome, sets the top speed. You'll want to vary it potenitally
// depending on your motor. 
const byte DOMESPEED = 220;

// Ramping- the lower this number the longer R2 will take to speedup or slow down,
// change this by incriments of 1
const byte RAMPING = 5;

// Compensation is for deadband/deadzone checking. There's a little play in the neutral zone
// which gets a reading of a value of something other than 0 when you're not moving the stick.
// It may vary a bit across controllers and how broken in they are, sometimex 360 controllers
// develop a little bit of play in the stick at the center position. You can do this with the
// direct method calls against the Syren/Sabertooth library itself but it's not supported in all
// serial modes so just manage and check it in software here
// use the lowest number with no drift
// DOMEDEADZONERANGE for the left stick, DRIVEDEADZONERANGE for the right stick
const byte DOMEDEADZONERANGE = 40;
const byte DRIVEDEADZONERANGE = 20;

#include <XBOXRECV.h>
#include <SoftwareSerial.h>

// Set some defaults for start up
// 0 = full volume, 255 off
byte vol = 20;
// 0 = drive motors off ( right stick disabled ) at start
boolean isDriveEnabled = false;

// Automated function variables
// Used as a boolean to turn on/off automated functions like periodic random sounds and periodic dome turns
boolean isInAutomationMode = false;
unsigned long automateMillis = 0;
byte automateDelay = random(5,20);// set this to min and max seconds between sounds
//How much the dome may turn during automation.
int turnDirection = 20;
// Action number used to randomly choose a sound effect or a dome turn
byte automateAction = 0;
char driveThrottle = 0;
char sticknum = 0;
char turnThrottle = 0;

boolean firstLoadOnConnect = false;

USB Usb;
XBOXRECV Xbox(&Usb);

// dome controller 
#define domeDirPin 2
#define domeSpeedPin 3

void setup(){

  // dome controller
  pinMode(domeDirPin, OUTPUT);
  pinMode(domeSpeedPin, OUTPUT);
  
   Serial.begin(115200);
  // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  while (!Serial);
    if (Usb.Init() == -1) {
      Serial.println("OSC did not start");
      while (1); //halt
    }
  Serial.println("Xbox Wireless Receiver Library Started");
}

int prevPwmVal = -1;

void loop(){
  Usb.Task();
  
  // if we're not connected, return so we don't bother doing anything else.
  // set all movement to 0 so if we lose connection we don't have a runaway droid!
  // a restraining bolt and jawa droid caller won't save us here!
  if(!Xbox.XboxReceiverConnected || !Xbox.Xbox360Connected[0]){
    firstLoadOnConnect = false;
    return;
  }

  // After the controller connects, Blink all the LEDs so we know drives are disengaged at start
  if(!firstLoadOnConnect){
    Serial.println("First Load On Connect");
    firstLoadOnConnect = true;
    Xbox.setLedMode(ROTATING, 0);
  }
  
  if (Xbox.getButtonClick(XBOX, 0)) {
    if(Xbox.getButtonPress(L1, 0) && Xbox.getButtonPress(R1, 0)){ 
      Xbox.disconnect(0);
    }
  }

  // enable / disable right stick (droid movement) & play a sound to signal motor state
  if(Xbox.getButtonClick(START, 0)) {
    if(isDriveEnabled){
      isDriveEnabled = false;
      Xbox.setLedMode(ROTATING, 0);
      Serial.println("Drive Disabled");
    } else {
      isDriveEnabled = true;
      // //When the drive is enabled, set our LED accordingly to indicate speed
      if(drivespeed == DRIVESPEED1){
        Serial.println("Drive Speed 1");
        Xbox.setLedOn(LED1, 0);
      } else if(drivespeed == DRIVESPEED2 && (DRIVESPEED3!=0)){
        Serial.println("Drive Speed 2");
        Xbox.setLedOn(LED2, 0);
      } else {
        Serial.println("Drive Speed 3");
        Xbox.setLedOn(LED3, 0);
      }
    }
  }

  //Toggle automation mode with the BACK button
  if(Xbox.getButtonClick(BACK, 0)) {
    if(isInAutomationMode){
      isInAutomationMode = false;
      automateAction = 0;
    } else {
      isInAutomationMode = true;
    }
  }

  // Volume Control of MP3 Trigger
  // Hold R1 and Press Up/down on D-pad to increase/decrease volume
  if(Xbox.getButtonClick(UP, 0)){
    // volume up
    if(Xbox.getButtonPress(R1, 0)){
      if (vol > 0){
        vol--;
        Serial.println("Volume Down");
      }
    }
  }
  if(Xbox.getButtonClick(DOWN, 0)){
    //volume down
    if(Xbox.getButtonPress(R1, 0)){
      if (vol < 255){
        vol++;

        Serial.println("Volume Up");
      }
    }
  }

  // GENERAL SOUND PLAYBACK AND DISPLAY CHANGING

  // Y Button and Y combo buttons
  if(Xbox.getButtonClick(Y, 0)){
    if(Xbox.getButtonPress(L1, 0)){
      Serial.println("MP3Trigger Play 8");
    } else if(Xbox.getButtonPress(L2, 0)){
      Serial.println("MP3Trigger Play 2");
    } else if(Xbox.getButtonPress(R1, 0)){
      Serial.println("MP3Trigger Play 9");
    } else {
      Serial.println("MP3Trigger Play Random");
    }
  }

  // A Button and A combo Buttons
  if(Xbox.getButtonClick(A, 0)){
    if(Xbox.getButtonPress(L1, 0)){
      Serial.println("MP3Trigger Play 6");
    } else if(Xbox.getButtonPress(L2, 0)){
      Serial.println("MP3Trigger Play 1");
    } else if(Xbox.getButtonPress(R1, 0)){
      Serial.println("MP3Trigger Play 11");
    } else {
      Serial.println("MP3Trigger Play Random");
    }
  }

  // B Button and B combo Buttons
  if(Xbox.getButtonClick(B, 0)){
    if(Xbox.getButtonPress(L1, 0)){
      Serial.println("MP3Trigger Play 7");
    } else if(Xbox.getButtonPress(L2, 0)){
      Serial.println("MP3Trigger Play 3");
    } else if(Xbox.getButtonPress(R1, 0)){
      Serial.println("MP3Trigger Play 10");
    } else {
      Serial.println("MP3Trigger Play Random");
    }
  }

  // X Button and X combo Buttons
  if(Xbox.getButtonClick(X, 0)){
    // leia message L1+X
    if(Xbox.getButtonPress(L1, 0)){
      Serial.println("MP3Trigger Play 5");
    } else if(Xbox.getButtonPress(L2, 0)){
      Serial.println("MP3Trigger Play 4");
    } else if(Xbox.getButtonPress(R1, 0)){
      Serial.println("MP3Trigger Play 12");
    } else {
      Serial.println("MP3Trigger Play Random");
    }
  }

  // Change drivespeed if drive is eabled
  // Press Right Analog Stick (R3)
  // Set LEDs for speed - 1 LED, Low. 2 LED - Med. 3 LED High
  if(Xbox.getButtonClick(R3, 0) && isDriveEnabled) {
    //if in lowest speed
    if(drivespeed == DRIVESPEED1){
      //change to medium speed and play sound 3-tone
      drivespeed = DRIVESPEED2;
      Xbox.setLedOn(LED2, 0);
      Serial.println("Drive Speed 2");
    } else if(drivespeed == DRIVESPEED2 && (DRIVESPEED3!=0)){
      //change to high speed and play sound scream
      drivespeed = DRIVESPEED3;
      Xbox.setLedOn(LED3, 0);
     Serial.println("Drive Speed 3");
    } else {
      //we must be in high speed
      //change to low speed and play sound 2-tone
      drivespeed = DRIVESPEED1;
      Xbox.setLedOn(LED1, 0);
      Serial.println("Drive Speed 1");
    }
  }

  // DOME DRIVE!
  int domeThrottle = 0;
  byte directon = 0;
  int pwmVal = 0;
  
  domeThrottle = (map(Xbox.getAnalogHat(LeftHatX, 0), -32768, 32767, -500, 500));
  if (domeThrottle > -DOMEDEADZONERANGE && domeThrottle < DOMEDEADZONERANGE){
    //stick in dead zone - don't spin dome
    domeThrottle = 0;
  }

  if ( domeThrottle > 0 ) {
    // forward
    directon = 0;
    pwmVal = map(domeThrottle, 0, 500, 0, DOMESPEED);
    digitalWrite(domeDirPin, HIGH);
  } else {
    // backward
    directon = 1;
    pwmVal = map(domeThrottle, 0, -500, 0, DOMESPEED);
    digitalWrite(domeDirPin, LOW);
  }
  
  // if near center stick ignore
  if ( pwmVal < DOMEDEADZONERANGE ) {
    pwmVal = 0;
  }
  // if somehow over speed set max speed
  if ( pwmVal > 255 ) {
    pwmVal = 255;
  }

  analogWrite(domeSpeedPin, pwmVal);
  
  // sends the values to the motor controller
  //analogWrite(domeSpeedPin, pwmVal);
  if( prevPwmVal != pwmVal){
    Serial.print("Direction: ");
    Serial.print(directon);
    Serial.print(" - Speed: ");
    Serial.println(pwmVal);
    Serial.println("------------");
    prevPwmVal = pwmVal;
  }
  
  delay(1);
} 

