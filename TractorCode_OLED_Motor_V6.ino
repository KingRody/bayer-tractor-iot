/* ESP8266 Bayer Tractor AWS IoT MQTT

   Author: Peter Yves Ruland | Thinkport GmbH
*/

// #include "secrets_pyr.h"
#include "secrets_dev_wework.h"
#include <Wire.h> // we connect OLED via I2C 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>


// lets create a variable for the tractor name project as we will use it multiple times
const char* projectname = "ESP8266 Bayer Tractor AWS IoT V2";

// Pin definition
// Motor control
const int controlPin1A = RX;  // L293D driver input 1A on pin 2 connected to digital output pin RX
const int controlPin2A = D3;  // L293D driver input 2A on pin 7 connected digital output pin D3
const uint8_t ENablePin = TX; // L293D ENable(1,2) input on pin 1 digital output pin TX
// Servo control
const int controlPin3A = D6;  // L293D driver input 3A on pin 10 connected to digital output pin D6
const int controlPin4A = D5;  // L293D driver input 4A on pin 15 connected to digital output pin D5
const uint8_t servoENablePin = D7;  // L293D ENable(3,4) input on pin 9 connected to digital output pin D7
// Collision Sensor
const int collsionPin = D8;
int collisionCount = 0;
int distance;
bool lastState = false;
// Led Strip
const int ledPin = D4;
// Motor control global variables:
uint8_t motorSpeed; // Motor speed 0..255
int motorDirection = 1;                      // Forward (1) or reverse (0)
// Servo control global variables:
uint8_t steering = 0;                            // Servo position 0..255
int steeringDirection = 0;                   // Left (0) and Right (1)

// OLED screen definition
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET     0 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // our LED has 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Led definition
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, ledPin, NEO_GRB + NEO_KHZ800);


// removed TP logo for OLED

// The MQTT topics that the tractor should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "tractor/1/data"
#define AWS_IOT_SUBSCRIBE_TOPIC "tractor/crop"

// configure SSL to use the AWS IoT device credentials
BearSSL::X509List client_crt(AWS_CERT_CRT);
BearSSL::PrivateKey client_key(AWS_CERT_PRIVATE);
BearSSL::X509List rootCert(AWS_CERT_CA);

WiFiClientSecure wiFiClient;
void msgReceived(char* topic, byte* payload, unsigned int len);
PubSubClient pubSubClient(AWS_IOT_ENDPOINT, 8883, msgReceived, wiFiClient);

void connectWifi() {
//  Serial.print("Connecting to "); Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Connecting to:")); display.print(WIFI_SSID);
  display.display();
  WiFi.waitForConnectResult();
//  Serial.print(", WiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void displayThinkport() {
  // Lets display Thinkport inverted when we boot up
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.setCursor(10, 0);
  display.clearDisplay(); // clear the display
  display.println(F("Thinkport"));
  display.display();
  delay (2000);
}

void displayProjectName() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 25);
  display.print(projectname);
  display.display();
  delay (1000);
}

void displayIP() {
  // display IP address on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("WiFi connected, IP address: ")); display.print(WiFi.localIP());
  display.display();
}

void powerOLED() {
  // we need to powerup the OLED. Error when not found
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
//    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
}

void controlMotors(int height, int width) {
  setHeight(height);
  setWidth(width);
  setLedGreen(width);
}

void setHeight(int height) {
  uint16_t i;
  for (i = 0; i < height; i++) {
    driveHeight();
  }
}
void driveHeight()  {
  digitalWrite(controlPin3A, LOW);
  digitalWrite(controlPin4A, HIGH);
  analogWrite(servoENablePin, 250);
  delay(250);
  digitalWrite(controlPin1A, HIGH);
  digitalWrite(controlPin2A, LOW);
  analogWrite(ENablePin, 249);
  delay(750);
  digitalWrite(controlPin1A, LOW);
  digitalWrite(controlPin2A, LOW);
  analogWrite(ENablePin, 0);
  digitalWrite(controlPin3A, LOW);
  digitalWrite(controlPin4A, LOW);
  analogWrite(servoENablePin, 0);
}

void setWidth(int width)  {
    uint16_t i;
  for (i = 0; i < width; i++) {
    driveWidth();
  }
}

void driveWidth() {
  digitalWrite(controlPin3A, HIGH);
  digitalWrite(controlPin4A, LOW);
  analogWrite(servoENablePin, 250);
  delay(250);
  digitalWrite(controlPin1A, LOW);
  digitalWrite(controlPin2A, HIGH);
  analogWrite(ENablePin, 249);
  delay(2700);
  digitalWrite(controlPin1A, LOW);
  digitalWrite(controlPin2A, LOW);
  analogWrite(ENablePin, 0);
  digitalWrite(controlPin3A, LOW);
  digitalWrite(controlPin4A, LOW);
  analogWrite(servoENablePin, 0);
}

void setLedYellow()  {
  uint16_t i;
  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(127, 127, 0));
    strip.show();
  }
}

void setLedRed()  {
  uint16_t i;
  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(127, 0, 0));
    strip.show();
  }
}

void setLedGreen(int leds)  {
  setLedRed();
  uint16_t i;
  for (i = 2-leds; i < strip.numPixels() - (2 - leds); i++) {
    strip.setPixelColor(i, strip.Color(0, 127, 0));
    strip.show();
  }
}

void publishTractorData()
{
  StaticJsonDocument<200> doc;
  doc["type"] = "John Deer 7R";
  doc["farmer"] = "Jane Doe";
  doc["location"] = "Monheim";
  doc["temp"] = 16;
  doc["humidity"] = 80;
  doc["wind"] = 30;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to pubSubClient
  pubSubCheckConnect();
  pubSubClient.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
//  Serial.print("Published: "); Serial.println(jsonBuffer);
}


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(controlPin1A, FUNCTION_3); // Set RX as DigitalPin
  pinMode(controlPin1A, OUTPUT);      // 1A
  pinMode(controlPin2A, OUTPUT);      // 2A
  pinMode(ENablePin, FUNCTION_3);     // Set TX as DigitalPin
  pinMode(ENablePin, OUTPUT);         // EN1,2
  pinMode(controlPin3A, OUTPUT);      // 3A
  pinMode(controlPin4A, OUTPUT);      // 4A
  pinMode(servoENablePin, OUTPUT);    // EN3,4
  pinMode(collsionPin, INPUT);
  digitalWrite(BUILTIN_LED, HIGH); // turn of LED
//  Serial.begin(115200); Serial.println();
  strip.begin();
  strip.show();
  setLedYellow();
  powerOLED();
  displayThinkport() ;
  displayProjectName() ;
  connectWifi();
  displayIP();
  setCurrentTime(); // get current time, otherwise certificates are flagged as expired

  wiFiClient.setClientRSACert(&client_crt, &client_key);
  wiFiClient.setTrustAnchors(&rootCert);

  pubSubCheckConnect();
  publishTractorData();
}

unsigned long lastPublish;
int msgCount;


void loop() {

  pubSubCheckConnect();

  if(digitalRead(D8) == HIGH && lastState == false) {
    collisionCount += 1;
    lastState = true;
    if (collisionCount == distance) {
      setLedRed();
    }
  } else if (digitalRead(D8) == LOW && lastState == true) {
    lastState = false;
  }

}

void msgReceived(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> cropData;
  // JsonObject root = receivedMessage.parseObject(payload);
  DeserializationError error = deserializeJson(cropData, payload);
  if (error) {
//    Serial.print(F("deserializeJson() failed: "));
//    Serial.println(error.f_str());
    return;
  }

  // Fetch values.
  int height = cropData["recommendation"]["sprayer"]["height"];
  int width  = cropData["recommendation"]["sprayer"]["width"];
  int distanceReceived  = cropData["recommendation"]["buffer"]["distance"];
  const char* labelname = cropData["product"];

  // Print values.
//  Serial.print("Labelname: "); Serial.println(labelname);
//  Serial.print("height: "); Serial.println(height);
//  Serial.print("width: "); Serial.println(width);
//  Serial.print("distance: "); Serial.println(distance);
  // Show values on LED
  display.clearDisplay();
   //display.setCursor(0, 10);
  display.setCursor(18, 10);
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
 // display.print(F("Labelname: ")); display.println(labelname);
  display.println(labelname);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(F("Height: ")); display.println(height);
  display.print(F("Width: ")); display.println(width);
  display.print(F("Distance: ")); display.println(distanceReceived);
  display.display();
  distance = distanceReceived;
  lastState = false;
  collisionCount = 0;
  // we do something with the tractor hardware
  controlMotors(height, width);
}


void pubSubCheckConnect() {
  if ( ! pubSubClient.connected()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("PubSubCLient connecting to:")); display.print(AWS_IOT_ENDPOINT);
    display.display();
//    Serial.print("PubSubClient connecting to: "); Serial.print(AWS_IOT_ENDPOINT);
    while ( ! pubSubClient.connected()) {
//      Serial.print(".");
      // pubSubClient.connect("ESPthing");
      pubSubClient.connect("BayerTractorESP");
    }
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(F("PubSubClient connected."));
    display.display();
//    Serial.println(" connected");
    pubSubClient.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  }
  pubSubClient.loop();
}

void setCurrentTime() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

//  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
//    Serial.print(".");
    now = time(nullptr);
  }
//  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
//  Serial.print("Current time: "); Serial.print(asctime(&timeinfo));
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 10);
  display.print(F("Current time: ")); display.print(asctime(&timeinfo));
  display.display();

}
