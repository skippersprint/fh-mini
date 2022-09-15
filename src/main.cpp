// Dependencies
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Stepper.h>

// Hard coded definitions (pins and values)
#define NUMPIXELS 1 
#define relayPin 15
#define touchPin 4
#define touchSensitivity 10
#define hallPin 34
const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution

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

unsigned short manualInterval = 30000;  // life of manualMode

unsigned long previousMillis2 = 0;        // will store last time LED was updated
unsigned long OnTime = 10000;           // defualt fog cycle
unsigned long OffTime = 20000  - OnTime;

const char *wifi_network_ssid = "hotispot";
const char *wifi_network_password = "maas-1004";
const char *soft_ap_ssid = "FarmHouse Pod 3";
const char *soft_ap_password = "maas-1004";

// Initialize neopixels
Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80); // accessible on port 80
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

void serverCalls()
{
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) { 
    request->send(200, "text/plain", "Connection sweet"); 
    });

  server.on("/ron", HTTP_GET, [](AsyncWebServerRequest *request) {
    relayState = 0;
    request->send(200, "text/plain", "relay on"); 
  }); // inverted logic (Common anode)

  server.on("/roff", HTTP_GET, [](AsyncWebServerRequest *request) {
    relayState = 1;
    request->send(200, "text/plain", "relay off"); 
  });

  server.on("/mg", HTTP_GET, [](AsyncWebServerRequest *request) {
    color = 0;
    request->send(200, "text/plain", "magenta"); 
  });

  server.on("/cy", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "cyan");
    color = 1;
  });

  server.on("/ledon", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "NEO ON");
    color = 0; 
  });

  server.on("/ledoff", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "NEO OFF");
    color = 2; 
  });

  server.on("/manualMode", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Control Mini manually");
    manualMode = true; 
  });

  server.on("/autoMode", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Mini now in auto mode");
    manualMode = false; 
  });
    
  server.on("/ron", HTTP_GET, [](AsyncWebServerRequest *request) {
    relayState = 0;
    request->send(200, "text/plain", "relay on"); 
  });

  server.on("/turnC", HTTP_GET, [](AsyncWebServerRequest *request) {
    rotation = 1;
    request->send(200, "text/plain", "clockwise"); 
  });

  server.on("/turnA", HTTP_GET, [](AsyncWebServerRequest *request) {
    rotation = -1;
    request->send(200, "text/plain", "anti-clockwise"); 
  });

  server.on("/turnoff", HTTP_GET, [](AsyncWebServerRequest *request) {
    rotation = 0;
    request->send(200, "text/plain", "OFF"); 
  });

  server.on("/cycle10", HTTP_GET, [](AsyncWebServerRequest * request) {
    OnTime = 600000;
    OffTime = 1200000 - OnTime;
    stateChange = true;
    request->send(200, "text/plain", "10 min cycle set");
  });

  server.on("/cycle6", HTTP_GET, [](AsyncWebServerRequest * request) {
      OnTime = 360000;
      OffTime = 1200000 - OnTime;
      stateChange = true;
      request->send(200, "text/plain", "6 min cycle set");
  });

  server.on("/10s", HTTP_GET, [](AsyncWebServerRequest * request) {
    OnTime = 5000;
    OffTime = 10000 - OnTime;
    stateChange = true;
    request->send(200, "text/plain", "6 min cycle set");
  });
  
}

void wifiSetup()
{
  WiFi.begin(wifi_network_ssid, wifi_network_password);

  // check if connection established in 5 sex, blink red led in the meantime
  while (WiFi.status() != WL_CONNECTED && millis() <= 5000)
  {
    Serial.println("Connecting to WiFi..");
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.softAP(soft_ap_ssid, soft_ap_password);
    Serial.print("Connect to Mini via WiFi: ");
    Serial.println(WiFi.softAPIP());
  }

  else
  {
    Serial.print("Mini connected to WiFI: ");
    Serial.println(WiFi.localIP());
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(hallPin, INPUT);
  myStepper.setSpeed(5);
  wifiSetup(); // Establish connection via WiFi

  serverCalls(); //Async request handler

  server.begin();
}

void pixelColor(void)
{
  pixels.setBrightness(brightness);
  switch (color){
    case 0:  // Magenta
      pixels.fill(pixels.Color(255, 0, 255));
      pixels.show();
      break;
    case 1:  // CYAN
      pixels.fill(pixels.Color(0, 255, 255));
      pixels.show();
    case 2:  // led OFF
       pixels.clear();
       pixels.show();
       break;
    case 10:  // RED
        pixels.fill(pixels.Color(255, 5, 5));
        pixels.show();
        break;
  }
}

void waterAlert()
{
  manualMode = false; // get out of Manual mode
  digitalWrite(relayPin, HIGH); // turn relay OFF if ON
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(brightness);
  for (int i = 255; i >= 0; i -= 5)
  {
    pixels.setBrightness(i);
    pixels.fill(pixels.Color(255, 5, 5));
    pixels.show();
    delay(5);
  }
  delay(200);
  for (int i = 0; i < 255; i += 5)
  {
    pixels.setBrightness(i);
    pixels.fill(pixels.Color(255, 5, 5));
    pixels.show();
    delay(5);
  }
  color = 10;
}

void miniShow()
{
  digitalWrite(relayPin, LOW); // turn relay ON
  pixels.setBrightness(100);
  for (int i = 0; i < 5; i++)
  {
    pixels.fill(pixels.Color(255, 255, 255));
    pixels.show();
    delay(100);
    pixels.clear();
    pixels.show();
    delay(100);
  }
  digitalWrite(relayPin, HIGH); // turn relay OFF before exiting the function
}

void manualModeF()
{
  int previousMillis = 0;
  previousMillis = millis();

  while (millis() <= previousMillis + manualInterval && manualMode == true && !digitalRead(hallPin))
  {
    pixelColor();
    if (relayState)
      digitalWrite(relayPin, HIGH);
    else
      digitalWrite(relayPin, LOW);
    
    myStepper.step(rotation);
  }
  
  manualMode = false;
  return;
  }

void loop() {

  // check for water level
  waterLevel = digitalRead(hallPin);

if (!waterLevel) {
  color = 1;
  if(manualMode) 
    manualModeF();
  if (touchVal < touchSensitivity)
    miniShow();

  touchVal = touchRead(touchPin); // get the touch value
  // growlight-like light spectrum
  pixels.setBrightness(200);
  pixels.fill(pixels.Color(255, 0, 255));
  pixels.show();

  // relay cycle
  unsigned long currentMillis = millis();

 if((relayState == false) && (currentMillis - previousMillis2 >= OnTime) && !stateChange)
  {
    relayState = HIGH;  // Turn relay off
    previousMillis2 = currentMillis;  // Remember the time
    digitalWrite(relayPin, relayState);  // Update the actual LED
    stateChange = false; // set stateChange to false
  }
  else if ((relayState == true) && (currentMillis - previousMillis2 >= OffTime)  || stateChange)
  {
    relayState = LOW;  // Turn relay on
    previousMillis2 = currentMillis;  
    digitalWrite(relayPin, relayState);   
    stateChange = false;
  }
  Serial.println(rotation);
  myStepper.step(rotation);
}   
// if water level is low, raise alert
else 
  waterAlert();
}

