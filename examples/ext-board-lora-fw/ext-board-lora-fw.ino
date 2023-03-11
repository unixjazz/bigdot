/* 
 *  librelab::Big Dot extension board fw
 *  (integrates env. sensors for LoRa TX)
 *  
 *  Dependencies:
 *  Adafruit Neopixel,
 *  Sparkfun BME680,
 *  LMIC,
 *  U8g2,
 *  PMS
 *  
 *  Contains OS LMIC LoRa library code from:
 *  Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *  Copyright (c) 2018 Terry Moore, MCCI
 */
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2lib.h>
#include <PMS.h>


// LoRa settings
static const u1_t PROGMEM APPEUI[8]={  };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8]={  };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = {  };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

// Schedule LoRa TX every this many seconds
const unsigned TX_INTERVAL = 60;

static uint8_t mydata[] = "test_bd_v1";
static osjob_t sendjob;

// LoRa module pin mapping: Big Dot v1
const lmic_pinmap lmic_pins = {
  .nss = D12,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = D5,
  .dio = {D6, D1, LMIC_UNUSED_PIN},
};

// LED strip
#define RGB_PIN      A1
#define NUMPIXELS    24
#define DELAYVAL     25
Adafruit_NeoPixel led_strip(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// PMS7003 sensor
const int pmsRSTpin = A4;
PMS pms(Serial1);
PMS::DATA data;

// BME280 sensor
BME280 bme680;
float humidity = 0;
float pressure = 0;
float temp = 0;

// SSD1306 (OLED) display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* clock=*/ D8, /* data=*/ D7, /* reset=*/ U8X8_PIN_NONE);

void setup()
{
  // USB Serial debug
  while (!Serial);
  Serial.begin(9600);

  // PMS7003 settings
  Serial1.begin(9600);
  pms.passiveMode();
  pinMode(pmsRSTpin, OUTPUT);
  Serial.println("Started PMS.");

  Wire.begin();
  
  // OLED screen settings
  //display.setI2CAddress(0x3C);
  //display.setBusClock(1);
  display.setPowerSave(0);
  display.begin();
  
  // OLED welcome screen
  display.clearBuffer();
  display.setFont(u8g_font_profont11);
  display.setCursor(4,40); display.print("librelab::Big Dot v1");
  display.sendBuffer();
  Serial.println("Started OLED display.");
  
  // BME280 settings
  bme680.setI2CAddress(0x77);
  bme680.beginI2C();
  Serial.println("Started BME680.");

  // LED strip
  led_strip.begin();
  led_strip.clear();
  led_update(100);
  Serial.println("Started LED strip.");

  // LMIC init
  os_init();
    
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setDrTxpow(DR_SF7,14);
  LMIC_selectSubBand(1);
    
  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

// Main
void loop()
{
  // Read and print PM values
  pms_update();
  Serial.println("Updated PMS values.");

  // Read and print env. sensor values
  bme_update();
  Serial.println("Updated temp/hum/pressure values.");

  // Wait a minute before running again
  delay(10000);

  // Start LoRa scheduled TX
  os_runloop_once();

}

// PMS read func
void pms_update()
{
  pms.wakeUp();
  digitalWrite(pmsRSTpin, HIGH);
  delay(30000);
  pms.requestRead();
  if (pms.read(data, 10000))
  {
    // PM 1.0
    //display.clearBuffer();
    //display.setCursor(1, 10); display.print("PM 1.0 (ug/m3): ");
    //display.setCursor(95, 10); display.print(data.PM_AE_UG_1_0);
    //display.sendBuffer();
    //Serial.print("PM 1.0 (ug/m3): ");
    //Serial.println(data.PM_AE_UG_1_0);

     // PM 2.5
    display.clearBuffer();
    display.setCursor(1, 10); display.print("PM 2.5 (ug/m3): ");
    display.setCursor(95, 10); display.print(data.PM_AE_UG_2_5);
    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);

    // PM 10
    display.setCursor(1, 20); display.print("PM 10 (ug/m3): ");
    display.setCursor(95, 20); display.print(data.PM_AE_UG_10_0);
    display.sendBuffer();
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);
    
    // Update LED strip)
    // TODO: create a scale w/ 3 values of PM
    led_update(data.PM_AE_UG_2_5);
    Serial.println("Updated LED strip values.");
  }
  pms.sleep();
  digitalWrite(pmsRSTpin, LOW);
}

// BME680 read func
void bme_update()
{
  humidity = bme680.readFloatHumidity();
  display.setCursor(1, 30); display.print("Humidity: ");
  display.setCursor(95, 30); display.print(humidity, 0);
  Serial.print("Humidity: ");
  Serial.println(humidity);

  pressure = bme680.readFloatPressure();
  display.setCursor(1, 40); display.print("Pressure: ");
  display.setCursor(95, 40); display.print(pressure, 0);
  Serial.print("Pressure: ");
  Serial.println(pressure);

  temp = bme680.readTempF();
  display.setCursor(1, 50); display.print("Temperature: ");
  display.setCursor(95, 50); display.print(temp, 2);
  display.sendBuffer();
  Serial.print("Temperature (F): ");
  Serial.println(temp);
}

// LED strip update func
void led_update(int color_val)
{
  int color_set = 0;
  switch (color_val) {
    case 0 ... 4:
      color_set = 50;
      break;
    case 5 ... 10:
      color_set = 100;
      break;
    case 11 ... 20:
      color_set = 150;
      break;
    case 21 ... 100:
      color_set = 200; 
      break;
    case 101 ... 200:
      color_set = 250;
      break;
  }
  led_strip.clear();
  for (int i=0; i<NUMPIXELS; i++) {
    led_strip.setPixelColor(i, led_strip.Color(0, color_set, 0));
    led_strip.show();
  }
}

// LoRa comm handling
void printHex2(unsigned v) 
{
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void onEvent (ev_t ev)
{
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("LoRa: EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("LoRa: EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("LoRa: EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("LoRa: EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("LoRa: EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("LoRa: EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            LMIC_setLinkCheckMode(0);
            break;

        case EV_JOIN_FAILED:
            Serial.println(F("LoRa: EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("LoRa: EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("LoRa: EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("LoRa: Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("LoRa: received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("LoRa: EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("LoRa: EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("LoRa: EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("LoRa: EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("LoRa: EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("LoRa: EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("LoRa: EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("LoRa: EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j)
{
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("LoRa: Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}
