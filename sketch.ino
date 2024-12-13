#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define green_led 2 // Pin used to control the green LED
#define red_led 40 // Pin used to control the red LED
#define yellow_led 9 // Pin used to control the yellow LED

#define ldr_pin 4 // Pin used to control the LDR
const int buttonPin = 18;  // the number of the pushbutton pin
int buttonState = 0;  // variable for reading the pushbutton status

const int ldrPin = 4;  // the number of the ldr pin
int threshold = 600;

long* millisList = new long[10];
long* waitingList = new long[10];
bool nightMode = false;
bool manualControl = false;
int pressCount = 0;

int state = 0;

void blinkLedWithMillis(int pin, long delay, int millisId);
void setup() {

  // Initial configuration of the pins to control the LEDs as OUTPUTs of the ESP32
  pinMode(green_led, OUTPUT);
  pinMode(red_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);

  // Input initialization
  pinMode(buttonPin, INPUT); // Initialize the pushbutton pin as an input

  digitalWrite(green_led, LOW);
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, LOW);

  Serial.begin(9600); // Configuration for debugging via serial interface between ESP and computer with a baud rate of 9600

  WiFi.begin("Wokwi-GUEST", "", 6);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(100);
  //   Serial.print(".");
  // }
  Serial.println("Successfully connected to WiFi!"); // Considering it exited the loop above, the ESP32 is now connected to WiFi (another option is to place this command inside the if below)
}

void loop() {

  int ldrstatus = 4063 - analogRead(ldrPin);

  if(ldrstatus <= threshold){
    Serial.print("its dark turn on led | LDR: ");
    Serial.println(ldrstatus);
    blinkLedWithMillis(yellow_led, 1000, 0); 
    nightMode = true;

  } else if(nightMode) {
    nightMode = false;
    Serial.print("its bright turn off light | LDR: ");
    Serial.println(ldrstatus);
    digitalWrite(yellow_led, LOW);
  }

  if(!nightMode) {
    // Check button state
    buttonState = digitalRead(buttonPin);
    // implementing debounce with millisList, wait 500ms to execute again
    if (state == 1 && buttonState == HIGH && millis() - millisList[2] >= 500) { 
      millisList[2] = millis();
      state = 1;
      pressCount = pressCount + 1;
      waitingList[1] = 1000;
    }

    if(pressCount >= 3) {
      pressCount = 0;
      Serial.println("Enviando alerta");
      if (WiFi.status() == WL_CONNECTED) { // If the ESP32 is connected to the Internet
        
        Serial.println("Conectado, pronto para enviar o alerta");
        // if connected send http request to www.google.com
        HTTPClient http;
        http.begin("http://www.google.com");
        int httpCode = http.GET();
        if (httpCode > 0) {
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);
          if (httpCode == HTTP_CODE_OK) {
            Serial.println("Alerta enviado");
          }
        } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
      } else {
        Serial.println("WiFi Disconnected");
      }
    }

    verifyState();
  } else {
    digitalWrite(red_led, LOW);
    digitalWrite(green_led, LOW);
  }
}

void verifyState() {
  unsigned long currentMillis = millis();

  if(currentMillis - millisList[1] <= waitingList[1]) { return; }

  Serial.print(currentMillis);
  Serial.print(" - ");
  Serial.print(millisList[1]);
  Serial.print(" <= ");
  Serial.println(waitingList[1]);

  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, LOW);

  if(state == 0) {
    millisList[1] = currentMillis;
    waitingList[1] = 5000;
    state = 1;

    digitalWrite(red_led, HIGH);
    return;
  }

  if(state == 1) {
    millisList[1] = currentMillis;
    waitingList[1] = 3000;
    state = 2;

    digitalWrite(green_led, HIGH);
    return;
  }

  if(state == 2) {
    millisList[1] = currentMillis;
    waitingList[1] = 2000;
    state = 0;

    digitalWrite(yellow_led, HIGH);
    return;
  }
}

void blinkLedWithMillis(int pin, long delay, int millisId) {
  unsigned long currentMillis = millis();

  if (currentMillis - millisList[millisId] >= delay) {
    millisList[millisId] = currentMillis;
    digitalWrite(pin, !digitalRead(pin));
  }
}
