#include <Arduino.h>

// Dependencies
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Stepper.h>

#define NUMPIXELS 1
#define relayPin 2
#define touchPin 4
#define touchSensitivity 10
#define hallPin 34

bool theShow = false;
bool waterLevel;
bool manualMode = false;
byte color = 0;
byte relayState = 0;
byte brightness = 80;

int touchVal = 0;
int manualInterval = 10000;

const char *wifi_network_ssid = "hotispot";
const char *wifi_network_password = "maas-1004";
const char *soft_ap_ssid = "FarmHouse Pod 3";
const char *soft_ap_password = "maas-1004";

Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);

void serverCalls()
{
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Connection sweet"); });

  server.on("/ron", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  relayState = 1;
    request->send(200, "text/plain", "relay on"); });
  server.on("/roff", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  relayState = 0;
    request->send(200, "text/plain", "relay off"); });

  server.on("/mg", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    color = 0;
    request->send(200, "text/plain", "magenta"); });
  server.on("/cy", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  
    request->send(200, "text/plain", "cyan");
    color = 1; });
  server.on("/ledon", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "NEO ON");
    color = 0; });
  server.on("/ledoff", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "NEO OFF");
    color = 2; });
  server.on("/manualMode", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "Control Mini manually");
    manualMode = true; });
  server.on("/autoMode", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "Mini now in auto mode");
    manualMode = false; });
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

  wifiSetup();

  serverCalls();

  server.begin();
}

void pixelColor(void)
{

  pixels.setBrightness(200);
  if (color == 0)
  {
    pixels.fill(pixels.Color(255, 0, 255));
    pixels.show();
  }
  else if (color == 1)
  {
    pixels.fill(pixels.Color(0, 255, 255));
    pixels.show();
  }
  else if (color == 2)
  {
    pixels.clear();
    pixels.show();
  }
  else if (color == 10)
  {
    pixels.fill(pixels.Color(255, 5, 5));
    pixels.show();
  }
}

void waterAlert()
{
  // NEOPIXELS INTRO
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
}

void manualModeF()
{
  Serial.println("in mm");
  int previousMillis = 0;
  previousMillis = millis();

  while (millis() <= previousMillis + manualInterval && manualMode == true)
  {
    Serial.println("in mm loop");
    pixelColor();
    if (relayState == 1)
      digitalWrite(relayPin, HIGH);
    else
      digitalWrite(relayPin, LOW);
  }
  manualMode = false;
  return;
}

void loop()
{

  // ideal run -->
  waterLevel = digitalRead(hallPin);

  // if water level is low, then you can't to the magic show
  if (waterLevel == 1)
    waterAlert();
  else if (touchVal < touchSensitivity)
    miniShow();

  touchVal = touchRead(touchPin); // get the touch value
  // growlight-like light spectrum
  pixels.setBrightness(200);
  pixels.fill(pixels.Color(255, 0, 255));
  pixels.show();
  // relay and light cycle

  // manual controls (limited) -->
  if (manualMode)
    manualModeF();
}
