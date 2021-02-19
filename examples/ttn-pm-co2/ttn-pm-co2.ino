 /* 
  *  SDS sensorbox prototype, v.0.2
  *  
  *  DEV_NAME=bigdot
  *  DEV_ID=bigdot0
  *  LOC=loc0
  *  
  *  Sensor list:
  *  SENSOR1 = PMS7003 (pm2.5, pm10)
  *  SENSOR2 = Sensiron SCD30 (CO2, temp, humidity)
  *  
  *  Depends on:
  *  Arduino-lmic, PMS, Sparkfun SCD30 library
  *  
  *  Derives from:
  *  Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
  *  Copyright (c) 2018 Terry Moore, MCCI
  *  Copyright (c) 2018 Brent Rubell, Adafruit Industries
  *  
  */

#include <lmic.h> 
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <PMS.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include "ttn-credentials.h"

// debug mode
#define DEBUG

// init sensor objs
PMS pms(Serial1);
PMS::DATA data;
SCD30 CO2Sensor;

// TTN credentials
void os_getArtEui (u1_t *buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t *buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t *buf) { memcpy_P(buf, APPKEY, 16); }

// payload to TTN gateway
static uint8_t payload[14];
static osjob_t sendjob;

// sleep interval in X minutes
//const unsigned SLEEP_INTERVAL = 1;
const unsigned TX_INTERVAL = 60;

// pin mapping for Big Dot, rev2
const lmic_pinmap lmic_pins = {
  .nss = 23,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {15, 9, LMIC_UNUSED_PIN},
  .rxtx_rx_active = 0,
};

// pin mapping for RST function of PMS7003
const int pmsRSTpin = 8;

// LoRa event handling
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
            Serial.println(F("EV_BEACON_MISSED"));
            #endif
            break;
        case EV_BEACON_TRACKED:
            #ifdef DEBUG
            Serial.println(F("EV_BEACON_TRACKED"));
            #endif
            break;
        case EV_JOINING:
            #ifdef DEBUG
            Serial.println(F("EV_JOINING"));
            #endif
            break;
        case EV_JOINED:
            #ifdef DEBUG
            Serial.println(F("EV_JOINED"));
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
              Serial.print("artKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                  Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("nwkKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                 if (i != 0)
                   Serial.print("-");
                   Serial.print(nwkKey[i], HEX);
              }
              Serial.println("");
            }
            #endif
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            #ifdef DEBUG
            Serial.println(F("EV_JOIN_FAILED"));
            #endif
            break;
        case EV_REJOIN_FAILED:
            #ifdef DEBUG
            Serial.println(F("EV_REJOIN_FAILED"));
            #endif
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Standby mode
            //Serial.println(":: now in standby ::");
            digitalWrite(LED_BUILTIN, LOW);
            // rtc.standbyMode() and detach USB
            //USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;
            //#endif
            // __WFI();
            // Return from Standby and re-attach USB
            //#ifdef DEBUG
            //USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
            //Serial.println("");
            //Serial.println(":: waking up from standby ::");
            //digitalWrite(LED_BUILTIN, HIGH);
            //#endif
            // Schedule next transmission
            do_send(&sendjob);
            delay(1*TX_INTERVAL*1000);
            break;
        case EV_LOST_TSYNC:
            #ifdef DEBUG
            Serial.println(F("EV_LOST_TSYNC"));
            #endif
            break;
        case EV_RESET:
            #ifdef DEBUG
            Serial.println(F("EV_RESET"));
            #endif
            break;
        case EV_RXCOMPLETE:
            #ifdef DEBUG
            Serial.println(F("EV_RXCOMPLETE"));
            #endif
            break;
        case EV_LINK_DEAD:
            #ifdef DEBUG
            Serial.println(F("EV_LINK_DEAD"));
            #endif
            break;
        case EV_LINK_ALIVE:
            #ifdef DEBUG
            Serial.println(F("EV_LINK_ALIVE"));
            #endif
            break;
        case EV_TXSTART:
            #ifdef DEBUG
            Serial.println(F("EV_TXSTART"));
            #endif
            break;
        default:
            #ifdef DEBUG
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            #endif
            break;
    }
}

// LoRa transmission loop
void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        #ifdef DEBUG
        Serial.println(F("OP_TXRXPEND, not sending"));
        #endif
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
        // PMS start and wait 20s for stable readings
        digitalWrite(pmsRSTpin, HIGH);
        pms.wakeUp();
        delay(20000);
        
        // read the CO2 sensor values
        CO2Sensor.readMeasurement();
        float co2 = CO2Sensor.getCO2();
        float temp = CO2Sensor.getTemperature();
        float rel_hum = CO2Sensor.getHumidity();
        #ifdef DEBUG
        Serial.print("CO2: ");
        Serial.print(co2);
        Serial.println( "ppm");
        Serial.print("Temp: ");
        Serial.print(temp);
        Serial.println(" C");
        Serial.print("Humidity: ");
        Serial.print(rel_hum);
        Serial.println(" \%");
        #endif
        
        // convert payload for CO2, temp, humidity
        uint32_t co2_conv = (uint32_t)(co2 * 100);
        // payload byte array
        payload[0] = (co2_conv & 0xFF);
        payload[1] = (co2_conv >> 8) & 0xFF;
        payload[2] = (co2_conv >> 16) & 0xFF;
        payload[3] = (co2_conv >> 24) & 0xFF;

        uint16_t payloadTemp = (uint16_t)(temp * 100);
        payload[4] = payloadTemp >> 8;
        payload[5] = payloadTemp & 0xFF;
        
        uint16_t payloadHumid = (uint16_t)(rel_hum * 100);
        payload[6] = payloadHumid >> 8;
        payload[7] = payloadHumid & 0xFF;
        
        // collect PMS values
        pms.requestRead();
        if (pms.readUntil(data)) {
          #ifdef DEBUG
          Serial.print("PM 2.5 (ug/m3): ");
          Serial.println(data.PM_AE_UG_2_5);
          #endif
          uint16_t pm2_5 = data.PM_AE_UG_2_5;
          payload[8] = pm2_5 >> 8;
          payload[9] = pm2_5 & 0xFF;

          #ifdef DEBUG
          Serial.print("PM 10.0 (ug/m3): ");
          Serial.println(data.PM_AE_UG_10_0);
          #endif
          uint16_t pm10 = data.PM_AE_UG_10_0;
          payload[10] = pm10 >> 8;
          payload[11] = pm10 & 0xFF;
        }
        digitalWrite(pmsRSTpin, LOW);
        
        // params: (port_number, payload_array, payload-size, request-ack?)
        LMIC_setTxData2(1, payload, sizeof(payload)-1, 0);
        digitalWrite(LED_BUILTIN, LOW);
    }
}

// MCU setup
void setup() {
    delay(1000);
    
    #ifdef DEBUG
    while (! Serial);
    Serial.begin(115200);
    Serial.println(F("Starting bigdot0 \\m/"));
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    #else
    digitalWrite(LED_BUILTIN, LOW);
    USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;
    #endif
    
    // Serial1 for PM sensor
    Serial1.begin(9600);
    pms.passiveMode();
    // PMS RST pin
    pinMode(pmsRSTpin, OUTPUT);
    digitalWrite(pmsRSTpin, LOW);
    
    // start CO2 + temp + humidity sensors
    Wire.begin();
    CO2Sensor.begin();
    CO2Sensor.setAutoSelfCalibration(1);    // turn on self-calibration
    CO2Sensor.setMeasurementInterval(60);   // in seconds
    CO2Sensor.setAltitudeCompensation(174); // in meters
    CO2Sensor.setAmbientPressure(835);      // in mBar
    CO2Sensor.readMeasurement();
    
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded
    LMIC_reset();
    // Disable link-check mode and ADR, because ADR tends to complicate testing
    LMIC_setLinkCheckMode(0);
    // Set the data rate to Spreading Factor 7.  This is the fastest supported rate for 125 kHz channels, and it
    // minimizes air time and battery power. 
    // Set the transmission power: 14 dBi (25 mW)
    LMIC_setDrTxpow(DR_SF7,14);
    // in the US, with TTN, it saves join time if we start on subband 1 (channels 8-15)
    LMIC_selectSubBand(1);
    // Start job (sending automatically starts OTAA)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
