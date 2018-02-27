#define dirOut 2
#define pwmOut 3
#define transIn 4

void setup() {
  pinMode(dirOut,OUTPUT);
  pinMode(pwmOut,OUTPUT);
  pinMode(transIn, INPUT);
}

void loop() {
  // Read in the length of the signal in microseconds
  int ch1 = pulseIn(transIn, HIGH,25000);
  int inputVal = map(ch1, 1000, 2000, -500, 500);
  int pwmVal = 0;
  
  if( inputVal > 0 ){
    // forward
    pwmVal = map(inputVal, 0, 500, 0, 255);
    digitalWrite(dirOut,HIGH);
  }else{
    // backward
    pwmVal = map(inputVal, 0, -500, 0, 255);
    digitalWrite(dirOut,LOW);
  }

  // if near center stick ignore
  if( pwmVal < 10 ){
    pwmVal = 0;
  }
  // if somehow over speed set max speed
  if( pwmVal > 255 ){
    pwmVal = 255;
  }

  // set the motor speed 
  analogWrite(pwmOut, pwmVal);
  
  delay(100);
}
