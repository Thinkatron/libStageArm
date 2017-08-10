#include "constants.h"
#include "pinModes.h"
typedef bool spin_direction;
int delay_ms = 4000;
spin_direction xDir;
spin_direction yDir;
spin_direction zDir;

float coord[3];
bool calibrated[3];
bool vac_on;

#include "collision.h"
#include "calibrate.h"

void setup() {
  for(int i = 0; i < 3; i++){
    coord[i] = 0;
    calibrated[i] = false;
  }
    
  Serial.begin(9600);
  set_pins();
  vac_on = false;

}

void loop() {
  digitalWrite(DEBUG_LIGHT, digitalRead(A3));
  if(Serial.available()){
    delay(15); //wait for transmission to finish
    byte command = Serial.read();

    if(command == 'a'){
      digitalWrite(DEBUG_LIGHT, LOW);
    }

    switch(command){
     case CALIBRATE: 
       Serial.print(calibrate());
       break;
     case REL_MOV:
       Serial.print(relMov());
       break;
     case ABS_MOV:
       Serial.print(absMov());
       break;
     case SET_VAC:
       Serial.print(setVac());
       break;
     case TOG_VAC:
       Serial.print(togVac());
       break;
     case SET_DEL:
       Serial.print(setDel());
     default: 
       Serial.print(GENERIC_FAIL);
     }
  }
}



byte relMov() {
  byte input[12];

  for(int i = 0; i < 12; i++){
    input[i] = Serial.read();
  }

  float x_coord = *((float*)(input+0));
  float y_coord = *((float*)(input+4));
  float z_coord = *((float*)(input+8));

  return rawRelMov(x_coord, y_coord, z_coord);
}

byte rawRelMov(float x_coord, float y_coord, float z_coord) {
  bool xDir = (x_coord<0)?COUNTERCLOCKWISE:CLOCKWISE; //counterclockwise is left
  //negative is left
  bool yDir = (y_coord<0)?COUNTERCLOCKWISE:CLOCKWISE; //counterclockwise is backwards
  //negative is backwards
  bool zDir = (z_coord<0)?CLOCKWISE:COUNTERCLOCKWISE; //for some reason, the z-axis goes backwards; counterclockwise is up
  //negative is down

  int xTicks = abs(x_coord*PULSES_PER_CIRCLE/DISTANCE_PER_TURN);
  int yTicks = abs(y_coord*PULSES_PER_CIRCLE/DISTANCE_PER_TURN);
  int zTicks = abs(z_coord*PULSES_PER_CIRCLE/DISTANCE_PER_TURN);
  int maxTicks = max(xTicks, max(yTicks, zTicks));
  double current_delay = 5000;
  digitalWrite(DIR_X_PIN, (x_coord > 0? X_POS: X_NEG));
  digitalWrite(DIR_Y_PIN, (y_coord > 0? Y_POS: Y_NEG));
  digitalWrite(DIR_Z_PIN, (z_coord > 0? Z_POS: Z_NEG));

  for(int i = 0; i < maxTicks; i++){
    if(isColliding(zDir,Z) && zTicks!=0){
      zTicks = 0;
      int maxTicks = max(xTicks, max(yTicks, zTicks));
    }
    if(isColliding(yDir,Y) && yTicks!=0){
      yTicks = 0;
      int maxTicks = max(xTicks, max(yTicks, zTicks));
    }
    if(isColliding(xDir,X) && xTicks!=0){
      xTicks = 0;
      int maxTicks = max(xTicks, max(yTicks, zTicks));
    }

    if(i<xTicks){
      digitalWrite(CP_X_PIN, HIGH);
      digitalWrite(CP_X_PIN, LOW);
    }
    if(i<yTicks){
      digitalWrite(CP_Y_PIN, HIGH);
      digitalWrite(CP_Y_PIN, LOW);
    }
    if(i<zTicks){
      digitalWrite(CP_Z_PIN, HIGH);
      digitalWrite(CP_Z_PIN, LOW);
    }
    if (5000 > delay_ms) {
      if (i <= 1.5 * PULSES_PER_CIRCLE/DISTANCE_PER_TURN) {
        current_delay -= (5000 - delay_ms)/(1.5 * PULSES_PER_CIRCLE/DISTANCE_PER_TURN);
      }
      if (i > maxTicks - (1.5 * PULSES_PER_CIRCLE/DISTANCE_PER_TURN)) {
        current_delay += (5000 - delay_ms)/(1.5 * PULSES_PER_CIRCLE/DISTANCE_PER_TURN);
      }
    }
    delayMicroseconds(max(delay_ms, current_delay));
  }

  return SUCCESS;
}


//Set Period of loop
byte setDel() {
  byte input[4];

  for(int i = 0; i < 4; i++){
    input[i] = Serial.read();
  }
  int _delay = *((int*)input);

  if (_delay >= 500) {
    delay_ms = _delay;
    return SUCCESS;
  } else {
    return VOOB;
  }
}


//Move head to coordinate position
byte absMov(){
  byte input[12];

  for(int i = 0; i < 12; i++){
    input[i] = Serial.read();
  }

  float x_coord = *((float*)(input+0));
  float y_coord = *((float*)(input+4));
  float z_coord = *((float*)(input+8));

  if(x_coord != 0 && !calibrated[X]) return UNDEFINED_FAIL;
  if(y_coord != 0 && !calibrated[Y]) return UNDEFINED_FAIL;
  if(z_coord != 0 && !calibrated[Z]) return UNDEFINED_FAIL;

  return rawRelMov(x_coord - coord[X], y_coord - coord[Y], z_coord - coord[Z]);
}

byte setVac() {
  byte input = Serial.read();

  digitalWrite(VACUUM_PIN, input == 1? VAC_ON: VAC_OFF);

  vac_on = (input == 1);

  return SUCCESS;
}

byte togVac() {
  vac_on = !vac_on;

  digitalWrite(VACUUM_PIN, vac_on? VAC_ON: VAC_OFF);

  return SUCCESS;
}

