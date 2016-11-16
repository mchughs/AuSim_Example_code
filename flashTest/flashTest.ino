/* 
 *  Samuel McHugh
 *  This is a set of LED tests secifically in an attempt to model a muzzle flash
 *  6/30/2016
 */
 
#include <Adafruit_NeoPixel.h>
#include <Math.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define NS1PIN 24
#define NS2PIN 30

#define STEP1 1 
#define STEP2 2
#define STEP3 3

//buffer speed
#define waitms 100

//ramp up/down time on flash5
#define ramp_speed 1000

#define PERIOD 10000

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel neostr1 = Adafruit_NeoPixel(5, NS1PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neostr2 = Adafruit_NeoPixel(5, NS2PIN, NEO_RGB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

unsigned long seed = 0;

void setup() {
  // put your setup code here, to run once:
   neostr1.begin();
   neostr1.show(); // Initialize all pixels to 'off'
   neostr2.begin();
   neostr2.show(); // Initialize all pixels to 'off'

   //Debug
   Serial.begin(9600);
}

void loop() {
 seed++;
 
 //store time passed
 unsigned long time_elaps = seed * waitms;
 
  //m_flash1();
  //m_flash_2(time_elaps);        // subroutine with time passed in.
  //m_flash_3(time_elaps);
  //m_flash_4(time_elaps);
  //m_flash_5();
  m_flash_7(time_elaps);
 neostr1.show();              //display the buffer
 delay(waitms);
}



void m_flash1(){
  if (seed % waitms <= waitms/2) {
    neostr1.setPixelColor(1, neostr1.Color(5*seed,5*seed,5*seed));
  } else if (seed > waitms/2 && seed <= waitms) {
     Serial.println(255 - (5*(seed%(waitms/2))));
    //neostr1.setPixelColor(1, neostr1.Color(255 - pow(1.111,seed/4) , 255 - pow(1.112,seed/4),255 - pow(1.113,seed/4)));
    neostr1.setPixelColor(1, neostr1.Color(255 - (5*(seed%(waitms/2))) + 130 ,255 - (5*(seed%(waitms/2))) + 50 ,255 - (5*(seed%(waitms)/2)))
    );
  } else if (seed > waitms) {
    neostr1.setPixelColor(1, neostr1.Color(0,0,0));
  }
}

//linear muzzle flash
void m_flash_2(unsigned long time_elaps){
  //computes a value 0 to 1 based on the time-elapsed
  float value = ( float(time_elaps % (2*ramp_speed)) / float(ramp_speed) );

  //stage one: ramp up all rgb values to the full brighness of 255
  if (time_elaps % (2*ramp_speed) <= ramp_speed) {
    neostr1.setPixelColor(2, neostr1.Color(value * 255,
                                           value * 255,
                                           value * 255));
  //stage two: decrease all rgb values, but decrease the g and b values slightly faster.
  } else if (time_elaps % (2*ramp_speed) > ramp_speed){
    neostr1.setPixelColor(2, neostr1.Color(258 - (value * 255),
                                           258 - 1.01*(value * 255),
                                           258 - 1.01*(value * 255)));
  }
}

//variation on the ramp_down stage
void m_flash_3(unsigned long time_elaps){
  float value = ( float(time_elaps % (2*ramp_speed)) / float(ramp_speed) );
  
  if (time_elaps % (2*ramp_speed) <= ramp_speed) {
    neostr1.setPixelColor(1, neostr1.Color(value * 255,
                                           value * 255,
                                           value * 255));
  } else if (time_elaps % (2*ramp_speed) > ramp_speed){
    neostr1.setPixelColor(1, neostr1.Color(258 - (value * 255),
                                           260 - 1.02*(value * 255),
                                           260 - 1.02*(value * 255)));
  }
}

//variation on the ramp down stage
void m_flash_4(unsigned long time_elaps){
  float value = ( float(time_elaps % (2*ramp_speed)) / float(ramp_speed) );
  
  if (time_elaps % (2*ramp_speed) <= ramp_speed) {
    neostr1.setPixelColor(3, neostr1.Color(value * 255,
                                           value * 255,
                                           value * 255));
  } else if (time_elaps % (2*ramp_speed) > ramp_speed){
    neostr1.setPixelColor(3, neostr1.Color(258 - (value * 255),
                                           262 - 1.1*(value * 255),
                                           262 - 1.1*(value * 255)));
  }
}

void m_flash_5(){
  uint32_t fclr[10];
  fclr[0] = neostr1.Color(100, 70, 10);
  fclr[1] = neostr1.Color(175, 175, 50);
  fclr[2] = neostr1.Color(230, 225, 70);
  
  fclr[3] = neostr1.Color(175, 130, 40);
  fclr[4] = neostr1.Color(60, 40, 0);
  fclr[5] = neostr1.Color(30,15, 0);
  fclr[6] = neostr1.Color(0, 0, 0);
  fclr[7] = neostr1.Color(0, 0, 0);
  fclr[8] = neostr1.Color(0, 0, 0);
  fclr[9] = neostr1.Color(0, 0, 0);
  neostr1.setPixelColor(0,fclr[seed % 10]);
}



void m_flash_6(unsigned long time_elaps){
  float value = ( float(time_elaps % (ramp_speed)) / float(ramp_speed) );
  neostr1.setPixelColor(1, neostr1.Color(250,200,100));

  /*

  switch(time_elaps){
    case STEP1: break;
    case STEP2: break;
    case STEP3: break;
  }
  */
  
  if (time_elaps % (4*ramp_speed) <= ramp_speed) {
    neostr1.setPixelColor(3, neostr1.Color(value * 250,
                                           value * 200,
                                           value * 100));
  } else if ((time_elaps % (4*ramp_speed) > ramp_speed)
          && (time_elaps % (4*ramp_speed) <= 2*ramp_speed)){
    neostr1.setPixelColor(3, neostr1.Color(250 - .5 * value * 250,
                                             200 - .5 * value * 200,
                                             100 - .5 * value * 100));
  } else {
    neostr1.setPixelColor(3, neostr1.Color(150 - (value * 255),
                                           100 - (value * 255),
                                           50 - (value * 255)));
  }
  Serial.println(value);
}

void m_flash_7(unsigned long time_elaps){
  int cycle_time = (time_elaps % PERIOD);
  float value = float( cycle_time % ((PERIOD/4)+1) )/float(PERIOD/4);
  if(cycle_time <= PERIOD/4){
    Serial.println(value);
    neostr1.setPixelColor(3, neostr1.Color(value * 250,
                                           value * 200,
                                           value * 100));
                                           
  } else if (cycle_time > PERIOD/4 && cycle_time <= 2*PERIOD/4){ 
    neostr1.setPixelColor(3, neostr1.Color(250 - .5 * value * 250,
                                           200 - .5 * value * 200,
                                           100 - .5 * value * 100));
    
  } else if (cycle_time > 2*PERIOD/4 && cycle_time <= 3*PERIOD/4){
    neostr1.setPixelColor(3, neostr1.Color(125 - .5 * value * 125,
                                           100 - .5 * value * 100,
                                           50 - .5 * value * 50));
  } else {
    neostr1.setPixelColor(3, neostr1.Color(63 - value * 63,
                                           50 - value * 50,
                                           25 - value * 25));
  }
}

int component_generate(){
  
  }

void display_led(int[3]){ //[r,g,b]

  
  }


