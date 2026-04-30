#include <LiquidCrystal.h>
#include <DHT.h>
//#include <LiquidCrystal_I2C.h>

#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//LiquidCrystal_I2C lcd(0x27, 16, 2);
LiquidCrystal lcd(48, 46, 41, 40, 39, 38);

// HARDWARE PINS
#define TRIG 9
#define ECHO 4

#define PIR 3

#define LED_RED 8
#define LED_GREEN 11
#define LED_YELLOW 12

#define JOY_X A12
#define JOY_Y A13
#define JOY_SW 28

#define BUZZER 5

#define LM35_PIN A2
#define TMP36_PIN A3

#define LDR_PIN A4   // photoresistor (light) - read 0..1023 -> map to 0..255

// MENU constants
#define NORMAL 0
#define GUARDIAN 1
#define SHUTDOWN 2

// menu items
const String menuItems[3] = {"Normal Mode", "Guardian Mode", "Shutdown"};

// MENU state
int selectedMenu = 0;     // 0..2
int mode = -1;            // -1 = menu, otherwise mode constants
unsigned long lastNav = 0;

// PIN entry
int pinDigits[4] = {0,0,0,0};
int pinPos = 0;
bool enteringPIN = false;
bool enteringLogoutPIN = false;
const String shutdownPIN = "1234";
const String logoutPIN   = "1234";
String enteredPIN = "";

// guardian
bool guardianAlarm = false, ledState = false;
unsigned long blinkTimer = 0;

String lastLine1 = "", lastLine2 = "";
void lcdPrintSmooth(String l1, String l2) {
  l1 = (l1 + "                ").substring(0, 16);
  l2 = (l2 + "                ").substring(0, 16);

  if (l1 != lastLine1) {
    lcd.setCursor(0, 0);
    lcd.print(l1);
    lastLine1 = l1;
  }
  if (l2 != lastLine2) {
    lcd.setCursor(0, 1);
    lcd.print(l2);
    lastLine2 = l2;
  }
}

void clearDisplay(){
  lastLine1 = "", lastLine2 = "";
}

//Static startup
void static_startup() {
  lcdPrintSmooth("   Blume Arc   ", "");
  clearDisplay();
}

// HC-SR04 smoothing
float distBuffer[5] = {1000,1000,1000,1000,1000};
int distIndex = 0;
float filteredDistance(){
  float sum = 0;
  for(int i = 0; i < 5; i++) sum += distBuffer[i];
  return sum / 5.0;
}

// distance
float measureDistance(){
  //Generate 10 microSecond Pulse
  digitalWrite(TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 30000); // timeout
  float distance = 9999; // no object
  if(duration > 0) distance = duration * 0.0343 / 2.0;
  distBuffer[distIndex] = distance;
  distIndex = (distIndex + 1) % 5;
  return filteredDistance();
}

// median of three floats
float median3(float a, float b, float c){
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  else if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  else return c;
}

// draw menu with rotating/scrolling effect
void drawMenu() {
    int idxTop = selectedMenu, idxBottom = (selectedMenu + 1) % 3;

    String top = (selectedMenu == idxTop ? "> " : "  ") + menuItems[idxTop], 
    bottom = (selectedMenu == idxBottom ? "> " : "  ") + menuItems[idxBottom];

    lcdPrintSmooth(top, bottom);
}

// shutdown confirm
bool shutdownChoice = false; // 0 = yes, 1 = no
void drawShutdownConfirm(){
  String l1 = "Shutdown?", l2 = (shutdownChoice == false ? ">Yes   No" : " Yes  >No");
  lcdPrintSmooth(l1, l2);
}

// ---------- Normal mode submenu state ----------
int normalSelection = 0; // 0=temp, 1=humidity, 2=light
unsigned long navDebounce = 0;

// read joystick directions & button
void readJoystick(bool& up, bool& down, bool& left, bool& right, bool& btn){
  int x = analogRead(JOY_X), y = analogRead(JOY_Y);
  btn = (digitalRead(JOY_SW) == LOW); //pull-up
  const int upT = 300, downT =  700, leftT = 300, rightT = 700;
  up = (y < upT);
  down = (y > downT);
  left = (x < leftT);
  right = (x > rightT);
}

// Global variable to count wrong logout PIN attempts
int pin_counter = 0;
bool waitForRelease = false;
unsigned long lastPinNav = 0;
const unsigned long pinNavDelay = 200;
void handlePinEntry(bool up, bool down, bool left, bool right, bool btn, String correctPin, bool isLogout){
    unsigned long now = millis();
    if(now - lastPinNav > pinNavDelay){
        if(up)    pinDigits[pinPos] = (pinDigits[pinPos] + 1) % 10;
        if(down)  pinDigits[pinPos] = (pinDigits[pinPos] + 9) % 10;
        if(right) pinPos = (pinPos + 1) % 4;
        if(left)  pinPos = (pinPos + 3) % 4;

        if(up || down || left || right){
            lastPinNav = now;
        }
    }

    // Display current PIN digits
    String line = "";
    for(int i=0;i<4;i++){
        if(i == pinPos) line += "[" + String(pinDigits[i]) + "] ";
        else line += " " + String(pinDigits[i]) + "  ";
    }
    lcdPrintSmooth("Enter PIN:", line);

    // Button pressed edge detection
    if(btn){
        if(!waitForRelease){  // only act on first press
            String entered = "";
            for(int i=0;i<4;i++) entered += String(pinDigits[i]);

            if(entered == correctPin){
                lcdPrintSmooth("PIN OK","Access granted");
                delay(600);
                if(isLogout){
                    enteringLogoutPIN = false;
                    guardianAlarm = false;
                    pin_counter = 0;
                    mode = -1;
                } else {
                    lcdPrintSmooth("Shutting","Down...");
                    delay(800);
                    while(true);
                }
                digitalWrite(LED_RED, LOW);
                digitalWrite(LED_YELLOW, LOW);
                digitalWrite(LED_GREEN, LOW);
                noTone(BUZZER);
            } else {
                lcdPrintSmooth("Wrong PIN","Try again");
                delay(800);

                if(isLogout){
                    pin_counter++;
                    if(pin_counter >= 3){
                        guardianAlarm = true;
                        enteringLogoutPIN = false;
                        mode = GUARDIAN;
                        digitalWrite(LED_RED, HIGH);
                        digitalWrite(LED_GREEN, LOW);
                        digitalWrite(LED_YELLOW, LOW);
                        tone(BUZZER, 1000);
                    }
                }
            }

            // Reset PIN digits
            for(int i=0;i<4;i++) pinDigits[i] = 0;
            pinPos = 0;

            if(isLogout){
                if(!guardianAlarm) enteringLogoutPIN = true;
            } else {
                enteringPIN = false;
            }

            waitForRelease = true; 
        }
    } else {
        waitForRelease = false;
    }
}

// ---------- SETUP ----------
void setup(){
  Serial.begin(9600);
  //DHT sensor
  dht.begin();
  //Liquid Crystal Display
  //lcd.init();
  //lcd.backlight();
  lcd.begin(16, 2);
  //Ultrasonic sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  //PIR motion sensor
  pinMode(PIR, INPUT);
  //Main three LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  //Joystick
  pinMode(JOY_SW, INPUT_PULLUP);
  //Buzzer
  pinMode(BUZZER, OUTPUT);
  // Startup animation
  static_startup();
  delay(2000);
  lcdPrintSmooth("Ready","Use joystick");
  Serial.print("ready");
  delay(800);
  clearDisplay();
}

// ---------- LOOP ----------
void loop(){

  float lm35_temp = analogRead(LM35_PIN) * (5.0 / 1023.0) * 100.0; // LM35: 10mV/°C -> V *100
  float tmp36_volt = analogRead(TMP36_PIN) * (5.0 / 1023.0);
  float tmp36_temp = (tmp36_volt - 0.5) * 100.0;
  float dht_temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  int rawLdr = analogRead(LDR_PIN);
  int lightVal = map(rawLdr, 0, 1023, 0, 255);
  float distance = measureDistance();
  int motion = digitalRead(PIR);

  bool up=false, down=false, left=false, right=false, btn=false;
  readJoystick(up, down, left, right, btn);

  unsigned long now = millis();

  // ======= SHUTDOWN PIN ENTRY =======
  if (enteringPIN) {
      handlePinEntry(up, down, left, right, btn, shutdownPIN, false);
      return;
  }

  // ======= LOGOUT PIN ENTRY =======
  if (enteringLogoutPIN) {
      handlePinEntry(up, down, left, right, btn, logoutPIN, true);
      return;
  }

  // ----------------- MAIN MENU MODE (mode == -1) -----------------
  if(mode == -1){
    if(now - lastNav > 180){
      if(up){
        selectedMenu = (selectedMenu + 2) % 3;
        lastNav = now;
      } else if(down){
        selectedMenu = (selectedMenu + 1) % 3;
        lastNav = now;
      } else if(right){
        lastNav = now;
        if(selectedMenu == 2){
          shutdownChoice = false;
        }
        if(selectedMenu == 0){
          mode = NORMAL;
          normalSelection = 0;
          clearDisplay();
          return;
        } else if(selectedMenu == 1){
          mode = GUARDIAN;
          clearDisplay();
          return;
        }
      }
    }

    // we need to handle the special case of shutdown confirm if right was pressed on Shutdown; let's implement
    static bool inShutdownConfirm = false;
    if(inShutdownConfirm == false && right && now - lastNav <= 300 && selectedMenu == 2){
      inShutdownConfirm = true;
      shutdownChoice = false;
      lastNav = now;
    }

    if(inShutdownConfirm){
      drawShutdownConfirm();
        if(now - lastNav > 180){
            if(left){
                shutdownChoice = !shutdownChoice;
                lastNav = now;
            } else if(right){
                shutdownChoice = !shutdownChoice;
                lastNav = now;
            }
        }

      if(btn && now - lastNav > 250){
        lastNav = now;
        if(shutdownChoice == false){
          enteringPIN = true;
          enteredPIN = "";
          waitForRelease = true;
          clearDisplay();
        }
        inShutdownConfirm = false;
      }

      return;
    }
    drawMenu();
    return;
  } // end menu

  // ----------------- ACTIVE MODES -----------------

  // GUARDIAN behavior
  if(mode == GUARDIAN){

    if (guardianAlarm) {
      tone(BUZZER, 1000);
      if (millis() - blinkTimer >= 500) {
        blinkTimer = millis();
        ledState = !ledState;
      }
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, ledState ? HIGH : LOW);
      digitalWrite(LED_GREEN, LOW);
      lcdPrintSmooth("ALARM! Wrong PIN", "Dist:" + String(distance, 1) + "cm");
    }
    else {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      noTone(BUZZER);
      lcdPrintSmooth("Guardian Mode", "Dist:" + String(distance, 1) + "cm");
    }

    if(!guardianAlarm && (distance < 30 || motion)){
      tone(BUZZER, 1000);
      if (millis() - blinkTimer >= 500) {
        blinkTimer = millis();
        ledState = !ledState;
      }
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, ledState ? HIGH : LOW);
      digitalWrite(LED_GREEN, LOW);
    }

    if(left && now - navDebounce > 300){
      enteringLogoutPIN = true;
      enteredPIN = "";
      navDebounce = now;
      clearDisplay();
    }

    return;
  }

  // NORMAL mode
  if(mode == NORMAL){
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    noTone(BUZZER);

      String top, bottom;
      float accurateTemp;
      if (isnan(dht_temp))
        accurateTemp = median3(lm35_temp, tmp36_temp, lm35_temp);
      else
        accurateTemp = median3(lm35_temp, tmp36_temp, dht_temp);

      String tempStr = "T:" + String(accurateTemp,1) + "C";

      String humidityStr = "H:" + (isnan(humidity) ? String(0,1) : String(humidity,1)) + "%";
      String lightStr = "Light:" + String(lightVal);

      // item strings
      String items[3];
      items[0] = tempStr;
      items[1] = humidityStr;
      items[2] = lightStr;

      int topIdx = normalSelection;
      int bottomIdx = (normalSelection + 1) % 3;
      top = ("> " + items[topIdx]);
      bottom = ("  " + items[bottomIdx]);
      
      if(normalSelection == topIdx) top = "> " + items[topIdx];
      else top = "  " + items[topIdx];
      if(normalSelection == bottomIdx) bottom = "> " + items[bottomIdx];
      else bottom = "  " + items[bottomIdx];

      lcdPrintSmooth(top, bottom);

      if(left && now - navDebounce > 220){
        mode = -1;
        navDebounce = now;
        clearDisplay();
        return;
      }

      return;
  } // end NORMAL

}