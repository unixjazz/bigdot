/***************************************************
/*
 * WiFi extension board v1.2 for librelab::BigDot
 * PMS 7003, and Neopixel LED ring
 * 
 * Library dependencies:
 * - PMS v1.1.0
 * - SAM32WiFiEsp v2.2.2-f1
 * - PubSubClient v2.8.0
 * - Adafruit MQTT Library v2.4.3
 * - Adafruit Neopixel v1.10.0
 *   (^ from UVA-DSI repo)
 *   
 *  Contains Adafruit MQTT Library ESP8266 Example code from:
 *  Tony DiColswa
 */

 /****************************************************/

#include <stdlib.h>
#include <Arduino.h>
#include <SAM32WiFiEsp.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <PMS.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "wiring_private.h"
#include "private_settings.h"

/*************************** Extension Board Pins ***********************************/
// Serial2 on SERCOM2 for Big Dot v1
#define PIN_SERIAL2_RX   (15ul)            // Pin D6
#define PIN_SERIAL2_TX   (14ul)            // Pin D5
#define PAD_SERIAL2_TX   (UART_TX_PAD_2)   // SERCOM2.2
#define PAD_SERIAL2_RX   (SERCOM_RX_PAD_3) // SERCOM2.3
int status = WL_IDLE_STATUS;
Uart Serial2(&sercom2, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);
/************ Global State (you don't need to change this!) ******************/
// Setup the MQTT class by passing in the MQTT server and login details.
WiFiEspClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// LED constants
#define LED_PIN      A1
#define NUMPIXELS    24
#define BRIGHTNESS   20
#define DELAY_VAL    25

// LED Ring
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

// USB DEBUG on/off
//#define DEBUG
#undef DEBUG

// PMS on Serial1
PMS pms(Serial1);
PMS::DATA data;
const int pmsRSTpin = A4;
char temp_var[8];

/****************************** Feeds ***************************************/
// Setup a feed called for publishing pms 2.5 values to.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish pm2_5 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/TEMP");
/*************************** Sketch Code ************************************/

void MQTT_connect();

void setup() {

  // LED ring
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  ledFill(init_color);

  // Initialize Serial1
  #ifdef DEBUG
  while (!Serial);
  Serial.begin(115200);
  Serial.println(F("[UART0] Started SerialUSB"));
  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  #endif

  // Initialize PMS
  Serial1.begin(9600);
  pms.passiveMode();
  pinMode(pmsRSTpin, OUTPUT);
  digitalWrite(pmsRSTpin, LOW);
  #ifdef DEBUG
  Serial.println("[PMS7003] Started particulate matter sensor");
  #endif
  
  // Serial2
  pinPeripheral(15, PIO_SERCOM); 
  pinPeripheral(16, PIO_SERCOM); 
  Serial2.begin(115200);

  while (!Serial2) ;
  #ifdef DEBUG
  Serial.println("Serial2 is open!");
  #endif

  WiFi.init(&Serial2);
  WiFi.status();
  // Check if the ESP12E is present
  if (WiFi.status() == WL_NO_SHIELD) {
    #ifdef DEBUG
    Serial.println("[WiFiEsp] No ESP12E shield present");
    #endif
  }
  while(status != WL_CONNECTED){
    #ifdef DEBUG
    Serial.print("[WiFiEsp] Attempting to connect to WPA SSID: ");
    Serial.println(WLAN_SSID);
    #endif
    status = WiFi.begin(WLAN_SSID, WLAN_PASS);
  }
  #ifdef DEBUG
  Serial.println("Initialized Wifi to Serial2");
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  #endif

}


void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  delay(10000);
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here
  // Now we can publish stuff!
  #ifdef DEBUG
  Serial.println("Sending bigdotpms val");
  #endif
  // Read PMS sensor and publish data
  pms_update();

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       #ifdef DEBUG
       Serial.println("Retrying MQTT connection in 5 seconds...");
       #endif
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  #ifdef DEBUG
  Serial.println("MQTT Connected!");
  #endif
}


void SERCOM2_Handler()
{
  Serial2.IrqHandler();
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
    // PM 2.5
    #ifdef DEBUG
    Serial.print("[PMS7003] PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);
    #endif
    itoa(data.PM_AE_UG_2_5, temp_var, 10);
    if (! pm2_5.publish(temp_var)) {
    #ifdef DEBUG
    Serial.println("Failed");
    #endif
  } else {
    #ifdef DEBUG
    Serial.println("OK!");
    #endif
  }
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


/*
 * ledFill changes the color of the NeoPixel ring
 * the delay is to display like an animation.
 */
void ledFill(uint32_t led_color)
{
  
  for (int i=0; i< NUMPIXELS; i++) {
    strip.setPixelColor(i, led_color);
    delay(1.5);    
    strip.show();
    delay(DELAY_VAL);
  }
}


//Adjust shades of color according to the EPA scale
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

    
    //If greater than 85 the LED will be RED
    default: 
    ledFill(dark_red);
    strip.show();
    break;
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
