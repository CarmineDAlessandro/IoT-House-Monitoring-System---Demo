/*
 * IoT_House_Monitoring_System_Demo.ino
 * 
 * This sketch was made for the project of the class
 * of Laboratory of IoT. The official material of the
 * class was the Arduino Uno Wifi Rev2 board and other
 * sensors and actuators. The project is made by 2 main
 * parts: the board and the cloud.
 * 
 * This sketch works for 3 sensors monitoring 4
 * environmental parameters (humidity, temperature,
 * light, gas), 3 actuators (display, 2 LEDs) and 1
 * button. The system automatically collects the data 
 * from the environment and displays it while the 2
 * LEDs are turned on or off.
 * 
 * More information can be found in the wiki of 
 * the project (link below).
 * 
 * Pin List:
 * Arduino ------------ Hardware
 * pin 8                green LED (+)
 * pin 9                red LED (+)
 * pin A1               light sensor
 * pin A2               gas sensor
 * SDA                  SDA OLED display 
 * SCL                  SCL OLED display
 * 5V                   VCC
 * GND                  GND
 * 
 * 
 * Created 29/11/2019
 * By Carmine D'Alessandro
 * 
 * Download "ArduinoSimpleThreads" at this link:
 * https://github.com/CarmineDAlessandro/Arduino-Simple-Threads-Library
 * 
 * Code:
 * https://github.com/CarmineDAlessandro/IoT-House-Monitoring-System-Demo
 *
 * Wiki:
 * https://github.com/CarmineDAlessandro/IoT-House-Monitoring-System-Demo/wiki
 *
 */

/*************************************************/
/* DEFINITIONS                                   */
/*************************************************/

/* You can download the library at this link
 * https://github.com/CarmineDAlessandro/Arduino-Simple-Threads-Library 
 */
#include "ArduinoSimpleThreads.h"

/* ThingSpeak Communication */
#include "ThingSpeak.h"
#include <WiFiNINA.h>
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

/* Display */
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);
int is_displaying;

/* DHT */
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float humidity;
float temperature;

/* LIGHT */
#define PIN_LIGHT_INPUT A1
float light;

/* LED */
#define PIN_LED_GREEN 8
#define PIN_LED_RED   9

/* MQ2 */
#define PIN_SMOKE_SENSOR A2
float smoke_value;

/* BUTTON */
bool transmission_mode = false;
#define PIN_BUTTON_INPUT 3

/*************************************************/
/* PSEUDO-THREADS                                */
/*************************************************/

/* SENSORS AND ACTUATORS */

void sensors_setup() {
  
  //LIGHT
  pinMode(PIN_LIGHT_INPUT, INPUT);
  // HUMIDITY - TEMPERATURE
  pinMode(DHTPIN, INPUT);
  dht.begin();
  // LEDs
  pinMode(PIN_LED_GREEN, OUTPUT);
  digitalWrite(PIN_LED_GREEN, LOW);
  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW);
  // GAS
  pinMode(PIN_SMOKE_SENSOR, INPUT);
  
}

void sensors_loop() {

  // Collect data
  light = analogRead(PIN_LIGHT_INPUT);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  smoke_value = analogRead(PIN_SMOKE_SENSOR);

  // Show the mode
  if(transmission_mode) {
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_LED_RED, LOW);
  } else {
    digitalWrite(PIN_LED_GREEN,LOW);
    digitalWrite(PIN_LED_RED, HIGH);
  }
}

/* OLED DISPLAY*/

void display_setup() {
  
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();
  is_displaying = 0;

  // display setup message
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(30, 10);
  display.print("Doing");
  display.setCursor(30, 30);
  display.print("Setup");
  display.display();

}

void display_loop() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);

  switch(is_displaying) {
    // Light
    case 0:
      display.setCursor(0, 10);
      display.print("Light");
      display.setCursor(5, 30);
      display.print(light);
      break;
      
    // Temperature
    case 1: 
      display.setCursor(0, 10);
      display.print("Temperatur");
      display.setCursor(5, 30);
      display.print(temperature);
      break;
      
    // Humidity
    case 2: 
      display.setCursor(0, 10);
      display.print("Humidity");
      display.setCursor(5, 30);
      display.print(humidity);
      break;

    // Gas
    case 3:
      display.setCursor(0, 10);
      display.print("Smoke");
      display.setCursor(5, 30);
      display.print(smoke_value);
      break;
    
    default:
      is_displaying = 0;
      break;
  } 

  is_displaying = (is_displaying+1)%4;
  display.display();
}

/* WIFI COMMUNICATION */

void thingSpeakCommunication_setup() {
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv != "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
    
  ThingSpeak.begin(client);  //Initialize ThingSpeak
}

void thingSpeakCommunication_loop() {

  if(transmission_mode) {
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(SECRET_SSID);
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial.print(".");
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }
  
    // set the fields with the values
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, light);
    ThingSpeak.setField(4, smoke_value);
  
    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  }
}

/*************************************************/
/* USER INTERRUPT                                */
/*************************************************/

void button_change_mode() {
  transmission_mode = !transmission_mode;
}

/*************************************************/
/* MAIN LOGIC                                    */
/*************************************************/

void setup() {
  Serial.begin(9600);  // Initialize serial

  if(usingSimpleThreads(3) == SIMPLE_THREAD_ERROR)
  {
    Serial.println("Error during thread definition");
    exit(1);
  }

  if(declareSimpleThread(display_loop,
                          display_setup,
                          5000L, // 5 seconds
                          STANDARD_PRIORITY) == SIMPLE_THREAD_ERROR)
  {
    Serial.println("Error during thread declaration");
    exit(1);
  }

  if(declareSimpleThread(sensors_loop,
                          sensors_setup,
                          200L, // 0.2 seconds
                          HIGH_PRIORITY) == SIMPLE_THREAD_ERROR)
  {
    Serial.println("Error during thread declaration");
    exit(1);
  }
  
  if(declareSimpleThread(thingSpeakCommunication_loop,
                          thingSpeakCommunication_setup,
                          20000L, // 20 seconds
                          LOW_PRIORITY) == SIMPLE_THREAD_ERROR)
  {
    Serial.println("Error during thread declaration");
    exit(1);
  }

  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_INPUT), button_change_mode, FALLING);
}

void loop() {

  runSimpleThreads();
}
