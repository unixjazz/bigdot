/*
 * WiFi extension board v1.2 for librelab::BigDot
 * PMS 7003, BME280, OLED display + Neopixel LED ring
 * 
 * Library dependencies:
 * - PMS v1.1.0
 * - U8g2 v2.31.2
 * - SSLclient (TODO)
 * - SAM32WiFiEsp v2.2.2-f1
 * - PubSubClient v2.8.0
 * - Sparkfun / Bosch BME280  v2.0.9
 * - Adafruit Neopixel v1.10.0
 *   (^ from UVA-DSI repo)
 */

#include <stdlib.h>
#include <Arduino.h>
#include <SparkFunBME280.h>
#include <Adafruit_NeoPixel.h>
#include <SAM32WiFiEsp.h>
#include <PubSubClient.h>
#include <U8g2lib.h>
#include <PMS.h>
#include "wiring_private.h"
#include "private_settings.h"

// Serial2 on SERCOM2 for Big Dot v1
#define PIN_SERIAL2_RX   (15ul)            // Pin D6
#define PIN_SERIAL2_TX   (14ul)            // Pin D5
#define PAD_SERIAL2_TX   (UART_TX_PAD_2)   // SERCOM2.2
#define PAD_SERIAL2_RX   (SERCOM_RX_PAD_3) // SERCOM2.3

// MQTT conf + endpoints
#define MQTT_KEEPALIVE   15000
#define SENSOR1_TOPIC1   "/bigdot0/sensor_1/pm1"
#define SENSOR1_TOPIC2   "/bigdot0/sensor_1/pm2_5"
#define SENSOR1_TOPIC3   "/bigdot0/sensor_1/pm10"
#define SENSOR2_TOPIC1   "/bigdot0/sensor_2/humidity"
#define SENSOR2_TOPIC2   "/bigdot0/sensor_2/pressure"
#define SENSOR2_TOPIC3   "/bigdot0/sensor_2/temp"

// LED constants
#define LED_PIN      A1
#define NUMPIXELS    24
#define BRIGHTNESS   20
#define DELAY_VAL    25

// LED ring
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Colors for LED ring
uint32_t init_color = strip.Color(25, 25, 25);
uint32_t light_green = strip.Color(5, 252, 95);
uint32_t dark_green = strip.Color(0, 255, 0);
uint32_t light_yellow = strip.Color(199, 255, 0);
uint32_t dark_yellow = strip.Color(255, 255, 0);
uint32_t light_orange = strip.Color(255, 212, 0);
uint32_t dark_orange = strip.Color(255, 165, 0);
uint32_t light_red = strip.Color(255, 100, 0);
uint32_t dark_red = strip.Color(255, 0, 0);

// OLED display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, D8, D7, U8X8_PIN_NONE);

// USB DEBUG on/off
//#define DEBUG
#undef DEBUG

// Serial2 for ESP12E
Uart Serial2(&sercom2, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);

// WiFi module
int status = WL_IDLE_STATUS;
WiFiEspClient ESPclient;
PubSubClient client(ESPclient);

// PMS on Serial1
PMS pms(Serial1);
PMS::DATA data;
const int pmsRSTpin = A4;
char temp_var[8];

// BME280 sensor
BME280 bme680;
float humidity = 0;
float pressure = 0;
float temp = 0;
String temp_char;

void setup()
{
  // LED ring
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  ledFill(init_color);
   
  // Initialize serial
  #ifdef DEBUG
  while (!Serial);
  Serial.begin(115200);
  Serial.println(F("[UART0] Started SerialUSB"));
  #endif

  // Initialize PMS
  Serial1.begin(9600);
  pms.passiveMode();
  pinMode(pmsRSTpin, OUTPUT);
  digitalWrite(pmsRSTpin, LOW);
  #ifdef DEBUG
  Serial.println("[PMS7003] Started particulate matter sensor");
  #endif

  Wire.begin();
  
  // Initialize OLED
  //display.setI2CAddress(0x3C);
  //display.setBusClock(1);
  //display.setPowerSave(0);
  display.begin();
  // OLED welcome screen
  display.clearBuffer();
  display.setFont(u8g_font_profont11);
  display.setCursor(4,40); display.print("librelab::Big Dot v1");
  display.sendBuffer();
  #ifdef DEBUG
  Serial.println("[OLED] Started mini display");
  #endif

  // Initialize BME
  bme680.setI2CAddress(0x77);
  bme680.beginI2C();
  #ifdef DEBUG
  Serial.println("[BME280] Started temp/humidity/pressure sensor");
  #endif

  // Serial2
  pinPeripheral(15, PIO_SERCOM); 
  pinPeripheral(16, PIO_SERCOM);

  // Initialize ESP12E module
  Serial2.begin(115200);
  WiFi.init(&Serial2);
  // Check if the ESP12E is present
  if (WiFi.status() == WL_NO_SHIELD) {
    #ifdef DEBUG
    Serial.println("[WiFiEsp] No ESP12E shield present");
    #endif
  }

  // Connect to WiFi network
  while ( status != WL_CONNECTED) {
    #ifdef DEBUG
    Serial.print("[WiFiEsp] Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    #endif
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, password);
  }
  #ifdef DEBUG
  printCurrentNet();
  #endif

  // Connect to MQTT
  //ESPclient.setCACert(ca_root);
  client.setServer(mqtt_server, mqtt_port);
  client.connect(mqtt_clientID, mqtt_user, mqtt_passwd);
  client.setKeepAlive(MQTT_KEEPALIVE);
}

void loop()
{  
  delay(10000);
  // Check if MQTT client is connected
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
  }
  
  // Read PMS sensor
  pms_update();

  // Read BME sensor
  bme_update();
}

void SERCOM2_Handler()
{
  Serial2.IrqHandler();
}

void reconnect()
{
  // Loop until reconnected
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.println("[MQTT] Attempting connection...");
    #endif
    // Attempt to connect
    if (client.connect(mqtt_clientID, mqtt_user, mqtt_passwd)) {
      #ifdef DEBUG
      Serial.println("[MQTT] Connected!");
      #endif
    } else {
      #ifdef DEBUG
      Serial.print("[MQTT] Failed, rc=");
      Serial.print(client.state());
      Serial.println("[MQTT] Try again in 5 seconds");
      #endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void printCurrentNet()
{
  #ifdef DEBUG
  // print the SSID of the network you're attached to
  Serial.print("[WiFiEsp] SSID: ");
  Serial.println(WiFi.SSID());
  
  // print the MAC address of the router you're attached to
  byte bssid[6];
  WiFi.BSSID(bssid);
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[5], bssid[4], bssid[3], bssid[2], bssid[1], bssid[0]);
  Serial.print("[WiFiEsp] BSSID: ");
  Serial.println(buf);
    
  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("[WiFiEsp] Signal strength (RSSI): ");
  Serial.println(rssi);
  #endif
}

void mqtt_connect() 
{
  #ifdef DEBUG
  Serial.println("[MQTT] Connecting...");
  #endif
  if (client.connect(mqtt_clientID, mqtt_user, mqtt_passwd)) {
    #ifdef DEBUG
    Serial.println("[MQTT] Connected to MQTT server");
    #endif
  } else {
    #ifdef DEBUG
    Serial.print("[MQTT] Connection failed, status code = ");
    Serial.print(client.state());
    #endif
  }
}

// BME280
void bme_update()
{
  humidity = bme680.readFloatHumidity();
  display.setCursor(1, 30); display.print("Humidity (%): ");
  display.setCursor(95, 30); display.print(humidity, 0);
  #ifdef DEBUG
  Serial.print("[BME280] Humidity (%): ");
  Serial.println(humidity);
  #endif
  temp_char = String(humidity);
  temp_char.toCharArray(temp_var, 3);
  client.publish(SENSOR2_TOPIC1, temp_var);
  delay(1000);
  
  pressure = bme680.readFloatPressure();
  display.setCursor(1, 40); display.print("Pressure (P): ");
  display.setCursor(95, 40); display.print(pressure, 0);
  #ifdef DEBUG
  Serial.print("[BME280] Pressure (P): ");
  Serial.println(pressure);
  #endif
  temp_char = String(pressure);
  temp_char.toCharArray(temp_var, 3);
  client.publish(SENSOR2_TOPIC2, temp_var);
  delay(1000);
  
  temp = bme680.readTempF();
  display.setCursor(1, 50); display.print("Temp (F): ");
  display.setCursor(95, 50); display.print(temp, 2);
  display.sendBuffer();
  #ifdef DEBUG
  Serial.print("[BME280] Temperature (F): ");
  Serial.println(temp);
  #endif
  temp_char = String(temp);
  temp_char.toCharArray(temp_var, 3);
  client.publish(SENSOR2_TOPIC3, temp_var);
  delay(1000);
}

// PMS7003
void pms_update()
{
  pms.wakeUp();
  digitalWrite(pmsRSTpin, HIGH);
  delay(3000);
  pms.requestRead();
  if (pms.readUntil(data))
  {
    // PM 1.0
    //itoa(data.PM_AE_UG_1_0, temp_var, 10);
    //client.publish(SENSOR1_TOPIC1, temp_var);
    //display.clearBuffer();
    //display.setCursor(1, 10); display.print("PM 1.0 (ug/m3): ");
    //display.setCursor(95, 10); display.print(data.PM_AE_UG_1_0);
    //display.sendBuffer();
    //#ifdef DEBUG
    //Serial.print("[PMS7003] PM 1.0 (ug/m3): ");
    //Serial.println(data.PM_AE_UG_1_0);
    //#endif
    
    // PM 2.5
    display.clearBuffer();
    display.setCursor(1, 10); display.print("PM 2.5 (ug/m3): ");
    display.setCursor(95, 10); display.print(data.PM_AE_UG_2_5);
    #ifdef DEBUG
    Serial.print("[PMS7003] PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);
    #endif
    itoa(data.PM_AE_UG_2_5, temp_var, 10);
    client.publish(SENSOR1_TOPIC2, temp_var);
    delay(1000);
    
    // PM 10
    display.setCursor(1, 20); display.print("PM 10 (ug/m3): ");
    display.setCursor(95, 20); display.print(data.PM_AE_UG_10_0);
    #ifdef DEBUG
    Serial.print("[PMS7003] PM 10 (ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);
    #endif
    itoa(data.PM_AE_UG_10_0, temp_var, 10);
    client.publish(SENSOR1_TOPIC3, temp_var);
    delay(1000);
    
    // Update LED strip
    led_update(data.PM_AE_UG_2_5);
    #ifdef DEBUG
    Serial.println("[LED] Updated LED ring with PM values");
    #endif
  }
  pms.sleep();
  digitalWrite(pmsRSTpin, LOW);
}

// LED ring
void ledFill(uint32_t led_color)
{
  for (int i=0; i< NUMPIXELS; i++) {
    strip.setPixelColor(i, led_color);    
    strip.show();
    delay(DELAY_VAL);
  }
}

// FIXME: adjust shades of color
// according to the EPA scale
void led_update(uint32_t pm_val)
{
  switch(pm_val) {
    case 0 ... 7:
    ledFill(light_green);
    break;

    case 8 ... 15:
    ledFill(dark_green);
    strip.show();
    break;

    case 16 ... 25:
    ledFill(light_yellow);
    strip.show();
    break;

    case 26 ... 40:
    ledFill(dark_yellow);
    strip.show();
    break;

    case 41 ... 50:
    ledFill(light_orange);
    strip.show();
    break;

    case 51 ... 65:
    ledFill(dark_orange);
    strip.show();
    break;

    case 66 ... 85:
    ledFill(light_red);
    strip.show();
    break;

    case 86 ... 150:
    ledFill(dark_red);
    strip.show();
    break;
  }
}
