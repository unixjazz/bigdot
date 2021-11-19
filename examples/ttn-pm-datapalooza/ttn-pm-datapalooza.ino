/*
 * LoRa PM extension board v1.2 for librelab::BigDot
 * PMS 7003, BME280, OLED display + Neopixel LED ring
 * Example code for Datapalooza workshop
 * 
 * Library dependencies:
 * - PMS v1.1.0
 * - U8g2 v2.31.2
 * - MCCI LoRaWAN library v3.3.0
 * - Sparkfun / Bosch BME280  v2.0.9
 * - Adafruit Neopixel v1.10.0
 *   (^ from UVA-DSI repo)
 *
 * Contains LoRaWAN code from:
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 * 
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED. 
 * 
 */

#include <SPI.h>
#include <Wire.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SparkFunBME280.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoLowPower.h>
#include <U8g2lib.h>
#include <PMS.h>

#define DEBUG
//#undef DEBUG

// copy node id, app id and appkey here:
static const u1_t PROGMEM APPEUI[8]={ COPY HERE };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8]={ COPY HERE };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { COPY HERE };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

// Payload to send to TTN gateway
static uint8_t payload[14];
static osjob_t sendjob;

// Schedule TX every 'x' seconds
// and sleep time in milliseconds
//const int SLEEP_TIME = 30000;
const unsigned TX_INTERVAL = 30;

// Pin mapping for Big Dot v1
const lmic_pinmap lmic_pins = {
  .nss = D12,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = D5,
  .dio = {D6, D1, LMIC_UNUSED_PIN},
};

// Init
// PMS7003 sensor
const int pmsRSTpin = A4;
PMS pms(Serial1);
PMS::DATA data;

// BME280 sensor
BME280 bme680;
float humidity = 0;
float pressure = 0;
float temp = 0;

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

// OLED display init
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, D8, D7, U8X8_PIN_NONE);

// LMIC helper function
void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16) {
        #ifdef DEBUG
        Serial.print('0');
        #endif
    }
    #ifdef DEBUG
    Serial.print(v, HEX);
    #endif
}

// LMIC event routine
void onEvent (ev_t ev) {
    #ifdef DEBUG
    Serial.print(os_getTime());
    Serial.print(": ");
    #endif
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            #ifdef DEBUG
            Serial.println(F("EV_SCAN_TIMEOUT"));
            #endif
            break;    
        case EV_BEACON_FOUND:
            #ifdef DEBUG
            Serial.println(F("EV_BEACON_FOUND"));
            #endif
            break;
        case EV_BEACON_MISSED:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_BEACON_MISSED"));
            #endif
            break;
        case EV_BEACON_TRACKED:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_BEACON_TRACKED"));
            #endif
            break;
        case EV_JOINING:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_JOINING"));
            #endif
            break;
        case EV_JOINED:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("[LORA] netid: ");
              Serial.println(netid, DEC);
              Serial.print("[LORA] devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("[LORA] artKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                  Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("[LORA] nwkKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                 if (i != 0)
                   Serial.print("-");
                   Serial.print(nwkKey[i], HEX);
              }
              Serial.println("");
            }
            #endif
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_JOIN_FAILED"));
            #endif
            break;
        case EV_REJOIN_FAILED:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_REJOIN_FAILED"));
            #endif
            break;
        case EV_TXCOMPLETE:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_TXCOMPLETE (includes waiting for RX windows)"));
            #endif
            if (LMIC.txrxFlags & TXRX_ACK)
              #ifdef DEBUG
              Serial.println(F("[LORA] Received ack"));
              #endif
            if (LMIC.dataLen) {
              #ifdef DEBUG
              Serial.println(F("[LORA] Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F("[LORA] Bytes of payload"));
              #endif
            }
            // Schedule next transmission
            delay(1000);
            #ifdef DEBUG
            Serial.println("[BigDot] Going to sleep until next TX");
            #endif
            //LowPower.sleep(SLEEP_TIME);
            #ifdef DEBUG
            Serial.println("[BigDot] Waking up from sleep");
            #endif
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_LOST_TSYNC"));
            #endif
            break;
        case EV_RESET:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_RESET"));
            #endif
            break;
        case EV_RXCOMPLETE:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_RXCOMPLETE"));
            #endif
            break;
        case EV_LINK_DEAD:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_LINK_DEAD"));
            #endif
            break;
        case EV_LINK_ALIVE:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_LINK_ALIVE"));
            #endif
            break;
        case EV_TXSTART:
            #ifdef DEBUG
            Serial.println(F("[LORA] EV_TXSTART"));
            #endif
            break;
        default:
            #ifdef DEBUG
            Serial.print(F("[LORA] Unknown event: "));
            Serial.println((unsigned) ev);
            #endif
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        #ifdef DEBUG
        Serial.println(F("[LORA] OP_TXRXPEND, not sending"));
        #endif
    } else {
        #ifdef DEBUG
        Serial.println("[PMS7003] Wake-up sensor");
        #endif
        pms.wakeUp();
        digitalWrite(pmsRSTpin, HIGH);
        delay(20000);

        display.clearBuffer();
        humidity = bme680.readFloatHumidity();
        #ifdef DEBUG
        Serial.print("[BME280] Humidity (%): ");
        Serial.println(humidity);
        #endif
        display.setCursor(1, 30); display.print("Humidity (%): ");
        display.setCursor(95, 30); display.print(humidity, 0);

        pressure = bme680.readFloatPressure();
        pressure = pressure / 100.0;
        #ifdef DEBUG
        Serial.print("[BME280] Pressure (hPa): ");
        Serial.println(pressure);
        #endif
        display.setCursor(1, 40); display.print("Pressure (hPa): ");
        display.setCursor(95, 40); display.print(pressure, 0);

        temp = bme680.readTempF();
        #ifdef DEBUG
        Serial.print("[BME280] Temperature (F): ");
        Serial.println(temp);
        #endif
        display.setCursor(1, 50); display.print("Temp (F): ");
        display.setCursor(95, 50); display.print(temp, 2);
        
        uint32_t pressure_conv = (uint32_t)((pressure) * 100);
        payload[0] = (pressure_conv & 0xFF);
        payload[1] = (pressure_conv >> 8) & 0xFF;
        payload[2] = (pressure_conv >> 16) & 0xFF;
        payload[3] = (pressure_conv >> 24) & 0xFF;

        uint16_t payloadTemp = (uint16_t)(temp * 100);
        payload[4] = payloadTemp >> 8;
        payload[5] = payloadTemp & 0xFF;
        
        uint16_t payloadHumid = (uint16_t)(humidity * 100);
        payload[6] = payloadHumid >> 8;
        payload[7] = payloadHumid & 0xFF;

        pms.requestRead();
        if (pms.readUntil(data)) {
          #ifdef DEBUG
          Serial.print("[PMS7003] PM 2.5 (ug/m3): ");
          Serial.println(data.PM_AE_UG_2_5);
          #endif
          display.setCursor(1, 10); display.print("PM 2.5 (ug/m3): ");
          display.setCursor(95, 10); display.print(data.PM_AE_UG_2_5);
          uint16_t pm2_5 = data.PM_AE_UG_2_5;
          payload[8] = pm2_5 >> 8;
          payload[9] = pm2_5 & 0xFF;

          #ifdef DEBUG
          Serial.print("[PMS7003] PM 10.0 (ug/m3): ");
          Serial.println(data.PM_AE_UG_10_0);
          #endif
          display.setCursor(1, 20); display.print("PM 10 (ug/m3): ");
          display.setCursor(95, 20); display.print(data.PM_AE_UG_10_0);
          display.sendBuffer();
          uint16_t pm10 = data.PM_AE_UG_10_0;
          payload[10] = pm10 >> 8;
          payload[11] = pm10 & 0xFF;
        }
        pms.sleep();
        digitalWrite(pmsRSTpin, LOW);

        // TX
        LMIC_setTxData2(1, payload, sizeof(payload)-1, 0);
    }
}

void setup() {
  delay(1000);
  #ifdef DEBUG
  while (!Serial);
  Serial.begin(115200);
  Serial.println(F("[BigDot] SerialUSB debug activated"));
  #endif

  // LED ring
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  ledFill(init_color);
  #ifdef DEBUG
  Serial.println("[LED] Started LED ring");
  #endif
  
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

  // BME280 settings
  bme680.setI2CAddress(0x77);
  bme680.beginI2C();
  #ifdef DEBUG
  Serial.println("[BME280] Started temp/humidity/pressure sensor");
  #endif
  
  // LMIC init and reset radio
  os_init();
  LMIC_reset();
  #ifdef DEBUG
  Serial.println("[LORA] Started LoRa radio");
  #endif
    
  // Adaptative data rate ON/OFF
  LMIC_setLinkCheckMode(0);
  LMIC_setAdrMode(0);

  // Set spreadfactor, power
  LMIC_setDrTxpow(DR_SF7,14);

  // Select freq. band
  LMIC_selectSubBand(1);

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
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
