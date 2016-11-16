/* Samuel McHugh
   Matrix displays data passed from the ORVCtrl
    - Measured sensor data + requests from the host
    - Text to be displayed
    - The countdown time and test number

   Sketch uses 18,750 bytes (7%) of program storage space.
   Global variables use 690 bytes (8%) of dynamic memory, leaving 7,502 bytes for local variables.
*/

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library
#include <Fonts\TomThumb.h>
#include <Fonts\FreeSerifBoldItalic9pt7b.h> //Fonts
#include <Fonts\FreeMono9pt7b.h>
#include <Wire.h>
#include "CtrlState.h"
#include "ProgressState.h"
#include "MarqueeLine.h"
#include "MarqueeBitmap.h"
#include <avr/pgmspace.h>

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#else
setoffalarm now
#endif

#define Use32linePanel 1

#define CLK 11  // MUST be on PORTB! (Use pin 11 on Mega)

#if Use32linePanel
#define LAT 10
#else
#define LAT A3
#endif
#define OE  9   // Output Enable
#define A   A0
#define B   A1
#define C   A2
#define D   A3

#define WIDTH 64 //Width of matrix

enum DisplayMode { MODE_DEFAULT = 0, MODE_DIAGNOSTIC = 1, MODE_PROGRESS = 2, MODE_MARQUEE = 3 } ;

//signature for identifying which type of data will be coming across the wire.
// "DH","PD","MS","DD","RS","ML","BM"
enum Headers { DIAGNOSTICS_HEADER = 18500, PROGRESS_HEADER = 17488, MARQUEE_HEADER = 21325,
               DEFAULT_HEADER = 17476, RESET = 21330, MARQUEE_LINE = 19533, BITMAP = 19778} ;

#define DEFAULT_FONT 0
#define DEFAULT_COLOR 0

#define BLINKRATE 1000 //in millis

#ifdef Use32linePanel
// Constuctor for 64x32 panel
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, true, 64);
#else
// Constuctor for 32x16 panel
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, true);
#endif

//<-----------StopLight States----------->
#define SLT_Off         0
#define SLT_Red         1
#define SLT_Yellow      2
#define SLT_Green       4
#define SLT_BlinkRed  (SLT_Red | SLT_Green)
#define SLT_BlinkYlw  (SLT_Yellow | SLT_Red)
#define SLT_BlinkGrn  (SLT_Green | SLT_Yellow)
#define SLT_Rotating  (SLT_Red | SLT_Green | SLT_Yellow)

char recv_buffer[32];
int  fresh_data_count = 0;
bool start_mark = false;

void setup()
{
  // Setup host communication
  Wire.begin(13);  // use I2C device address 13
  Wire.onReceive(receiveEvent);  // register event handler

  // Setup RGB LED Matrix
  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge
  matrix.setTextSize(1);

  // remove serial port debugger when releasing
  Serial.begin(9600);
}

// function prototypes
int  display_progress(ProgressState state);
void display_marquee(MarqueeLine mline[], MarqueeBitmap mbmp);
void display_diagnostics(ORVCtrlUnion state);

void loop() {
  static int mode = 0;
  static int prev_mode = 0;
  static ORVCtrlUnion orvcu = {0, 0};
  static ProgressState prog;
  static MarqueeLine mline[4];
  static MarqueeBitmap mbmp;
  static int hdr;
  const int nHdrBytes = 2;
  static int md_count = 0;
  static int linenum = 0;

  if (fresh_data_count > 0) {
    hdr = (recv_buffer[0] | recv_buffer[1] << 8);
    switch (hdr) {
      case DEFAULT_HEADER:
        mode = MODE_DEFAULT;
        break;
      case DIAGNOSTICS_HEADER :
        if (mode != MODE_DIAGNOSTIC) {
          prev_mode = mode; //Sets the mode aside to be called back later
        }
        memcpy( &orvcu, &recv_buffer[nHdrBytes], sizeof( orvcu ) );
        mode = MODE_DIAGNOSTIC;
        break;
      case PROGRESS_HEADER :
        start_mark = true;
        memcpy( &prog, &recv_buffer[nHdrBytes], sizeof( prog ) );
        mode = MODE_PROGRESS;
        break;
      case MARQUEE_HEADER :
        if (recv_buffer[2] != 0) {
          mode = MODE_MARQUEE;
        } else {
          mline[0].reset();
          mline[1].reset();
          mline[2].reset();
          mline[3].reset();

        //mbmp.reset();
          mode = MODE_DEFAULT;
        }
        break;
      case MARQUEE_LINE:
        static char * pmline;
        if (md_count == 0) {
          linenum = recv_buffer[2];
          pmline = (char*)&mline[linenum];
          memcpy(&pmline[0], &recv_buffer[nHdrBytes + 1], 29);
          md_count++;
        } else if (md_count == 1) {
          memcpy(&pmline[29], &recv_buffer[nHdrBytes], 30);
          md_count++;
        } else if (md_count == 2) {
          memcpy(&pmline[59], &recv_buffer[nHdrBytes], 6);
          md_count = 0;
          Serial.println((int)mline[linenum].x);
          Serial.println((int)mline[linenum].y);
          Serial.println((int)mline[linenum].font);
          Serial.println((int)mline[linenum].color);
          Serial.println(mline[linenum].text);
        }
        break;
      case BITMAP:
        static char * pmbmp = (char*)&mbmp; //It's a palindrome!
        if (md_count == 0) {
          memcpy(&pmbmp[0], &recv_buffer[nHdrBytes], 30);
          md_count++;
        } else if (md_count <= 10) { 
          memcpy(&pmbmp[29 + (md_count * 30)], &recv_buffer[nHdrBytes], 30);
          md_count++;
        } else if (md_count > 10) {
          md_count = 0;
        }
        break;
      case RESET :
        mode = prev_mode;
        break;
    }
    fresh_data_count = 0;
  }

  // Clear background
  matrix.fillScreen(0);
  matrix.setFont(0);

  switch (mode) {
    case MODE_DEFAULT:
      display_default();
      break;
    case MODE_DIAGNOSTIC:
      display_diagnostics(orvcu);
      break;
    case MODE_PROGRESS:
      prog.seconds_remaining = display_progress(prog);
      break;
    case MODE_MARQUEE:
      display_marquee(mline);//, mbmp);
      break;
  }
  start_mark = false;
  matrix.swapBuffers(true);
  delay(10);
}

void receiveEvent(int howMany) {
  if (Wire.available()) {
    Wire.readBytes(recv_buffer, howMany);
  }
  fresh_data_count = howMany;
}

void display_default() {
  matrix.setTextColor(matrix.Color888(150, 150, 150));
  matrix.setCursor(12, 1);
  matrix.print("Welcome");
  matrix.setCursor(25, 11);
  matrix.print("to");
  matrix.setCursor(13, 21);
  matrix.print("Oculus");
}

void display_marquee(MarqueeLine mline[]) { //, MarqueeBitmap mbmp) {
  static int scroll_position[4] = {WIDTH, WIDTH, WIDTH, WIDTH};
  int char_num = max(max(strlen(mline[0].text), strlen(mline[1].text)), max(strlen(mline[2].text), strlen(mline[3].text))); //number of characters in the string
  static int glyph_size[4];
  
  for (int j = 0; j < 4; j++) {
    switch (mline[j].font) {
      case 0:
        //TomThumb.yAdvance;
        glyph_size[j] = 4;
        break;
      case 1:
        //FreeSerifBoldItalic9pt7b.yAdvance;
        glyph_size[j] = 10;
        break;
      case 2:
        //FreeMono9pt7b.yAdvance;
        glyph_size[j] = 10;
        break;
    }
  }
  
  printMarquee(mline, scroll_position);

  for (int j = 0; j < 4; j++) {
    if ((int)(WIDTH - ((strlen(mline[j].text) + 1 ) * glyph_size[j])) > 0) {
      scroll_position[j] = 0;
    } else if (scroll_position[j] < -((char_num + 1 ) * glyph_size[j])) {
      scroll_position[j] = WIDTH;
    } else {
      scroll_position[j]--;
    }
  }

  //Pixel range from (0,0) to (w-1,h-1)
  //  const uint16_t colors[4] = {0, mbmp.color1, mbmp.color2, mbmp.color3};
  //  for (int j = 0; j < 512; j++) {
  //    char c = mbmp.bitmap[j];
  //    bool a2 = (c >> 8) & 1;
  //    bool a1 = (c >> 7) & 1;
  //    bool b2 = (c >> 6) & 1;
  //    bool b1 = (c >> 5) & 1;
  //    bool c2 = (c >> 4) & 1;
  //    bool c1 = (c >> 3) & 1;
  //    bool d2 = (c >> 2) & 1;
  //    bool d1 = (c >> 1) & 1;
  //    int color_A = 2 * a2 + a1;
  //    int color_B = 2 * b2 + b1;
  //    int color_C = 2 * c2 + c1;
  //    int color_D = 2 * d2 + d1;
  //    matrix.drawPixel(mbmp.x + (((4 * j))     % mbmp.szx), mbmp.y + (((4 * j))     / mbmp.szx), colors[color_A]);
  //    matrix.drawPixel(mbmp.x + (((4 * j) + 1) % mbmp.szx), mbmp.y + (((4 * j) + 1) / mbmp.szx), colors[color_B]);
  //    matrix.drawPixel(mbmp.x + (((4 * j) + 2) % mbmp.szx), mbmp.y + (((4 * j) + 2) / mbmp.szx), colors[color_C]);
  //    matrix.drawPixel(mbmp.x + (((4 * j) + 3) % mbmp.szx), mbmp.y + (((4 * j) + 3) / mbmp.szx), colors[color_D]);
  //  }
}

void printMarquee(MarqueeLine mline[], int scroll_position[]) {
  for (int j = 0; j < 4; j++) {
    switch (mline[j].font) {
        { case 0:
          const GFXfont *pfont = &TomThumb;
          matrix.setFont(pfont);
          break;
        }
        { case 1:
          const GFXfont *qfont = &FreeSerifBoldItalic9pt7b;
          matrix.setFont(qfont);
          break;
        }
        { case 2:
          const GFXfont *rfont = &FreeMono9pt7b;
          matrix.setFont(rfont);
          break;
        }
    }
    matrix.setCursor((int)mline[j].x + scroll_position[j], (int)mline[j].y);
    matrix.setTextColor((int)mline[j].color);
    matrix.print(mline[j].text);
  }
}

int display_progress(ProgressState prog) {
  static long start_time = millis();
  if (start_mark) { //resets the start_time if the timer is updated
    start_time = millis();
  }
  const uint16_t bold = matrix.Color888(129, 129, 129);
  matrix.setFont(&TomThumb);
  matrix.setTextColor(bold);

  matrix.setCursor(1, 5);
  matrix.print("Test type: " + String(prog.test_type));
  matrix.setCursor(1, 12);
  matrix.print(String(prog.current_test) + " of " + String(prog.total_test));
  matrix.setCursor(1, 19);
  matrix.print(String(prog.subject_name));

  int seconds_since_start = ((millis() - start_time) / 1000);
  matrix.setCursor(1, 26);
  int remaining = (prog.seconds_remaining - seconds_since_start);
  if (remaining < 1) {
    matrix.print("Test complete");
  } else {
    matrix.print( String( (remaining / 3600)) + ":" +
                  String(((remaining / 60) % 60)) + ":" +
                  String( (remaining % 60)) );
  }
  return (prog.seconds_remaining);
}

void display_diagnostics(ORVCtrlUnion orvcu) {

  uint16_t black  = matrix.Color888(0  , 0  , 0  );
  uint16_t dimgry = matrix.Color888(16 , 16 , 16 );
  uint16_t medgry = matrix.Color888(32 , 32 , 32 );
  uint16_t brtgry = matrix.Color888(48 , 48 , 48 );
  uint16_t bold   = matrix.Color888(129, 129, 129);
  uint16_t dimgrn = matrix.Color888(0  , 16 , 0  );
  uint16_t medgrn = matrix.Color888(0  , 97 , 0  );
  uint16_t brtgrn = matrix.Color888(0  , 129, 0  );
  uint16_t dimred = matrix.Color888(16 , 0  , 0  );
  uint16_t medred = matrix.Color888(97 , 0  , 0  );
  uint16_t brtred = matrix.Color888(129, 0  , 0  );
  uint16_t dimblu = matrix.Color888(0  , 0  , 16 );
  uint16_t medblu = matrix.Color888(0  , 0  , 97 );
  uint16_t brtblu = matrix.Color888(0  , 0  , 255);
  uint16_t medmgt = matrix.Color888(65 , 0  , 65 );
  uint16_t dimylw = matrix.Color888(16 , 16 , 0  );
  uint16_t medylw = matrix.Color888(65 , 65 , 0  );
  uint16_t medcyn = matrix.Color888(0  , 65 , 65 );

  //Number of the right of the colon is the default "0" state
  printTightDefault((orvcu.bit.CVT ? "24V" : "0 V"), 16, 8, (orvcu.bit.CVT ? dimgrn : dimred));
  printTightDefault("MOS", 16, 16, (orvcu.bit.MOS ? (orvcu.bit.MOR ? bold : medylw) : dimblu));
  printTightDefault("R2E", 0 , 24, (orvcu.bit.R2E ? bold   : dimblu));
  printTightDefault("ESP", 0 , 8 , (orvcu.bit.ESP ? dimgrn : brtred));
  printTightDefault("DRO", 16, 0 , (orvcu.bit.DRO ? brtred : dimgrn));
  printTightDefault("SBT", 0 , 16, (orvcu.bit.SBT ? brtred : dimgrn));
  printTightDefault("MBK", 48, 0 , (orvcu.bit.MBK ? bold   : dimblu));
  printTightDefault("R2R", 32, 0 , (orvcu.bit.R2R ? dimgrn : dimred));
  printTightDefault("SRO", 32, 8 , (orvcu.bit.SRO ? dimgrn : dimred));
  printTightDefault("CMT", 48, 16, (orvcu.bit.CMT ? brtred : dimgrn));
  printTightDefault("CMH", 48, 8 , (orvcu.bit.CMH ? dimgrn : dimred));
  printTightDefault("RES", 16, 24, (orvcu.bit.R2E ? medmgt : dimblu));

  // Draws particular attention if the machine is told to be in motion
  switch (orvcu.bit.IMN) {
    case 0 :
      printTightDefault("IMN", 32, 16, dimred);
      break;
    case 1:
      if ( (millis() % BLINKRATE) <= BLINKRATE / 2) {
        printTightDefault("IMN", 32, 16, dimylw);
      } else {
        printTightDefault("IMN", 32, 16, medylw);
      } break;
  }

  //displays the TCV number
  String temp_TCV = String(orvcu.bit.TCV);
  printTightDefault(temp_TCV.c_str(), 43, 24, dimblu);

  String temp_TCR = String(orvcu.bit.TCR);
  printTightDefault("T", 32, 24, medcyn);
  printTightDefault(temp_TCR.c_str(), 37, 24, medcyn);

  //Matches the blinking behavior of the StopLight to be on the display
  switch (orvcu.bit.SLT) {
    case SLT_Off :
      printTightDefault("SLT", 0, 0, dimgry);
      break;
    case SLT_Green:
      printTightDefault("SLT", 0, 0, dimgrn);
      break;
    case SLT_Yellow:
      printTightDefault("SLT", 0, 0, medylw);
      break;
    case SLT_Red:
      printTightDefault("SLT", 0, 0, brtred);
      break;
    case SLT_BlinkRed:
      if ( (millis() % BLINKRATE) <= BLINKRATE / 2) {
        printTightDefault("SLT", 0, 0, brtred);
      } else {
        printTightDefault("SLT", 0, 0, dimred);
      }
      break;
    case SLT_BlinkYlw:
      if ( (millis() % BLINKRATE) <= BLINKRATE / 2) {
        printTightDefault("SLT", 0, 0, medylw);
      } else {
        printTightDefault("SLT", 0, 0, dimylw);
      }
      break;
    case SLT_BlinkGrn:
      if ( (millis() % BLINKRATE) <= BLINKRATE / 2) {
        printTightDefault("SLT", 0, 0, dimgrn);
      } else {
        printTightDefault("SLT", 0, 0, dimgrn);

      }
      break;
    case SLT_Rotating:
      if ( (millis() % BLINKRATE) <= 333) {
        printTightDefault("SLT", 0, 0, brtred);
      } else if ( (millis() % BLINKRATE) > 333
                  && (millis() % BLINKRATE) <= 666) {
        printTightDefault("SLT", 0, 0, medylw);
      } else {
        printTightDefault("SLT", 0, 0, dimgrn);
      }
      break;
  }
}

void printTightDefault(const char *str, int Xstart, int Yline, uint16_t color)
{
  matrix.setFont(0);
  matrix.setTextColor(color);
  for (int i = 0; str[i]; i++) {
    matrix.setCursor(Xstart + (i * 5), Yline);
    matrix.print(str[i]);
  }
}
