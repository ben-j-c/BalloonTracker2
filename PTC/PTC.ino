#include <Servo.h>
#define HOME_PAN ((int) 1367)
#define HOME_TILT ((int) 1750)

Servo pan;
Servo tilt;
void setup() {
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
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
  byte b[5];
  b[0] = Serial.read();

  if(b[0] != 'p') { //if not a position
    switch(b[0]) {
      case 'b': //init instruction
        initMotors();
        Serial.print('a'); 
      break;
      
      case 'd': //detach instruction
        pan.detach();
        tilt.detach();
        pinMode(10, OUTPUT);
        pinMode(11, OUTPUT);
        digitalWrite(10, LOW);
        digitalWrite(11, LOW);
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
  
  for(int i = 1; i < 5;i++) { 
    b[i] = Serial.read();
  }

  int p = ((int) b[2] << 8) + ((int) b[1]);
  int t = ((int) b[4] << 8) + ((int) b[3]);
  moveTo(p,t);
}

void loop() {
  //delay(20);
}
