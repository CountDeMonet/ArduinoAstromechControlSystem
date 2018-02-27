#define dirOut 2
#define pwmOut 3
#define TransIn 4

int ch1 = 0;  // Receiver Channel 1 PPM value

void setup() {
  pinMode(dirOut,OUTPUT);
  pinMode(pwmOut,OUTPUT);
  pinMode(TransIn, INPUT);
  Serial.begin(9600);
}

void loop() {
  // Read in the length of the signal in microseconds
  ch1 = pulseIn(TransIn, HIGH,25000);
  int inputVal = map(ch1, 1000, 2000, -500, 500);
  int pwmVal = 0;
  int directon = 0;
  
  if( inputVal > 0 ){
    // forward
    directon = 0;
    pwmVal = map(inputVal, 0, 500, 0, 255);

    digitalWrite(dirOut,HIGH);
  }else{
    // backward
    directon = 1;
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

  Serial.print("Direction: ");
  Serial.print(directon);
  Serial.print(" - Speed: ");
  Serial.println(pwmVal);
  Serial.println("------------");
  
  delay(100);
}
