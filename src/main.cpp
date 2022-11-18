// Dependencies
#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#include <ESPAsyncWebServer.h>

#include <WiFi.h>

#include <Stepper.h>

// Hard coded definitions (pins and values)
#define NUMPIXELS 3
#define relayPin 15
#define touchPin 4
#define touchSensitivity 10
#define hallPin 34
const int stepsPerRevolution = 2048; // change this to fit the number of steps per revolution

// ULN2003 Motor Driver Pins
#define IN1 21
#define IN2 19
#define IN3 18
#define IN4 5

bool theShow = false;
bool waterLevel = false;
bool manualMode = false;
bool relayState = false;
bool stateChange = false; // turns true upon async call - ensures the relay initializes in ON state of the cycle (always)
bool waterVal = false;

byte color = 0;
byte brightness = 200;
byte touchVal = 0;
short rotation = -1;

String hexString = "Color(0xff00ff57)";
int alphaVal = 200;
int hexVal = 16516088;

unsigned short manualInterval = 30000; // life of manualMode - in sync with timeout on App as well. App says - You will kill me!

unsigned long previousMillis2 = 0; // will store last time LED was updated
unsigned long OnTime = 3000; // defualt fog cycle
unsigned long OffTime = 6000 - OnTime;

const char * wifi_network_ssid = "Induct Technologies";
const char * wifi_network_password = "maas-1004";
const char * soft_ap_ssid = "FarmHouse Pod 3";
const char * soft_ap_password = "maas-1004";

// Initialize neopixels
Adafruit_NeoPixel pixels(NUMPIXELS, 14, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80); // accessible on port 80
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

void serverCalls() {
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send(200, "text/plain", "Connection sweet");
  });

  server.on("/mg", HTTP_GET, [](AsyncWebServerRequest * request) {
    color = 0;
    request -> send(200, "text/plain", "magenta");
  });

  server.on("/cy", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send(200, "text/plain", "cyan");
    color = 1;
  });

  /**
   * Sets LED state either ON or OFF
   *
   * Usage "/led?state=true"
   * true = ON
   * false = 0FF
   */
  server.on("/led", HTTP_GET, [](AsyncWebServerRequest * request) {
    bool state = request -> getParam("state") -> value() == "true";
    color = state ? 0 : 2;
    request -> send(200, "text/plain", state ? "NEO ON" : "NEO OFF");
  });

  server.on("/hex", HTTP_GET, [](AsyncWebServerRequest * request) {

    hexString = request -> getParam("hexCode") -> value();
    alphaVal = strtol(hexString.substring(8, 10).c_str(), NULL, 16);
    hexVal = strtol(hexString.substring(10, 16).c_str(), NULL, 16);

    request -> send(200, "text/plain", "magenta");
  });

  /**
   * Sets mode either true or false
   *
   * Usage "/mode?data=true"
   *
   */
  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest * request) {
    manualMode = request -> getParam("data") -> value() == "true" ? true : false;
    request -> send(200, "text/plain", "Mini now in manual mode");
  });

  /**
   * Changes the "rotation" value according to "spinValue"
   *
   * Usage "/spin?spinValue=1"
   *
   */
  server.on("/spin", HTTP_GET, [](AsyncWebServerRequest * request) {

    int spinValue = request -> getParam("spinValue") -> value().toInt();
    switch (spinValue) {
    case 2:
      rotation = 1;
      request -> send(200, "text/plain", "clockwise");
      break;

    case 0:
      rotation = -1;
      request -> send(200, "text/plain", "anti-clockwise");
      break;

    case 1:
      rotation = 0;
      request -> send(200, "text/plain", "halt");
      break;

    default:
      request -> send(406, "text/plain", "invalid parameter");
      break;
    }
  });

  /**
   * Change relayState and set cycle for N minutes
   *
   * Usage "/fog?fogValue=1"
   *
   */
  server.on("/fog", HTTP_GET, [](AsyncWebServerRequest * request) {
    int fogValue = request -> getParam('fogValue') -> value().toInt();

    switch (fogValue) {
    case 1:
      relayState = 0;
      request -> send(200, "text/plain", "relay on");
      break;

    case 11:
      relayState = 1;
      request -> send(200, "text/plain", "relay off");
      break;

    case 0:
      OnTime = 0;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "0 min cycle set");
      break;

    case 2:
      OnTime = 120000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "2 min cycle set");

    case 4:
      OnTime = 240000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "4 min cycle set");
      break;

    case 6:
      OnTime = 360000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "6 min cycle set");
      break;

    case 8:
      OnTime = 480000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "8 min cycle set");
      break;

    case 10:
      OnTime = 600000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "10 min cycle set");
      break;

    case 12:
      OnTime = 720000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "12 min cycle set");
      break;

    case 14:
      OnTime = 840000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "14 min cycle set");
      break;

    case 16:
      OnTime = 960000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "16 min cycle set");
      break;

    case 18:
      OnTime = 1080000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "18 min cycle set");
      break;

    case 20:
      OnTime = 1200000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request -> send(200, "text/plain", "20 min cycle set");
      break;
    default:
      break;
    }
  });
}

void wifiSetup() {
  WiFi.begin(wifi_network_ssid, wifi_network_password);

  // check if connection established in 5 sex, blink red led in the meantime
  while (WiFi.status() != WL_CONNECTED && millis() <= 5000) {
    Serial.println("Connecting to WiFi..");
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.softAP(soft_ap_ssid, soft_ap_password);
    Serial.print("Connect to Mini via WiFi: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.print("Mini connected to WiFI: ");
    Serial.println(WiFi.localIP());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(hallPin, INPUT);
  myStepper.setSpeed(5);
  wifiSetup(); // Establish connection via WiFi

  serverCalls(); // Async request handler

  server.begin();
}

void pixelColor(void) {
  pixels.setBrightness(brightness);
  switch (color) {
  case 0: // Magenta
    pixels.fill(pixels.Color(255, 0, 255));
    pixels.show();
    break;
  case 1: // CYAN
    pixels.fill(pixels.Color(0, 255, 255));
    pixels.show();
  case 2: // led OFF
    pixels.clear();
    pixels.show();
    break;
  case 10: // RED
    pixels.fill(pixels.Color(255, 5, 5));
    pixels.show();
    break;
  }
}

void waterAlert() {
  manualMode = false; // get out of Manual mode
  digitalWrite(relayPin, HIGH); // turn relay OFF if ON
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(brightness);
  for (int i = 255; i >= 0; i -= 5) {
    pixels.setBrightness(i);
    pixels.fill(pixels.Color(255, 5, 5));
    pixels.show();
    delay(5);
  }
  delay(200);
  for (int i = 0; i < 255; i += 5) {
    pixels.setBrightness(i);
    pixels.fill(pixels.Color(255, 5, 5));
    pixels.show();
    delay(5);
  }
  color = 10;
}

void miniShow() {
  digitalWrite(relayPin, LOW); // turn relay ON
  pixels.setBrightness(100);
  for (int i = 0; i < 5; i++) {
    pixels.fill(pixels.Color(255, 255, 255));
    pixels.show();
    delay(100);
    pixels.clear();
    pixels.show();
    delay(100);
  }
  digitalWrite(relayPin, HIGH); // turn relay OFF before exiting the function
}

void manualModeF() {
  int previousMillis = 0;
  previousMillis = millis();

  pixels.setBrightness(alphaVal);
  pixels.fill(hexVal, 0, NUMPIXELS);
  pixels.show();

  if (relayState)
    digitalWrite(relayPin, HIGH);
  else
    digitalWrite(relayPin, LOW);

  myStepper.step(rotation);
  Serial.println("man mode");
  Serial.print("Relay state :");
  Serial.println(relayState);
}

void loop() {

  // check for water level
  waterLevel = digitalRead(hallPin);

  if (!waterLevel) {
    if (touchVal < touchSensitivity) {
      miniShow();
    }
    touchVal = touchRead(touchPin); // get the touch value
    if (manualMode) {
      manualModeF();
    } else {

      Serial.println("auto mode");
      myStepper.step(rotation);

      // growlight-like light spectrum
      pixels.setBrightness(200);
      pixels.fill(pixels.Color(255, 0, 255));
      pixels.show();

      // relay cycle
      unsigned long currentMillis = millis();

      if ((relayState == false) && (currentMillis - previousMillis2 >= OnTime) && !stateChange) {
        relayState = HIGH; // Turn relay off
        previousMillis2 = currentMillis; // Remember the time
        digitalWrite(relayPin, relayState); // Update the actual LED
        stateChange = false; // set stateChange to false
      } else if ((relayState == true) && (currentMillis - previousMillis2 >= OffTime) || stateChange) {
        relayState = LOW; // Turn relay on
        previousMillis2 = currentMillis;
        digitalWrite(relayPin, relayState);
        stateChange = false;
      }
      // myStepper.step(rotation);
      // Serial.println(manualMode);
    }
  }
  // if water level is low, raise alert
  else
    waterAlert();
}