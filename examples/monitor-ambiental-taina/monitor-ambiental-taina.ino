/*
 * Monitamento Ambiental Tainã v1.0
 * Oficina Extensão Comunitária 2024 
 * Aniversário de 35 Anos!
 *
 * Partes:
 * - Placa Big Dot v2
 * - Placa de Extensão Tainã v1.4
 * - Sensor de umidade (HD-38)
 * - Sensor de materia particulada (PMS7003)
 * - Sensor de monitoramento atmosférico (BME280)
 * - Tela OLED 1.3in (SH1106)
 * - Modulo WiFi ESP-12F (firmware AT 1.7.4.0)
 * 
 * Bibliotecas:
 * - PMserial v.1.2.0 (sensor de materia particulada)
 * - U8g2 v2.31.2 (tela OLED)
 * - Sparkfun BME280 v2.0.9 (atmos. sensor)
 * - WiFiEspAT v1.5.0 (WiFi)
 * - TBPubSubClient v2.10.0 (MQTT) 
 * - RTClib v2.1.4 (Relógio)
 * 
 */

#include <Wire.h>
#include <RTClib.h>
#include <U8g2lib.h>
#include <PMserial.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <SparkFunBME280.h>
#include "wiring_private.h"
#include "private_settings.h"

// Módulo ESP-12F
// Serial2 na SERCOM2 da placa Big Dot v2
#define PIN_SERIAL2_RX   (15ul)            // Pino D6
#define PIN_SERIAL2_TX   (14ul)            // Pino D5
#define PAD_SERIAL2_TX   (UART_TX_PAD_2)   // SERCOM2.2
#define PAD_SERIAL2_RX   (SERCOM_RX_PAD_3) // SERCOM2.3

// Configuracao do MQTT
#define MQTT_KEEPALIVE   15000
#define SENSOR1_TOPIC1   "/tc/sensor_1/pm2_5"
#define SENSOR1_TOPIC2   "/tc/sensor_1/pm10"
#define SENSOR2_TOPIC1   "/tc/sensor_2/umidade"
#define SENSOR2_TOPIC2   "/tc/sensor_2/temperatura"
#define SENSOR2_TOPIC3   "/tc/sensor_2/pressao"
#define SENSOR3_TOPIC1   "/tc/sensor_3/umidade_solo"

// SENSOR1
// Sensor PMS7003
SerialPM pms(PMSx003, Serial1);
const int pmsRSTpin = D2;

// SENSOR2
// Sensor temp.
BME280 bme280;
float humidity = 0;
float pressure = 0;
float temp = 0;
String temp_var;

// SENSOR3
// Sensor HD-38
const int soil_sensor = A4;
int soil_read = 0;

// Relógio (RTC)
RTC_PCF8523 rtc;

// Módulo WiFi
int status = WL_IDLE_STATUS;
WiFiClient ESPclient;
PubSubClient client(ESPclient);

// Inicia tela
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, D8, D7, U8X8_PIN_NONE);

// Serial2 para o módulo WiFi
Uart Serial2(&sercom2, PIN_SERIAL2_RX, PIN_SERIAL2_TX, PAD_SERIAL2_RX, PAD_SERIAL2_TX);

void setup()
{
  Serial.begin(115200);
  Serial.println("[BigDot] Saida serial USB ativada.");

  // Inicia sensor PM
  pms.init();
  pinMode(pmsRSTpin, OUTPUT);
  digitalWrite(pmsRSTpin, HIGH);
  Serial.println("[PMS7003] Iniciando o sensor de materia particulada.");
  
  // LED de status WiFi
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Inicia tela
  Wire.begin();
  //display.setI2CAddress(0x3C);
  //display.setBusClock(1);
  //display.setPowerSave(0);
  display.begin();
  
  // Tela inicial
  display.clearBuffer();
  display.setFont(u8g_font_profont11);
  display.setCursor(2,10); display.print(":::::::::::::::::::::");
  display.setCursor(2,20); display.print("                     ");
  display.setCursor(2,30); display.print("Monitor Ambiental, v1"); 
  display.setCursor(2,40); display.print("CASA DE CULTURA TAINã"); 
  display.setCursor(2,50); display.print("              35 Anos");
  display.setCursor(2,60); display.print(":::::::::::::::::::::");
  display.sendBuffer();
  Serial.println("[OLED] Iniciando a tela.");
  
  // Configuração sensor atmos.
  bme280.setI2CAddress(0x77);
  bme280.beginI2C();
  Serial.println("[BME280] Iniciando sensor temp/umidade/pressao.");

  // Verifica se o RTC foi conectado propriamente
  if (!rtc.begin()) {
    Serial.println("[RTC] Relógio não encontrado.");
  }
  
  // Inicializa o ESP-12F
  pinPeripheral(15, PIO_SERCOM); 
  pinPeripheral(16, PIO_SERCOM);
  Serial2.begin(115200);
  WiFi.init(&Serial2);
  
  // Verifica se o ESP-12F está conectado
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("[ESP-12F] Modulo WiFi não foi encontrado.");
  } else {
    // Conecta na rede WiFI
    wifi_connect();
    
    // Conexão ao MQTT
    //ESPclient.setCACert(CACERT);
    //ESPclient.setFingerprint(FINGERPRINT);
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setKeepAlive(MQTT_KEEPALIVE);
    mqtt_connect();  
  }
}

void loop() 
{
  // Iniciando ciclo de leitura
  Serial.println("");
  Serial.print("[Big Dot] Identificação da mucua: ");
  Serial.println(MQTT_CLIENT_ID);
  Serial.println("[Big Dot] Iniciando o ciclo de leitura e envio de dados.");
  
  // Verifica conexão WiFi, caso perdida
  wifi_connect();
  
  // Verifica conexão MQTT, caso perdida
  if (!client.connected()) {
    mqtt_reconnect();
  } else {
    client.loop();
  }
  
  // Acordando e lendo o sensor PM
  Serial.println("[PMS7003] Lendo sensor de materia particulada.");
  digitalWrite(pmsRSTpin, HIGH);
  pms.wake();
  pms.read();
  delay(3 * 1000);
  
  // Limpa tela
  display.clearBuffer();

  // Leitura do relógio
  if (rtc.begin()) {
    DateTime now = rtc.now();
    display.setCursor(1, 10);
    display.print(" ");
    display.print(now.day());
    display.print("/");
    display.print(now.month());
    display.print("/");
    display.print(now.year());
    display.print(" ");
    display.print(now.hour());
    display.print(":");
    display.print(now.minute());
  }

  // Leitura sensor de umidade do ar
  humidity = bme280.readFloatHumidity();
  Serial.print("[BME280] Umidade (%): ");
  Serial.println(humidity);
  display.setCursor(8, 20); display.print("Umidade (%): ");
  display.setCursor(86, 20); display.print(humidity, 0);

  // Envio dos dados via MQTT
  temp_var = String(humidity);
  client.publish(SENSOR2_TOPIC1, temp_var.c_str());
  delay(1000);

  // Leitura sensor de temperatura
  temp = bme280.readTempC();
  Serial.print("[BME280] Temperatura (C): ");
  Serial.println(temp);
  display.setCursor(8, 30); display.print("Temp. (C): ");
  display.setCursor(86, 30); display.print(temp, 2);
  
  // Envio dos dados via MQTT
  temp_var = String(temp, 2);
  client.publish(SENSOR2_TOPIC2, temp_var.c_str());
  delay(1000);

  // Leitura da pressao atmosferica
  pressure = (bme280.readFloatPressure() / 100.0F);  
  Serial.print("[BME280] Pressão (hPa): ");
  Serial.println(pressure);
  display.setCursor(8, 40); display.print("Press.(hPa): ");
  display.setCursor(86, 40); display.print(pressure, 2);
  
  // Envio dos dados via MQTT
  temp_var = String(pressure, 2);
  client.publish(SENSOR2_TOPIC3, temp_var.c_str());
  delay(1000);

  // Leitura do sensor de umidade do solo
  Serial.println("[HD-38] Lendo sensor de umidade do solo.");
  soil_read = analogRead(soil_sensor); 
  Serial.print("[HD-38] Valor de leitura: ");
  Serial.println(soil_read);
  
  // Envio dos dados via MQTT
  temp_var = String(soil_read);
  client.publish(SENSOR3_TOPIC1, temp_var.c_str());
  delay(1000);

  // Leitura do sensor de materia particulada
  pms.read();
  if (pms.has_particulate_matter()) {
    // Coleta de PM2.5
    Serial.print("[PMS7003] PM 2.5 (ug/m3): ");
    Serial.println(pms.pm25);
    display.setCursor(8, 50); display.print("PM 2.5 (ug): ");
    display.setCursor(86, 50); display.print(pms.pm25);

    // Envio da concentração de PM2.5
    temp_var = String(pms.pm25);
    client.publish(SENSOR1_TOPIC1, temp_var.c_str());
    delay(1000);

    // Coleta de PM10
    Serial.print("[PMS7003] PM 10.0 (ug/m3): ");
    Serial.println(pms.pm10);
    display.setCursor(8, 60); display.print("PM 10 (ug): ");
    display.setCursor(86, 60); display.print(pms.pm10);
    
    // Envio da concentração de PM10
    temp_var = String(pms.pm10);
    client.publish(SENSOR1_TOPIC2, temp_var.c_str());
    delay(1000);
  }
  
  // Atualizacao da tela
  display.sendBuffer();
  
  // Desliga o sensor PMS
  Serial.println("[PMS7003] Dormindo por 30sec.");
  Serial.println("");
  pms.sleep();
  digitalWrite(pmsRSTpin, LOW); 
  // Tempo de espera:
  // 30sec para nova leitura
  delay(30 * 1000);
}

// Handler da Serial2
void SERCOM2_Handler()
{
  Serial2.IrqHandler();
}

//
// Funções de rede
//
// Conexão WiFi
void wifi_connect()
{
  if (status != WL_CONNECTED) {
    Serial.print("[ESP-12F] Tentando conectar ao SSID: ");
    Serial.println(WLAN_SSID);
    status = WiFi.begin(WLAN_SSID, WLAN_PASS);
  } else {
    Serial.println("[ESP-12F] Conexão WiFi ativa.");
    Serial.print("[ESP-12F] Força do signal WiFi (RSSI): ");
    Serial.println(WiFi.RSSI());
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

// Conexão MQTT
void mqtt_connect()
{
  Serial.print("[MQTT] Conectando ao broker MQTT: ");
  Serial.println(MQTT_SERVER);
  if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWD)) {
    Serial.println("[MQTT] Conectado ao servidor MQTT!");
      digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Serial.print("[MQTT] Conexão falhou, código de status = ");
    Serial.println(client.state());
    digitalWrite(LED_BUILTIN, LOW);
  }
}

// Reconexão MQTT:
// Em caso de perda
void mqtt_reconnect()
{
  for (int i = 1; i == 4; i++) {
    Serial.print("[MQTT] Tentando conexão... tentativa ");
    Serial.println(i);
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWD)) {
      Serial.println("[MQTT] Connectado ao servidor MQTT!");
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.println("[MQTT] Conexão falhou, tentando novamente.");
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
