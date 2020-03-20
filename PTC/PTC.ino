#include <Servo.h>
#define HOME_PAN ((int) 1367)
#define HOME_TILT ((int) 1750)

Servo pan;
Servo tilt;
void setup() {
  Serial.begin(115200);
  delay(100);
  waitForBoot();
}

int panValue = HOME_PAN;
int tiltValue = HOME_TILT;

bool waitForBoot(){
  while (!Serial); // dont do anything until serial comm with host is open
}

void moveTo(int p, int t) {
  panValue = p;
  tiltValue = t;
  pan.writeMicroseconds(p);
  tilt.writeMicroseconds(t);
}

void attachServos() {
  pan.attach(10,553,2425);
  tilt.attach(11,553,2520);
}

void initMotors() {
  attachServos();
  moveTo(HOME_PAN, HOME_TILT);
}

void serialEvent() {
  int numBytes = Serial.available();
  byte b5 = Serial.read();
  byte b4,b3,b2,b1;

  if(b5 != 'p') { //if not a position
    switch(b5) {
      case 'b': //init instruction
        initMotors();
        Serial.print('a'); 
      break;
      
      case 'd': //detach instruction
        pan.detach();
        tilt.detach();
        Serial.print('c');
      break;

      case 'h': //home instruction
        moveTo(HOME_PAN, HOME_TILT);
      break;
    }
    return;
  }
  else if(numBytes < 4) {
    while(Serial.available() < 4); //wait for 4 more chars in buffer
  }
  
  while(Serial.available()) { //Say 50 characters are in the buffer, only the last 5 are what we need (newest pos)
    b1 = b2;
    b2 = b3;
    b3 = b4;
    b4 = b5;
    b5 = Serial.read();
  }

  int p = ((int) b3 << 8) + ((int) b2);
  int t = ((int) b5 << 8) + ((int) b4);
  moveTo(p,t);
}

void loop() {
  delay(50);
}
