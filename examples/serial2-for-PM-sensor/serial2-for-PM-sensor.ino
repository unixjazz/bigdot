/*
 * 
 * Serial2 example for librelab::Big Dot
 * Depends on PMS library, v. 1.1.0
 * 
 */
#include <Arduino.h>
#include "wiring_private.h"
#include "PMS.h"

// Serial2 on SERCOM2 for Big Dot v1
#define PIN_SERIAL2_RX   (15ul)              // Pin D6
#define PIN_SERIAL2_TX   (14ul)              // Pin D5
#define PAD_SERIAL2_TX   (UART_TX_PAD_2)     // SERCOM2.2
#define PAD_SERIAL2_RX   (SERCOM_RX_PAD_3)   // SERCOM2.3

// Instantiate Serial2
Uart Serial2(&sercom2, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);
void SERCOM2_Handler()
{
  Serial2.IrqHandler();
}

// Declare PMS RESET pin
const int pmsRSTpin = A4;

// PMS on Serial2
PMS pms(Serial2);
PMS::DATA data;

void setup()
{
  // Set pins for Serial2
  pinPeripheral(15, PIO_SERCOM); 
  pinPeripheral(16, PIO_SERCOM);

  Serial.begin(115200); // SerialUSB
  Serial2.begin(9600);  // Serial2 (D6: TX, D5: TX)
  pms.passiveMode();

  // Set RESET pin
  pinMode(pmsRSTpin, OUTPUT);
  digitalWrite(pmsRSTpin, LOW);
}

void loop()
{
  Serial.println("PMS: Wake up");
  pms.wakeUp();
  digitalWrite(pmsRSTpin, HIGH);
  delay(3000);
  
  pms.requestRead();
  Serial.println("PMS: Wait 10 sec. for sensor read");
  if (pms.readUntil(data))
  {
    Serial.println("Data:");
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);

    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);

    Serial.print("PM 10.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);
  }
  else
  {
    Serial.println("PMS: Got no data.");
  }

  Serial.println("PMS: sleep for 60 sec.");
  pms.sleep();
  digitalWrite(pmsRSTpin, LOW);
  delay(60000);
}
