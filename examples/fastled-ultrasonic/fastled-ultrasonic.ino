/*
 * Ostenda proto, v.0.5
 * 
 * Dependencies: FastLED v3.3, PubSub v2.8
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * See AUTHORS and LICENSE files for more details
 *
 */

#include <FastLED.h>

// Ultrasonic conf
#define SENSORPIN A0                  // Analog Pin for reading Ultrasonic Sensor
#define DELAYVAL 50                   // Delay for Stability
#define RANGE 140                     //distance in cm

// LED strip conf
#define DATA_PIN           9                         // Any PWM Output Pin on the Feather M0
#define LED_TYPE           WS2812B                   // If you use Neopixels
#define COLOR_ORDER        GRB                       // GRB for WS2182B 
#define NUM_LEDS           1                         // number of LEDs
#define FRAMES_PER_SECOND  120                       // redefine if needed
#define BRIGHTNESS         5                        // set initial brightness: 2% (0 - 255)
#define TEMPERATURE_1 CoolWhiteFluorescent          //Set color correction

CRGB leds[NUM_LEDS];


// Ultrasonic vars //
int rxPin = 5;          // initial pullup pin to send Ultrasonic Ping                      
int DISTANCE;           // Mapped Parameter used to drive brightness



// SAMPLE BUFFER Vars //

//settings for smoothing
const int NUMSAMPLE = 10;           //Samples taken. More = Smoother but also slower.
const int DELAY = 4;                 //Delay to keep it stable

//sample variables for smoothing (leave these alone)
int SAMP[NUMSAMPLE] = {0};          //value of sample
int INDEX = 0;                       //number of sample from 1-[NUMSAMPLE]
int TOTAL = 0;                      //running total of sample values
int AVG = 0;                        //average of sample values over [NUMSAMPLE]



void setup() {
  
  Serial.begin(115200);             // Specify Baud Rate
  pinMode(rxPin, OUTPUT);           // Define trigger pin as Output
 
  
  // LED STRIP SETUP //
  
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  leds[0] = CRGB::White;                              //Set initial color to White
  FastLED.setTemperature( TEMPERATURE_1 );            //Set color temp to specified temp in header
  FastLED.setBrightness(BRIGHTNESS);                  //Set initial Brightness
  FastLED.show();                                      //Write settings to LEDs
  delay(20);                                           //Delay for Stability

}


void loop() {

  
  digitalWrite(rxPin, HIGH);             //Pull up trigger pin to HIGH, starting Ultrasonic ping       
  delay(DELAY);                          //Hold trigger for a slight delay
  digitalWrite(rxPin, LOW);              //Pull down trigger pin to LOW, ready for next ping
  TOTAL -= SAMP[INDEX];                  
  SAMP[INDEX] = analogRead(SENSORPIN);   //Write analog readings to sample index
  TOTAL += SAMP[INDEX++];                //Add indexed readings to running total, increment index
  if (INDEX >= NUMSAMPLE) {              //checks if limit of samples is reached
    INDEX = 0;                           //resets INDEX to 0, restarting the process
  }
  delay(DELAY);
  AVG = ((TOTAL * 0.5) / NUMSAMPLE);        //Calculates the average
  DISTANCE = map(AVG, 0, RANGE, 255, 0);     //Maps average to 0-255 value for LED control
  //Serial.println(DISTANCE);                //Uncomment if you want to stream values to Serial
  delay(DELAY);

// Subtle Glow if sensor values are lower than range threshold
  if (AVG >= RANGE) {
    
    uint16_t fade = beatsin16(4, 10, 50);       //sin wave function with arguments (BPM, Low Value, High Value)
    leds[0] = CRGB::White;                      //Sets color to White
    FastLED.setBrightness(fade);                //Assigns sin function to brightness
    FastLED.show();                             //Write to LEDs

// Otherwise map sensor values to LED brightness
  } else {

    leds[0] = CRGB::White;                      //Sets color to White  
    FastLED.setBrightness(DISTANCE);            //Map average distance readings to brightness
    FastLED.show();                             //Write to LEDs
      
  }
  
  FastLED.show();
  delay(DELAYVAL); // Pause before next pass through loop

}
