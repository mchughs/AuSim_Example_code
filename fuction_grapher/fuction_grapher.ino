 /* Samuel McHugh
  * This program takes a desired resolution, a set of (time,rgb_value) points, and a period.
  * This program then displays a time-history-dependent blink based on those parameters.
  * 7/5/2016
  */

#include <Adafruit_NeoPixel.h>
#include <Math.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define NS1PIN 24
#define NS2PIN 30
#define WAIT 10   //resolution time
#define PERIOD 6000 
#define LENGTH 6 //size of endpoint array

Adafruit_NeoPixel neostr1 = Adafruit_NeoPixel(5, NS1PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neostr2 = Adafruit_NeoPixel(5, NS2PIN, NEO_RGB + NEO_KHZ800);


unsigned long count = 0;
float time_values  [LENGTH] = {0,100,200,300,400,500}; //first x should always be 0
float r_values     [LENGTH] = {0,150,255,75,75,0};     //first and last y should always be 0
float p_time_values[LENGTH] = {50,150,250,350,450,550};
float g_values     [LENGTH] = {0,100,255,50,40,0}; 
float b_values     [LENGTH] = {0,50,100,30,0,0}; 

void setup() {
   neostr1.begin();
   neostr1.show(); // Initialize all pixels to 'off'
   neostr2.begin();
   neostr2.show(); // Initialize all pixels to 'off'

   //Debug
   Serial.begin(9600);
}

void loop() {
  count++;
  unsigned long time_elaps = count*WAIT;
  
  float interpolated_r = component_generator(time_values, r_values, time_elaps % PERIOD);
  float interpolated_g = component_generator(time_values, g_values, time_elaps % PERIOD);
  float interpolated_b = component_generator(time_values, b_values, time_elaps % PERIOD);
  
  display_LED(interpolated_r,interpolated_g,interpolated_b);
  neostr1.show();
  delay(WAIT);
}

//calculates the interpolated y based on the parameters set by the given endpoint lists
float component_generator(float x[], float y[], float used_x){
  for (int i = 1; i < LENGTH; i++) {
    if (x[i] >= used_x) {
      return ( ( ((y[i]-y[i-1]) / (x[i]-x[i-1])) * (used_x - x[i-1]) ) + y[i-1]); //slope*x + y-intercept
    } else if (used_x > x[LENGTH-1]) {
      return 0; //if we've exceeded our endpoints return the rgb-value as 0
    }
  }
}

void display_LED(float r_value, float g_value, float b_value){
  neostr1.setPixelColor(1,r_value,g_value,b_value);
}

