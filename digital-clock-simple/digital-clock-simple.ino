#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Font_Data.h"
#include <virtuabotixRTC.h>
#include <EEPROM.h>
#include <PS2Keyboard.h>

virtuabotixRTC myRTC(13, 12, 11);

const byte LDR_PIN = A0; // LDR Sensor pin
const byte LM35_PIN = A1; // LM35 Sensor pin

const byte buttonSet_PIN = 6; // Button Setting
const byte buttonSel_PIN = 5; // Button Sellect

const int DataPin = 2; // D- Keyboard
const int IRQpin =  3; // D+ Keyboard

const byte buzzer_PIN = 7; // Buzzer

uint16_t  dd = 6, mm = 6, yy = 2022;
uint16_t  h, m, s;
float temp_C;
int setup_menu = 0;
int alarm_menu = 0, setting_menu = 0;
unsigned long time_pressed;
unsigned long previousMillis = 0;
int buzzState = LOW; 
boolean secTime = false;
boolean inputKey = false;
int cm, cs;
int sTemp;

int hA1, mA1;


const int romhA1 = 0, rommA1 = 5, romsetA1 = 15;


boolean alarmSet1;


boolean change_alarm_menu = false;

boolean buttonSet_read;
boolean buttonSel_read;

#define MAX_DEVICES 4 // Set the number of devices
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW 

#define CLK_PIN   8
#define DATA_PIN  9
#define CS_PIN    10

// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_MAX72XX::fontType_t *pFont;

PS2Keyboard keyboard;

#define SPEED_TIME 75 // speed of the transition
#define PAUSE_TIME  0  

static uint8_t  display = 0;  // current display mode

#define MAX_MESG  20

// Global variables
char szTime[9];    // mm:ss\0
char szMesg[MAX_MESG+1] = "";
char charh[3], charm[2];
static char c;
uint16_t indexC = 0;
char alarm5c[20];
uint16_t indexhOm = 0;
char hOmChar[2];

uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 }; // Deg C

// Time Setup: Code for reading clock time

void alarm_menus(const char *Alarm, int hA, int mA, boolean alarmSet, int romhA, int rommA, int romsetA, bool f = true);

void getSec() {
  myRTC.updateTime();
  s = myRTC.seconds;
  m = myRTC.minutes;
}

void getTime(char *psz, bool f = true) {
  myRTC.updateTime();
  s = myRTC.seconds;
  m = myRTC.minutes;
  h = myRTC.hours; //24hr Format
  if(secTime) { sprintf(psz, "%02d%c%02d", m, (f ? ':' : ' '), s); }
  //else if(degTime) { temp_C = Get_Temp(LM35_PIN); dtostrf(temp_C, 3, 1, psz); strcat(psz, "$"); }
  else { sprintf(psz, "%02d%c%02d", h, (f ? ':' : ' '), m); }

  Serial.println(s);
}

float Get_Temp(int pin) {
  float temp_adc_val;
  float temp_val;
  float temp;
  unsigned long temptot = 0;
  for(int x=0; x<100 ; x++)
  {
    temptot += analogRead(pin);
   }
  temp_adc_val = temptot/100;
  temp_val = temp_adc_val * (5000 / 1024);
  temp = temp_val*0.1;
  
  
  //Serial.println(temp);
  return temp;
}

void setup(void)
{
  Serial.begin(9600);
  
  P.begin(2);
  P.setInvert(false);
  P.setZone(0,  MAX_DEVICES-4, MAX_DEVICES-1);
  
  P.setZone(1, MAX_DEVICES-4, MAX_DEVICES-1);
  P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

  P.displayZoneText(0, szMesg, PA_CENTER, SPEED_TIME, 0,PA_PRINT , PA_NO_EFFECT);

  P.addChar('$', degC); 

  pinMode(buttonSet_PIN, INPUT_PULLUP);
  pinMode(buttonSel_PIN, INPUT_PULLUP);

  pinMode(buzzer_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT_PULLUP);  

  keyboard.begin(DataPin, IRQpin);

  //analogReference(INTERNAL);
}

void loop(void)
{
  byte ledIntensity = ledIntensitySelect(analogRead(LDR_PIN));
  P.setIntensity(ledIntensity); // value between 0 and 15 for brightness
  static uint32_t lastTime = 0; // millis() memory
  static bool flasher = false;  // seconds passing flasher
  
  buttonSet_read = digitalRead(buttonSet_PIN);
  buttonSel_read = digitalRead(buttonSel_PIN);

  hA1 = EEPROM.read(romhA1),  mA1 = EEPROM.read(rommA1), alarmSet1 = EEPROM.read(romsetA1);

  
  if (keyboard.available()) { c = keyboard.read(); }
  //if(Serial.available()){ c = Serial.read(); }  
  
  buzzer_alarmOFF(romsetA1, alarmSet1, hA1, mA1);

  getSec();

  if(inputKey){readKeyboardA5();}

  //Serial.println(secTime);

  P.displayAnimate();

  if (P.getZoneStatus(0))
  {
    switch (display)
    {
      case 0: // Clock
         P.setFont(0, numeric7Seg);
         //P.setFont(0, pFont);
         P.setTextEffect(0, PA_OPENING, PA_NO_EFFECT);
         P.displayText(szMesg, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
         P.setPause(0,0);
    
         if (millis() - lastTime >= 1000)
            {
            lastTime = millis();
            getTime(szMesg, flasher);
            flasher = !flasher;
            setup_menu = 0;
            alarm_menu = 0;
            change_alarm_menu = false;

//            Serial.print(sTemp);
//            Serial.print(" ");
//            Serial.println(s);
            
            
//            if (((s-1) != sTemp) && s!=sTemp){
//              if(s !=0){
//                sTemp = s;
//                myRTC.setDS1302Time(s-1, m, h, 7, dd, mm, yy);
//                Serial.println("===========================");
//              }
              
//            }

            sTemp = s;
            
            }
            // Alarm 1
            else if(h==hA1 && m==mA1 && alarmSet1 == true){
              display=2;
              P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
            }
            // Button Set
            else if (buttonSet_read == LOW || c == PS2_ENTER ){
//            else if (buttonSet_read == LOW || c == 'm' ){
              c = 0;
              display=7;
              P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
            }
            // Button Sellect
            else if (buttonSel_read == LOW || c == ' ' ){
//            else if (buttonSet_read == LOW || c == 'm' ){
              c = 0;
              if (secTime == false) {
                secTime = true;
              } else {
                secTime = false;
              }
            }

            
        break;
      
      case 1:    // Temperature deg C
        P.setPause(0,1000);
        temp_C = Get_Temp(LM35_PIN);
        P.setTextEffect(0, PA_OPENING, PA_CLOSING);
        display=0;    
        dtostrf(temp_C, 3, 1, szMesg);
        strcat(szMesg, "$");
        break;
        
      case 2:   // Alarm 1
        alarm("Auniii");
        break;

      case 7:   // Setting
        if (millis() - lastTime >= 100){ lastTime = millis(); flasher = !flasher; }
        switch (setup_menu)
        {
          case 0: 
            alarm_menus("Setting", h, m, ' ', 100, 100, 100, flasher);
            break;
            
          case 1:
            alarm_menus("Alarm 1", hA1, mA1, alarmSet1, romhA1, rommA1, romsetA1, flasher);
            break;
        }
        break;
    }
    P.displayReset(0);  
  }
} 

void alarm_menus(const char *Alarm, int hA, int mA, boolean alarmSet, int romhA, int rommA, int romsetA, bool f = true){
   if (!change_alarm_menu){
      P.setFont(0, pFont);
      P.displayText(Alarm, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
      alarm_menu = 0;
      if (buttonSel_read == LOW){
        setup_menu = setup_menu + 1;
        if (setup_menu == 2) { setup_menu = 0; display = 0; }
        P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
      }
      else if (c == PS2_RIGHTARROW || c == PS2_UPARROW){
//      else if (c == 'd' || c == 's'){
        setup_menu = setup_menu + 1;
        if (setup_menu == 6) { setup_menu = 0;}
        P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
      }
      else if (c == PS2_LEFTARROW || c == PS2_DOWNARROW){
//      else if (c == 'a' || c == 'w'){
        setup_menu = setup_menu - 1;
        if (setup_menu < 0) { setup_menu = 1;}
        P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
      }
      else if (buttonSet_read == LOW || c == PS2_ENTER){
//      else if (buttonSet_read == LOW || c == 'k'){
        //time_pressed = millis(); c = 0;
        change_alarm_menu = true;
        P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);  
      }
      else if (c == PS2_ESC){
//      else if ( c == 'x'){
        display = 0;
        P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);  
      }
      c = 0;
    }
    else if (change_alarm_menu){
      switch(alarm_menu){
        case 0:
          P.setFont(0, numeric7Seg);
          //P.setFont(0, pFont);
          readKeyboardTime(hA);         
          sprintf(charh, "%02d", hA);
          sprintf(szMesg, "%s%c%02d", (f ? charh : "      "), ':', mA);
          P.displayText(szMesg, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
          time_choice(hA);
          menu_choice();
          break;
          
        case 1:
          P.setFont(0, numeric7Seg);
          //P.setFont(0, pFont);
          readKeyboardTime(mA);
          sprintf(charm, "%02d", mA);
          sprintf(szMesg, "%02d%c%s", hA, ':', (f ? charm : "      "));
          P.displayText(szMesg, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
          time_choice(mA);
          menu_choice();
          break;

        case 2:
          P.setFont(0, pFont);
          if(alarmSet){
            P.displayText("ON", PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
            if (buttonSel_read == LOW || c == PS2_LEFTARROW || c == PS2_RIGHTARROW || c == PS2_UPARROW || c == PS2_DOWNARROW){
//            if (buttonSel_read == LOW || c == 'a' || c == 'd' || c == 'w' || c == 's'){
              c = 0;
              alarmSet = false;
              P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);               
            }
          }
          else if (!alarmSet){
            P.displayText("OFF", PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
            if (buttonSel_read == LOW || c == PS2_LEFTARROW || c == PS2_RIGHTARROW || c == PS2_UPARROW || c == PS2_DOWNARROW){
//            if (buttonSel_read == LOW || c == 'a' || c == 'd' || c == 'w' || c == 's'){
              c = 0;
              alarmSet = true;
              P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);               
            }
          }
          menu_choice();
          break;

        case 3:
          readKeyboardA5();
          if(alarm5c[6] != 0) { inputKey = true; P.displayText(alarm5c, PA_CENTER, P.getSpeed(), P.getPause(), PA_SCROLL_LEFT, PA_SCROLL_LEFT); }
          else { P.displayText(alarm5c, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT); }
          break;
      }
    }
    EEPROM.update(romhA, hA);
    EEPROM.update(rommA, mA);
    EEPROM.update(romsetA, alarmSet);
}

void menu_choice(){
  if(buttonSet_read == LOW || c == PS2_ENTER){
//  if buttonSet_read == LOW || c == 'k'){
    c = 0;
    if(alarm_menu == 2 && setup_menu != 5 || alarm_menu == 3) { change_alarm_menu = false; }
    else if(setup_menu == 5 && alarm_menu == 2) { alarm_menu = alarm_menu + 1; }
    else if(setup_menu == 0 && alarm_menu == 1) { change_alarm_menu = false; }
    else if(alarm_menu < 2) { alarm_menu = alarm_menu + 1; } 
    P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);              
  }
  else if(c == PS2_DELETE){
//  else if(c == 'b'){
    c = 0;
    if(alarm_menu == 0) { change_alarm_menu = false; }
    else { alarm_menu = alarm_menu - 1; }
    P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);              
  }
  else if (c == PS2_ESC){
//  else if (c == 'x'){
    c = 0;
    display = 0;
    P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);              
  }
}

void time_choice(int &hOm){
  if ((millis() - time_pressed) > 100 && buttonSel_read == LOW || c == PS2_RIGHTARROW || c == PS2_UPARROW){
//  if ((millis() - time_pressed) > 100 && buttonSel_read == LOW || c == 'd' || c == 'w'){
    time_pressed = millis(); c = 0;
    if(setup_menu != 0){
      hOm = hOm + 1;
      if (alarm_menu == 0 && hOm >= 24) { hOm = 0; }
      else if (hOm >= 60) { hOm = 0; }
    }
    else { 
      if(alarm_menu == 0){ h = h + 1; if(h >= 24){ h = 0;}}
      else { m = m + 1; if(m >= 60) { m = 0; }}
      myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy);}
  }
  if(c == PS2_LEFTARROW || c == PS2_DOWNARROW){
//  else if(c == 'a' || c == 's'){
    c = 0;
    if(setup_menu != 0){
      hOm = hOm - 1;
      if (alarm_menu == 0 && hOm <= -1) { hOm = 23; }
      else if (hOm <= -1) { hOm = 59; }
    }
    else { 
      if(alarm_menu == 0){ h = h - 1; if(h == -1){h = 23;}}
      else { m = m - 1; if (m == -1) { m = 59; }}
      myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy);}
  } 
}

void readKeyboardTime(int &hOm){
  if(c == PS2_ENTER) {
//  if(c == 'k') {
    c = 0;
    if(setup_menu == 0 && alarm_menu == 1) { change_alarm_menu = false; }
    else {alarm_menu = alarm_menu + 1; }
  }
  else if(c == PS2_DELETE) {
//  else if(c == 'b') {
    c = 0;
    if(alarm_menu == 0) {change_alarm_menu = false;}
    else alarm_menu = alarm_menu - 1;
    hOmChar[indexhOm] = 0; hOmChar[indexhOm - 1] = 0; indexhOm = 0;
  }
  else if(c == PS2_ESC) {display = 0; c = 0; }
//  else if(c == 'x') {display = 0; c = 0; }
  else if(isDigit(c)){
    hOmChar[indexhOm] = c;
    hOm = atoi(hOmChar);
    if(hOmChar[indexhOm] != ' ') { 
      indexhOm = indexhOm + 1; c = 0; 
      if (alarm_menu == 0 && hOm >= 24) { hOm = 0; }
      else if (hOm >= 60) { hOm = 0; }
    }
    if(setup_menu == 0) {
      if(alarm_menu == 0) { 
        h = atoi(hOmChar); if(h >= 24){ h = 0;} myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy); }
      else {
        m = atoi(hOmChar); if(m >= 60) { m = 0; } myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy); }
    }
  }
  else if(indexhOm == 2) { hOmChar[indexhOm] = 0; hOmChar[indexhOm - 1] = 0; indexhOm = 0; }
}

void readKeyboardA5(){
  alarm5c[indexC] = c;
  if(buttonSet_read == LOW || c == PS2_ENTER) {alarm5c[indexC] = 0; c = 0;  change_alarm_menu = false; inputKey = false;}
  //else if(c == PS2_BACK) {alarm5c[indexC] = 0; c = 0;  alarm_menu = alarm_menu - 1;}
  else if(c == PS2_ESC) {alarm5c[indexC] = 0; c = 0;  display = 0; inputKey = false;}
  else if(c == PS2_DELETE) {alarm5c[indexC] = 0; indexC = indexC - 1; c = 0;}
  else if(alarm5c[indexC] != 0){ indexC = indexC + 1; c = 0;}
}

void alarm(const char *Alarm){
  P.setFont(0, pFont);
  P.displayText(Alarm, PA_CENTER, P.getSpeed(), 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void buzzer_alarmOFF(int romsetA, int alarmSet, int hA, int mA){
  static uint32_t lastTime = 0;
  unsigned long currentMillis = millis();
  if(h==hA && m==mA && alarmSet == true && display != 0 && display != 1 && display != 7){
    // buzzer
    if (currentMillis - previousMillis >= 100) { 
      previousMillis = currentMillis;   
      if (buzzState == LOW) {
        buzzState = HIGH;
      } else {
        buzzState = LOW;
      }
      digitalWrite(buzzer_PIN, buzzState);
    }
    // alarm OFF
    if (buttonSet_read == LOW || buttonSel_read == LOW || s>=59 || c != 0){
      c = 0;
      lastTime = millis();
      alarmSet = false;
      display=0;
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);  
      buzzState = LOW;
      digitalWrite(buzzer_PIN, buzzState);
    }
  }
  EEPROM.update(romsetA, alarmSet);
}

byte ledIntensitySelect(int light) {
  byte _value = 0;
  if (light >= 0 && light <= 300) {
    _value = 0;
  } else if (light >= 301 && light <= 750) {
    _value = 6; 
  } else if (light >= 751) {
    _value = 15;
  }
  return _value;
};
