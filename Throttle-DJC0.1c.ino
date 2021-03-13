//Includes
#include "Arduino.h"
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "roster.h"

const char Version[21] = "11/03/2021-DJCv0.1c ";//Pin changes to accommodate motor driver

//Hardware setup
LiquidCrystal_I2C lcd(0x27,20,4);
const int pot1Pin = A3;
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
const char keys[ROWS][COLS] = {
    {'1','2','3', 'A'},
    {'4','5','6', 'B'},
    {'7','8','9', 'C'},
    {'*','0','#', 'D'}
};
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
const boolean usePotMotor = 1;
const byte enA = 10;//PWM pin for driver
const byte in1 = 11;
const byte in2 = 12;
const int Trigger1 = A1;
const int Trigger2 = A0;

//Globals
byte funcTenTw;
char key;
boolean TrigVal1 = 1;
boolean TrigVal2 = 1;
boolean trackPwr = 0;
byte ActiveAddress = 0;
int potValue = 0;//Only used to compare pot pos

// Array set for Locos
const byte maxLocos = 4;
int LocoAddress[maxLocos];
boolean LocoDirection[maxLocos];
byte LocoSpeed[maxLocos];
char LocoNames[maxLocos][21]; //used to buffer names
byte LocoFN0to4[maxLocos];
byte LocoFN5to8[maxLocos];
byte LocoFN9to12[maxLocos];
byte LocoFN13to20[maxLocos];
byte LocoFN21to28[maxLocos];

//Icon
byte icon0[] = {B00000, B01111, B01000, B01000, B11000, B10000, B10000, B10000};
byte icon1[] = {B00000, B11000, B01000, B01000, B01111, B01000, B01000, B01000};
byte icon2[] = {B00000, B00000, B01100, B01100, B11111, B00000, B00000, B00000};
byte icon3[] = {B00000, B11000, B11000, B11000, B11110, B00001, B00001, B00001};
byte icon4[] = {B11111, B10011, B10100, B11000, B01000, B00100, B00011, B11111};
byte icon5[] = {B11111, B00001, B00010, B11100, B10100, B00010, B00001, B11111};
byte icon6[] = {B11111, B00000, B10001, B01110, B01010, B10001, B00000, B11111};
byte icon7[] = {B11100, B10010, B01011, B00101, B00100, B01000, B10000, B11111};

void setup() {
  //Fill loco arrays
  for (byte ii = 0; ii < maxLocos; ii++) {
    LocoAddress[ii] = 999 - ii;
    LocoDirection[ii] = 1;
    LocoSpeed[ii] = 0;
    LocoFN0to4[ii] = 128;
    LocoFN5to8[ii] = 176;
    LocoFN9to12[ii] = 160;
    LocoFN13to20[ii] = 0;
    LocoFN21to28[ii] = 0;
  }

  //Setup pins
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(Trigger1, INPUT);
  pinMode(Trigger2, INPUT);
  digitalWrite(Trigger1, HIGH);// turn on pullup resistors
  digitalWrite(Trigger2, HIGH);// turn on pullup resistors

  //Setup LCD
  lcd.init();
  lcd.createChar(0, icon0);
  lcd.createChar(1, icon1);
  lcd.createChar(2, icon2);
  lcd.createChar(3, icon3);
  lcd.createChar(4, icon4);
  lcd.createChar(5, icon5);
  lcd.createChar(6, icon6);
  lcd.createChar(7, icon7);
  lcd.backlight();

  //Setup serial connection to DCC++ base station and turn off the power
  Serial.begin (115200);
  delay(100);
  Serial.print("<0>");

  //Print welcome screen
  lcd.setCursor(0, 0);
  lcd.print("   DCC++ Throttle   ");
  lcd.setCursor(0, 1);
  lcd.print(Version);
  lcd.setCursor(8, 2);
  lcd.write(0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.setCursor(8, 3);
  lcd.write(4);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);

  //Read EEPROM
  getAddresses();

  delay(2000);
  lcd.clear();
  doMainLCD();

} //End setup

void loop() {
  TrigVal1 = digitalRead(Trigger1);
  TrigVal2 = digitalRead(Trigger2);
  key = keypad.getKey();
  if (key > 47 && key < 58) { //Function request
    doFunction(key+(funcTenTw*10)-48-1);
    funcTenTw = 0;
    lcd.setCursor(1,2);
    lcd.print(funcTenTw);
  } else if (key == 42) { //Access functions 10+ and 20+
    funcTenTw++;
    if (funcTenTw > 2) {
      funcTenTw = 0;
    }
    lcd.setCursor(1,2);
    lcd.print(funcTenTw);
  } else if (key == 65) { //A - Track power button
    TrackPwr(2);
  } else if (key == 66) { //B - Get new loco address
    getLocoAddress();
    lcd.clear();
    doMainLCD();
  } else if (key == 68) { //D - Change loco direction
    changeDirection(2);
  } else if (key == 35) { //# - Switch loco
    ActiveAddress++;
    if (ActiveAddress >= 4) {
      ActiveAddress = 0;
    }
    doMainLCD();
    if (usePotMotor) {
      movePot((LocoSpeed[ActiveAddress]*8)+3);
      delay(100);
      movePot((LocoSpeed[ActiveAddress]*8)+3);
      delay(100);
      movePot((LocoSpeed[ActiveAddress]*8)+3);
      delay(100);
      potValue = analogRead(pot1Pin);
    }
  }
  if (TrigVal2 == 0) { //Switch loco
    ActiveAddress++;
    if (ActiveAddress >= 4) {
      ActiveAddress = 0;
    }
    doMainLCD();
    if (usePotMotor) {
      movePot((LocoSpeed[ActiveAddress]*8)+3);
      delay(100);
      movePot((LocoSpeed[ActiveAddress]*8)+3);
      delay(100);
      movePot((LocoSpeed[ActiveAddress]*8)+3);
      delay(100);
      potValue = analogRead(pot1Pin);
    } else {
      delay(100);
    }
  }
  if (TrigVal1 == 0) { //Get new loco address
    getLocoAddress();
    lcd.clear();
    doMainLCD();
  }
  getSpeed(); //Measure pot position
} //End of loop

void getSpeed() {
  int val = analogRead(pot1Pin);
  if (abs(potValue - val) > 4) {//Only make changes if the pot moves significantly
    LocoSpeed[ActiveAddress] = val / 8;
    doDCC();
    lcd.setCursor(0, 0);
    lcd.print("S=   ");
    lcd.setCursor(2, 0);
    lcd.print(LocoSpeed[ActiveAddress], DEC);
    potValue = val;
    delay(50);
  }
}

void TrackPwr(byte stat) {
  if (stat == 1 || stat == 0) {
    Serial.print("<");
    Serial.print(stat);
    Serial.print(">");
    trackPwr = stat;
  } else {
    if (trackPwr == 0) {
      Serial.print("<1>");
      trackPwr = 1;
    } else {
      Serial.print("<0>");
      trackPwr = 0;
    }
  }
  doMainLCD();
}

void changeDirection(byte stat) {
  if (stat == 1 || stat == 0) {
    LocoDirection[ActiveAddress] = stat;
  } else {
    if (LocoDirection[ActiveAddress] == 0) {
      LocoDirection[ActiveAddress] = 1;
    } else {
      LocoDirection[ActiveAddress] = 0;
    }
  }
  doDCC();
  lcd.setCursor(6, 0);
  if (LocoDirection[ActiveAddress] == 1 ) {
    lcd.print(">");
  }
  else {
    lcd.print("<");
  }
}

void doFunction(byte funcVal) {
  if (funcVal == -1) {
    if (bitRead(LocoFN0to4[ActiveAddress], 5) == 0 ) {
      bitWrite(LocoFN0to4[ActiveAddress], 5, 1);
    } else {
      bitWrite(LocoFN0to4[ActiveAddress], 5, 0);
    }
    sendDCCfunction(LocoFN0to4[ActiveAddress], 0);
  } else if (funcVal < 4) {
    if (bitRead(LocoFN0to4[ActiveAddress], funcVal) == 0 ) {
      bitWrite(LocoFN0to4[ActiveAddress], funcVal, 1);
    } else {
      bitWrite(LocoFN0to4[ActiveAddress], funcVal, 0);
    }
    sendDCCfunction(LocoFN0to4[ActiveAddress], 0);
  } else if (funcVal < 8) {
    if (bitRead(LocoFN5to8[ActiveAddress], funcVal-4) == 0 ) {
      bitWrite(LocoFN5to8[ActiveAddress], funcVal-4, 1);
    } else {
      bitWrite(LocoFN5to8[ActiveAddress], funcVal-4, 0);
    }
    sendDCCfunction(LocoFN5to8[ActiveAddress], 0);
  } else if (funcVal < 12) {
    if (bitRead(LocoFN9to12[ActiveAddress], funcVal-8) == 0 ) {
      bitWrite(LocoFN9to12[ActiveAddress], funcVal-8, 1);
    } else {
      bitWrite(LocoFN9to12[ActiveAddress], funcVal-8, 0);
    }
    sendDCCfunction(LocoFN9to12[ActiveAddress], 0);
  } else if (funcVal < 20) {
    if (bitRead(LocoFN13to20[ActiveAddress], funcVal-12) == 0 ) {
      bitWrite(LocoFN13to20[ActiveAddress], funcVal-12, 1);
    } else {
      bitWrite(LocoFN13to20[ActiveAddress], funcVal-12, 0);
    }
    sendDCCfunction(222, LocoFN13to20[ActiveAddress]);
  } else if (funcVal < 28) {
    if (bitRead(LocoFN21to28[ActiveAddress], funcVal-20) == 0 ) {
      bitWrite(LocoFN21to28[ActiveAddress], funcVal-20, 1);
    } else {
      bitWrite(LocoFN21to28[ActiveAddress], funcVal-20, 0);
    }
    sendDCCfunction(223, LocoFN21to28[ActiveAddress]);
  }
  displayFunctions();
}// End of doFunction

void getLocoAddress() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set DCC Addr # ");
  lcd.print(ActiveAddress + 1);
  lcd.setCursor(6, 2);
  boolean finished = 0;
  int number = 0;
  do {
    key = keypad.getKey();
    if (key > 47 && key < 58) {
      number = (number * 10) + key - 48;
      lcd.print(key-48);
    } else if (key == 66) {
      finished = 1;
    } else if (key == 67) {
      lcd.setCursor(0, 1);
      lcd.print("Selection cancelled");
      delay (1000);
      return;
    } else if (key == 65) { //Track power button
      TrackPwr(2);
    }
  } while (finished == 0);
  LocoAddress[ActiveAddress] = number;
  LocoFN5to8[ActiveAddress] = 176;
  LocoFN9to12[ActiveAddress] = 160;
  LocoFN13to20[ActiveAddress] = 0;
  LocoFN21to28[ActiveAddress] = 0;
  LocoDirection[ActiveAddress] = 1;
  LocoSpeed[ActiveAddress] = 0;
  strcpy_P(LocoNames[ActiveAddress], (char *)pgm_read_word(&(RosterNames[findNameInRoster(LocoAddress[ActiveAddress])])));
  saveAddresses();
  lcd.clear();
  doMainLCD();
}

void doDCC() {
  Serial.print("<t1 ");
  Serial.print(LocoAddress[ActiveAddress]);
  Serial.print(" ");
  Serial.print(LocoSpeed[ActiveAddress]);
  Serial.print(" ");
  Serial.print(LocoDirection[ActiveAddress]);
  Serial.print(">");
}

void sendDCCfunction(byte b1, byte b2) {
  Serial.print("<f ");
  Serial.print(LocoAddress[ActiveAddress]);
  Serial.print(" ");
  Serial.print(b1);
  if (b1 > 221) {
    Serial.print(" ");
    Serial.print(b2);
  }
  Serial.print(">");
}

void displayFunctions() {
  lcd.setCursor(0, 2);
  lcd.print("F");
  lcd.print(funcTenTw);
  lcd.print("  ");
  lcd.print(bitRead(LocoFN0to4[ActiveAddress], 5));
  lcd.print(" ");
  for (byte ii = 0; ii < 4; ii++) {
    lcd.print(bitRead(LocoFN0to4[ActiveAddress], ii));
  }
  lcd.print(" ");
  for (byte ii = 0; ii < 4; ii++) {
    lcd.print(bitRead(LocoFN5to8[ActiveAddress], ii));
  }
  lcd.print(" ");
  for (byte ii = 0; ii < 4; ii++) {
    lcd.print(bitRead(LocoFN9to12[ActiveAddress], ii));
  }
  lcd.setCursor(0, 3);
  lcd.print(" ");
  for (byte ii = 0; ii < 8; ii++) {
    lcd.print(bitRead(LocoFN13to20[ActiveAddress], ii));
    if (ii == 3) {
      lcd.print(" ");
    }
  }
  lcd.print(" ");
  for (byte ii = 0; ii < 8; ii++) {
    lcd.print(bitRead(LocoFN21to28[ActiveAddress], ii));
    if (ii == 3) {
      lcd.print(" ");
    }
  }
}

void getAddresses() { //1KB available
  int pos = 0;
  for (byte ii = 0; ii < maxLocos; ii++) {
    EEPROM.get(pos, LocoAddress[ii]);
    pos = pos + sizeof(LocoAddress[ii]);
    strcpy_P(LocoNames[ii], (char *)pgm_read_word(&(RosterNames[findNameInRoster(LocoAddress[ii])])));
  }
}

void saveAddresses() {
  int pos = 0;
  for (byte ii = 0; ii < maxLocos; ii++) {
    EEPROM.put(pos, LocoAddress[ii]);
    pos = pos + sizeof(LocoAddress[ii]);
  }
}

void doMainLCD() {
  lcd.setCursor(0, 0);
  lcd.print("S=");
  lcd.print(LocoSpeed[ActiveAddress], DEC);
  lcd.print("  ");
  lcd.setCursor(6, 0);
  if (LocoDirection[ActiveAddress] == 1 ) {
    lcd.print(">");
  }
  else {
    lcd.print("<");
  }
  lcd.setCursor(8, 0);
  lcd.print("L=");
  lcd.print(LocoAddress[ActiveAddress] , DEC);
  lcd.print("   ");
  lcd.setCursor(15, 0);
  lcd.print("#");
  lcd.setCursor(16, 0);
  lcd.print(ActiveAddress + 1);
  lcd.setCursor(19, 0);
  if (trackPwr == 1) {
    lcd.print("^");
  } else {
    lcd.print("!");
  }
  lcd.setCursor(0,1);
  lcd.print(LocoNames[ActiveAddress]);
  displayFunctions();
}

int findNameInRoster(int addr) {
  int pos = 0;
  for (byte ii = 0; ii < (sizeof(RosterIndex) / sizeof(RosterIndex[0])); ii++) {
    if (addr == RosterIndex[ii]) {
      pos = ii;
    }
  }
  return pos;
}

void movePot(int target){
  int val;
  do {
    val = analogRead(pot1Pin);
    if (val > target){
          digitalWrite(in1, LOW);
          digitalWrite(in2, HIGH); 
      } else {
          digitalWrite(in1, HIGH);
          digitalWrite(in2, LOW); 
      }
      analogWrite(enA, max(min(abs(val - target), 255), 120));
  } while (abs(val - target) > 4);
  analogWrite(enA, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);  
}
