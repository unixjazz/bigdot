#include <PMS.h>
#include <Adafruit_NeoPixel.h>

// LED constants
#define LED_PIN      A1
#define NUMPIXELS    24
#define BRIGHTNESS   20
#define DELAY_VAL    25

// LED ring
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
// Colors for LED ring
uint32_t init_color = strip.Color(25, 25, 25);
uint32_t light_green = strip.Color(120, 255, 92);
uint32_t dark_green = strip.Color(0, 255, 0);
uint32_t light_yellow = strip.Color(199, 255, 0);
uint32_t dark_yellow = strip.Color(255, 255, 0);
uint32_t light_orange = strip.Color(255, 212, 0);
uint32_t dark_orange = strip.Color(255, 165, 0);
uint32_t light_red = strip.Color(255, 100, 0);
uint32_t dark_red = strip.Color(255, 0, 0);

// PMS7003 sensor
const int pmsRSTpin = A4;
PMS pms(Serial1);
PMS::DATA data; 

void setup(){
  Serial.begin(9600);

  // LED ring
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  ledFill(init_color);
 
  
  Serial1.begin(9600);
  pms.passiveMode();
  pinMode(pmsRSTpin, OUTPUT);
  digitalWrite(pmsRSTpin, LOW);

}
void loop(){
  delay(10000);
  pms_update();

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
    //PM 1.0
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);
    // PM 2.5
    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);
    // PM 10
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);
    // Update LED strip
    led_update(data.PM_AE_UG_2_5);

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

    
    //If greater than 85 the LED will be RED
    default: 
    ledFill(dark_red);
    strip.show();
    break;
  }
}
