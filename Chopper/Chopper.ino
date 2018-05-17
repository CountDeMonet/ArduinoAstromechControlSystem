// =======================================================================================
// /////////////////////////Padawan360 Body Code - Mega v1.0 ////////////////////////////////////
// =======================================================================================
/*
by Dan Kraus
dskraus@gmail.com
Astromech: danomite4047

Heavily influenced by DanF's Padwan code which was built for Arduino+Wireless PS2
controller leveraging Bill Porter's PS2X Library. I was running into frequent disconnect
issues with 4 different controllers working in various capacities or not at all. I decided
that PS2 Controllers were going to be more difficult to come by every day, so I explored
some existing libraries out there to leverage and came across the USB Host Shield and it's
support for PS3 and Xbox 360 controllers. Bluetooth dongles were inconsistent as well
so I wanted to be able to have something with parts that other builder's could easily track
down and buy parts even at your local big box store.

*** Modified  
by Eric Banker
ebanker@vineripeconsulting.com
Astromech: C0UNTDEM0NET

This has been modified to use some cheaper components like an Adafruit Audio FX board and a 
Cytron motor controller for the dome rotation. Some features have been removed as this code
is being used in a Chopper build.

https://astromech.net/forums/showthread.php?34866-C0unt-s-Chopper-Build

Hardware:
Arduino Mega 2560
USB Host Shield
Microsoft Xbox 360 Controller
Xbox 360 USB Wireless Reciver
Sabertooth 2x Motor Controller
Cytron 10A Motor Controller
Adafruit AudioFX Board

Set Sabertooth 2x32/2x25/2x12 Dip Switches 1 and 2 Down, All Others Up
*/

#include <Sabertooth.h>
#include <XBOXRECV.h>
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

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
const byte DOMESPEED = 230;

// Ramping- the lower this number the longer R2 will take to speedup or slow down,
// change this by incriments of 1
const byte RAMPING = 5;

// Set the baude rate for the Sabertooth motor controller (feet)
// 9600 is the default baud rate for Sabertooth packet serial.
// for packetized options are: 2400, 9600, 19200 and 38400. I think you need to pick one that works
// and I think it varies across different firmware versions.
const int SABERTOOTHBAUDRATE = 9600;

// Compensation is for deadband/deadzone checking. There's a little play in the neutral zone
// which gets a reading of a value of something other than 0 when you're not moving the stick.
// It may vary a bit across controllers and how broken in they are, sometimex 360 controllers
// develop a little bit of play in the stick at the center position. You can do this with the
// direct method calls against the Syren/Sabertooth library itself but it's not supported in all
// serial modes so just manage and check it in software here
// use the lowest number with no drift
// DOMEDEADZONERANGE for the left stick, DRIVEDEADZONERANGE for the right stick
const byte DOMEDEADZONERANGE = 20;
const byte DRIVEDEADZONERANGE = 30;

/* define the pins in use */
Sabertooth Sabertooth2x(128, Serial1);

// dome controller 
#define domeDirPin 2
#define domeSpeedPin 3

// soundboard pins and setup
#define SFX_RST 4
#define SFX_RX 5
#define SFX_TX 6
const int ACT = 7;    // this allows us to know if the audio is playing

// init the audio board
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard( &ss, NULL, SFX_RST);

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
int rightStick = 0;
char sticknum = 0;
char turnThrottle = 0;

boolean firstLoadOnConnect = false;

USB Usb;
XBOXRECV Xbox(&Usb);

void setup(){
  // softwareserial at 9600 baud for the audio board
  ss.begin(9600);

  // see if we have the soundboard
  // If we fail to communicate, loop forever for now but it would be nice to warn the user somehow
  if (!sfx.reset()) {
    while (1);
  }

  // set act modes for the fx board
  pinMode(ACT, INPUT);
  
  // dome controller
  pinMode(domeDirPin, OUTPUT);
  pinMode(domeSpeedPin, OUTPUT);

  // foot controller
  Serial1.begin(SABERTOOTHBAUDRATE);
  
  Sabertooth2x.autobaud();
  // The Sabertooth won't act on mixed mode packet serial commands until
  // it has received power levels for BOTH throttle and turning, since it
  // mixes the two together to get diff-drive power levels for both motors.
  Sabertooth2x.drive(0);
  Sabertooth2x.turn(0);
  Sabertooth2x.setTimeout(950);

  // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  while (!Serial);
    if (Usb.Init() == -1) {
      while (1); //halt
    }
}

int prevPwmVal = -1;

void loop(){
  Usb.Task();

  // find out of the audio board is playing audio
  int playing = digitalRead(ACT);
  
  // if we're not connected, return so we don't bother doing anything else.
  // set all movement to 0 so if we lose connection we don't have a runaway droid!
  // a restraining bolt and jawa droid caller won't save us here!
  if(!Xbox.XboxReceiverConnected || !Xbox.Xbox360Connected[0]){
    Sabertooth2x.drive(0);
    Sabertooth2x.turn(0);
    digitalWrite(domeDirPin, LOW);
    analogWrite(domeSpeedPin, 0);
    firstLoadOnConnect = false;
    return;
  }

  // After the controller connects, Blink all the LEDs so we know drives are disengaged at start
  if(!firstLoadOnConnect){
    firstLoadOnConnect = true;
    char track[] = "T00     OGG";
    playAudioTrack(track, playing); 
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
      char track[] = "T33     OGG";
      playAudioTrack(track, playing);
    } else {
      isDriveEnabled = true;
      char track[] = "T37     OGG";
      playAudioTrack(track, playing);
      
      //When the drive is enabled, set our LED accordingly to indicate speed
      if(drivespeed == DRIVESPEED1){
        Xbox.setLedOn(LED1, 0);
      } else if(drivespeed == DRIVESPEED2 && (DRIVESPEED3!=0)){
        Xbox.setLedOn(LED2, 0);
      } else {
        Xbox.setLedOn(LED3, 0);
      }
    }
  }

  //Toggle automation mode with the BACK button
  if(Xbox.getButtonClick(BACK, 0)) {
    if(isInAutomationMode){
      isInAutomationMode = false;
      automateAction = 0;
      char track[] = "T37     OGG";
      playAudioTrack(track, playing);
    } else {
      isInAutomationMode = true;
      char track[] = "T51     OGG";
      playAudioTrack(track, playing);
    }
  }

  // Plays random sounds or dome movements for automations when in automation mode
  if (isInAutomationMode) {
    unsigned long currentMillis = millis();

    if (currentMillis - automateMillis > (automateDelay * 1000)) {
      automateMillis = millis();
      automateAction = random(1, 5);

      if (automateAction > 1) {
        playAudio(random(20, 51),playing); 
      }
      
      if (automateAction < 4) {
        // set the direction
        if( turnDirection > 0 ){
          digitalWrite(domeDirPin, LOW);
        } else {
          digitalWrite(domeDirPin, HIGH);
        }
        // move the dome
        analogWrite(domeSpeedPin, 200);

        delay(750);
        
        // stop the dome
        analogWrite(domeSpeedPin, 0);

        // change the direction for the next go
        if (turnDirection > 0) {
          turnDirection = -45;
        } else {
          turnDirection = 45;
        }
      }

      // sets the mix, max seconds between automation actions - sounds and dome movement
      automateDelay = random(3,10);
    }
  }

  // Volume Control of MP3 Trigger
  // Hold R1 and Press Up/down on D-pad to increase/decrease volume
  if(Xbox.getButtonClick(UP, 0)){
    // volume up
    if(Xbox.getButtonPress(R1, 0)){
      sfx.volUp();
    }
  }
  if(Xbox.getButtonClick(DOWN, 0)){
    //volume down
    if(Xbox.getButtonPress(R1, 0)){
      sfx.volDown();
    }
  }

  // GENERAL SOUND PLAYBACK AND DISPLAY CHANGING

  // Y Button and Y combo buttons
  if(Xbox.getButtonClick(Y, 0)){
    if(Xbox.getButtonPress(L1, 0)){
      char track[] = "T01     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(L2, 0)){
      char track[] = "T05     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(R1, 0)){
      char track[] = "T09     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(R2, 0)){
      char track[] = "T13     OGG";
      playAudioTrack(track, playing);
    } else {
      playAudio(random(0, 14),playing); 
    }
  }

  // A Button and A combo Buttons
  if(Xbox.getButtonClick(A, 0)){
    if(Xbox.getButtonPress(L1, 0)){
      char track[] = "T02     OGG";
      playAudioTrack(track, playing); 
    } else if(Xbox.getButtonPress(L2, 0)){
      char track[] = "T06     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(R1, 0)){
      char track[] = "T10     OGG";
      playAudioTrack(track, playing); 
    } else if(Xbox.getButtonPress(R2, 0)){
      char track[] = "T14     OGG";
      playAudioTrack(track, playing);
    } else {
      playAudio(random(15, 28),playing);
    }
  }

  // B Button and B combo Buttons
  if(Xbox.getButtonClick(B, 0)){
    if(Xbox.getButtonPress(L1, 0)){
      char track[] = "T03     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(L2, 0)){
      char track[] = "T07     OGG";
      playAudioTrack(track, playing); 
    } else if(Xbox.getButtonPress(R1, 0)){
      char track[] = "T11     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(R2, 0)){
      char track[] = "T15     OGG";
      playAudioTrack(track, playing);
    } else {
      playAudio(random(29, 42),playing);
    }
  }

  // X Button and X combo Buttons
  if(Xbox.getButtonClick(X, 0)){
    // leia message L1+X
    if(Xbox.getButtonPress(L1, 0)){
      char track[] = "T04     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(L2, 0)){
      char track[] = "T08     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(R1, 0)){
      char track[] = "T12     OGG";
      playAudioTrack(track, playing);
    } else if(Xbox.getButtonPress(R2, 0)){
      char track[] = "T16     OGG";
      playAudioTrack(track, playing);
    } else {
      playAudio(random(43, 51),playing);
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
    } else if(drivespeed == DRIVESPEED2 && (DRIVESPEED3!=0)){
      //change to high speed and play sound scream
      drivespeed = DRIVESPEED3;
      Xbox.setLedOn(LED3, 0);
    } else {
      //we must be in high speed
      //change to low speed and play sound 2-tone
      drivespeed = DRIVESPEED1;
      Xbox.setLedOn(LED1, 0);
    }
  }

  // FOOT DRIVES
  // Xbox 360 analog stick values are signed 16 bit integer value
  // Sabertooth runs at 8 bit signed. -127 to 127 for speed (full speed reverse and  full speed forward)
  // Map the 360 stick values to our min/max current drive speed
  rightStick = (map(Xbox.getAnalogHat(RightHatY, 0), -32768, 32767, -drivespeed, drivespeed));
  if (rightStick > -DRIVEDEADZONERANGE && rightStick < DRIVEDEADZONERANGE) {
    // stick is in dead zone - don't drive
    driveThrottle = 0;
  } else {
    if (driveThrottle < rightStick) {
      if (rightStick - driveThrottle < (RAMPING + 1) ) {
        driveThrottle += RAMPING;
      } else {
        driveThrottle = rightStick;
      }
    } else if (driveThrottle > rightStick) {
      if (driveThrottle - rightStick < (RAMPING + 1) ) {
        driveThrottle -= RAMPING;
      } else {
        driveThrottle = rightStick;
      }
    }
  }

  turnThrottle = map(Xbox.getAnalogHat(RightHatX, 0), -32768, 32767, -TURNSPEED, TURNSPEED);

  // DRIVE!
  // right stick (drive)
  if (isDriveEnabled) {
    // Only do deadzone check for turning here. Our Drive throttle speed has some math applied
    // for RAMPING and stuff, so just keep it separate here
    if (turnThrottle > -DRIVEDEADZONERANGE && turnThrottle < DRIVEDEADZONERANGE) {
      // stick is in dead zone - don't turn
      turnThrottle = 0;
    }
    Sabertooth2x.turn(-turnThrottle);
    Sabertooth2x.drive(driveThrottle);
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
  
  // sends the values to the motor controller
  analogWrite(domeSpeedPin, pwmVal);
 
  if( prevPwmVal != pwmVal){
    prevPwmVal = pwmVal;
  }
  
  delay(1);
} 

/* ************* Audio Board Helper Functions ************* */
// helper function to play a track by name on the audio board
void playAudioTrack( char trackname[], int playing ) {
  // stop track if one is going
  if (playing == 0) {
    sfx.stop();
  }
    
  // now go play
  if (sfx.playTrack(trackname)) {
    sfx.unpause();
  }
}

// helper function to play a track by number on the audio board
void playAudio( uint8_t tracknum, int playing ) {
  // stop track if one is going
  if (playing == 0) {
    sfx.stop();
  }
    
  // now go play
  if (sfx.playTrack(tracknum)) {
    sfx.unpause();
  }
}


