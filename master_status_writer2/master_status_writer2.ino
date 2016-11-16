#include <Wire.h>
#include "CtrlState.h"
#include "MarqueeState.h" //TODO
#include "ProgressState.h" //TODO
#include <SPI.h>
#include <Ethernet.h>

#define BUF_SIZ 512
#define RECEP_ERROR -1

#define TCGpin  A8    // Turn-Counter Ground (Analog)
#define TCRpin  A9    // Turn-Counter Sensor (Analog)
#define TCVpin  A10   // Turn-Counter Power (Analog)
#define PWRpin  A14   // Power Indicator (Analog)
#define PWGpin  A15   // Chassis Power Indicator Ground (Analog)
#define SLRpin   5   // Stoplight RED
#define SLYpin   6   // Stoplight YELLOW
#define SLGpin   7   // Stoplight Green
#define MOSpin  22   // Manual Override Switch On Sense
#define WBGpin  23   // WallBox Ground    <-- This may be removed so the wire can provide 5V to relay -->
#define CVTpin  24   // Control-Cabinet 24 Volts On Sense
#define MORpin  25   // Manual Override Enable Relay Signal
#define ESPpin  26   // E-Stop / Actually "IndraDrive Ready" Sense
#define MOBpin  27   // Manual Override Enabled LED
#define DROpin  28   // Door Open Sense
#define MOApin  29   // Manual Override Ready LED
#define SBTpin  32   // Seatbelt Secure (Black)
#define SBGpin  33   // SeatBelt Ground (White)
#define MBKpin  34   // Mechanical Brake Engaged (Yellow)
#define MBGpin  35   // Mech Brake Ground (Red)
#define CMTpin  40   // Commutation Sensor On
#define CMVpin  41   // Commutation Power
#define CMHpin  42   // Commutation Sensor Home
#define RQSpin  47   // Request Status LED
#define R2Epin  48   // Enter Requested Sense
#define REGpin  49   // Request-To-Enter Ground  <-- This may be removed as there is already a ground available -->

//<----------- from host----------->
int R2R = 0 ; // 0: not ready,     1: ready [Ready Run]
int IMN = 0 ; // 0: not in motion, 1: in motion [In Motion]
String displayString = String("");
int displayColor = 0 ;
int displayLine = 0 ;

// <------------from the control system ------------>
int SBT = 1; // 0: buckled,          1: unbuckled [Seat Belt]
int CTV = 1; // 0: off,         1: on [24 volts]
int DOR = 0; // 0: open,        1:closed [door open]
int CPH = 0; // 0: not at home, 1: at home [Commutation Position]
int R2E = 0; // 0: no request,  1: request [Request to Enter]
int ESP = 1; // 0: on,          1: off [E-stop]
int SRO = 0; // 0: off,         1: on [Power-on]

//<-----------StopLight States----------->
#define SLT_Off         0
#define SLT_Red         1
#define SLT_Yellow      2
#define SLT_Green       4
#define SLT_BlinkRed  (SLT_Red | SLT_Green)
#define SLT_BlinkYlw  (SLT_Yellow | SLT_Red)
#define SLT_BlinkGrn  (SLT_Green | SLT_Yellow)
#define SLT_Rotating  (SLT_Red | SLT_Green | SLT_Yellow)

// <-------------Header Set -------------------------->
/*  CS = CtrlState          Nothing to display, just update and return the data to the host
 *  PD = ProgressDisplay    Changes display to progress mode
 *  MX = Marquee LineX      Update lineX text (x = (1,2,3,4))
 *  MB = Bitmap             Update bitmap
 *  MC = Clear              Clear all on screen data
 *  MD = MarqueeDisplay     Changes display to marquee mode
 *  DD = DefaultDisplay     Changes display to default mode
 */

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 86); // Ethernet
//IPAddress myDns(192, 168, 1, 1);
//IPAddress gateway(192, 168, 1, 1);
//IPAddress subnet(255, 255, 0, 0);

EthernetServer server(23); // Ethernet
boolean alreadyConnected = false;

String inputStream; // input string buffer for client commands

void setup() {
  Ethernet.begin(mac, ip); //, myDns, gateway, subnet); // Ethernet

  server.begin();
 

 // join i2c bus (address optional for master)
  initializePins(); // set pinModes for pins used on Arduino board
  Serial.begin(9600);

  Serial.print("Server address is: "); // Ethernet
  Serial.println(Ethernet.localIP());
}

//First two bytes are the header bytes
//char hdr[2]; //TODO
char send_buffer[250];//TODO

void loop() {
  static ORVCtrlUnion orvcu = {0,0};
  static ProgressState prog;
  static MarqueeState marq;
  static int hdr = 0;//TODO

  //collects data from all the sensors
  read_inputs(orvcu);

  // wait for a new client:
  EthernetClient client = server.available();

  manual_override_check(orvcu);
  request_enter_check(orvcu);

  //collects data to determine the state of the stoplight
  orvcu.bit.SLT = RYG_SET(orvcu);
  //lights the stoplight according to the state
  led_control(orvcu);
  // when the client sends the first byte, say hello:
  if (client) {

    if (!alreadyConnected) {
      // clear out the input buffer:
      client.flush();
      Serial.println("New client is online");
      //.println("Please type a command and press Enter to finish");
      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      if (thisChar != '\n') {
        if (thisChar == 11) {//TODO
          hdr = String("DD").toInt();
        } else if (thisChar == 32 || (thisChar >= 65 && thisChar <= 126)) {
          inputStream.concat(thisChar);
        } else {
          if (inputStream.equalsIgnoreCase("Change host fields")) {
            orvcu = changeStatus(thisChar, client, orvcu);
          } else if (inputStream.equalsIgnoreCase("Set time")) {
            prog.seconds_remaining = setTime(thisChar, client);
          } else if (inputStream.equalsIgnoreCase("Set current test number")) {
            prog.current_test = setTest(thisChar, client);
          } else if (inputStream.equalsIgnoreCase("Set total test number")) {//TODO
            prog.total_test = setTest(thisChar, client);
          } else if (inputStream.equalsIgnoreCase("Set override status") || inputStream.startsWith("Override") || inputStream.startsWith("override")) {
            orvcu.bit.MOR = manOver(thisChar, client);
          }
          inputStream = "";
        }
      }
    }
  }

  
  //TODO Need to decide what to send
  if(1==1){//TODO
    memcpy(send_buffer, &orvcu, sizeof(orvcu));
  } else if (hdr == String("PD").toInt()) {
    memcpy(send_buffer, &prog , sizeof(prog ));
  } else if (hdr == String("MD").toInt()) {
    memcpy(send_buffer, &marq , sizeof(marq ));
  }

    Serial.print(send_buffer[2]);
  Wire.beginTransmission(13); // transmit to device #13
    //Wire.write(hdr);
    for (int i = 0; i < sizeof(send_buffer); i++){
      Wire.write(send_buffer);
    }
  Wire.endTransmission();    // stop transmitting
  delay(10);
}

//<------------------------------------------------------------>

void initializePins() {

  // chassis power indicator
  pinMode(PWGpin, OUTPUT);
  pinMode(PWRpin, OUTPUT);
  digitalWrite(PWGpin, LOW);
  digitalWrite(PWRpin, HIGH);

  // turn-counter
  pinMode(TCGpin, OUTPUT);
  pinMode(TCRpin, INPUT);
  pinMode(TCVpin, OUTPUT);
  digitalWrite(TCGpin, LOW); // Ground
  digitalWrite(TCVpin, HIGH); // Reference Voltage

  // output pins for StopLight
  pinMode(SLRpin, OUTPUT);
  pinMode(SLGpin, OUTPUT);
  pinMode(SLYpin, OUTPUT);
  digitalWrite(SLRpin, LOW); // relay off
  digitalWrite(SLYpin, LOW); // relay off
  digitalWrite(SLGpin, LOW); // relay off

  // request to enter
  pinMode(R2Epin, INPUT_PULLUP);
  pinMode(REGpin, OUTPUT);
  pinMode(RQSpin, OUTPUT);
  digitalWrite(REGpin, LOW); // Ground
  digitalWrite(RQSpin, LOW); // LED off

  // control-cab, wallbox
  pinMode(CVTpin, INPUT_PULLUP);
  pinMode(ESPpin, INPUT_PULLUP);
  pinMode(MOSpin, INPUT_PULLUP);
  pinMode(DROpin, INPUT_PULLUP);
  pinMode(WBGpin, OUTPUT);
  pinMode(MORpin, OUTPUT);
  pinMode(MOApin, OUTPUT);
  pinMode(MOBpin, OUTPUT);
  digitalWrite(WBGpin, LOW); // Ground
  digitalWrite(MORpin, LOW); // Manual Override off


  digitalWrite(MOBpin, LOW); // LED off

  // seatbelt and brake
  pinMode(MBKpin, INPUT_PULLUP);
  pinMode(SBTpin, INPUT_PULLUP);
  pinMode(MBGpin, OUTPUT);
  pinMode(SBGpin, OUTPUT);
  digitalWrite(MBGpin, LOW); // Ground
  digitalWrite(SBGpin, LOW); // Ground

  // commutation
  pinMode(CMTpin, INPUT);
  pinMode(CMHpin, INPUT);
  pinMode(CMVpin, OUTPUT);
  digitalWrite(CMVpin, LOW); // Power Off

  return;
}

void read_inputs(ORVCtrlUnion orvcu)
{
  // R2E, CVT, ESP, DRS, MOR, SBT, and MBK are normally pulled-up
  // When switched close, they are pulled LOW.
  // Read Req2Enter
  orvcu.bit.R2E = (digitalRead(R2Epin) == HIGH) ? 0 : 1;
  // Read CtrlCab 24V and Ready
  orvcu.bit.CVT = (digitalRead(CVTpin) == HIGH) ? 0 : 1;
  //CtrlCabReady
  orvcu.bit.ESP = (digitalRead(ESPpin) == HIGH) ? 0 : 1;
  // Read Door Secure
  orvcu.bit.DRO = (digitalRead(DROpin) == HIGH) ? 0 : 1;
  // Read Override Switch
  Serial.println(orvcu.bit.DRO);
  orvcu.bit.MOS = (digitalRead(MOSpin) == HIGH) ? 0 : 1;
  // Read SeatBelt & Brake
  orvcu.bit.SBT = (digitalRead(SBTpin) == HIGH) ? 0 : 1;
  orvcu.bit.MBK = (digitalRead(MBKpin) == HIGH) ? 0 : 1;

  // Read Turn-Counter
  //TURN_COUNTER_INPUT();
}

int  RYG_SET(ORVCtrlUnion orvcu) {
  int stoplightstate = SLT_Off;
  if (orvcu.bit.CVT == 1 // when 24 volts on, ready to run, not moving,  e-stop off, power off
      && orvcu.bit.R2R == 1
      && orvcu.bit.IMN == 0
      && orvcu.bit.ESP == 1
      && orvcu.bit.SRO == 0) {
    stoplightstate = SLT_Yellow;
  }
  else {
    if (orvcu.bit.IMN == 1 // machine moving  when 24 volts on and e-stop off
        && orvcu.bit.CVT == 1
        && orvcu.bit.ESP == 1) {
      stoplightstate = SLT_BlinkRed;
    }
    else if (orvcu.bit.SRO == 1 // power on when ready
             && orvcu.bit.CVT == 1
             && orvcu.bit.ESP == 1) {
      stoplightstate = SLT_Red;
    }
    else if (orvcu.bit.CVT == 0 || orvcu.bit.ESP == 0) { // 24 volts off or E-stop on
      stoplightstate = SLT_Green;
    }
    else if (orvcu.bit.CVT == 1 // 24 volts on but not ready to run and power is off
             && orvcu.bit.R2R == 0
             && orvcu.bit.SRO == 0
             && orvcu.bit.IMN == 0) {
      stoplightstate = SLT_Green;
    }
  }
  return stoplightstate;
}

void led_control(ORVCtrlUnion orvcu) {
  static unsigned int old_state = 0;
  if (old_state != orvcu.bit.SLT) {
    digitalWrite(SLRpin, (orvcu.bit.SLT & SLT_Red) ? HIGH : LOW);
    digitalWrite(SLYpin, (orvcu.bit.SLT & SLT_Yellow) ? HIGH : LOW);
    digitalWrite(SLGpin, (orvcu.bit.SLT & SLT_Green) ? HIGH : LOW);
    old_state = orvcu.bit.SLT;
  }
}

void manual_override_check(ORVCtrlUnion orvcu) {
  digitalWrite(MOApin, (orvcu.bit.MOR ? HIGH : LOW));
  if (orvcu.bit.MOR == 1) {
    digitalWrite(MORpin, HIGH);
    digitalWrite(MOBpin, (orvcu.bit.MOS ? HIGH : LOW));
  }
}

void request_enter_check(ORVCtrlUnion orvcu) {  
  digitalWrite(R2Epin, (orvcu.bit.R2E ? HIGH : LOW));
  if (orvcu.bit.R2E == 0) { //might need to be flipped depending on default state TODO
    digitalWrite(RQSpin, HIGH); // LED might be broken TODO
    send_buffer[0] = 'D';
    send_buffer[1] = 'H';
  }
}

ORVCtrlUnion changeStatus(char input, EthernetClient client, ORVCtrlUnion orvcu) {
  client.read();
  client.println("R2R?");
  while (input != '\n') {
    char digit = client.read();
    if (digit == 48 || digit == 49) {
      Serial  .println(digit);
      orvcu.bit.R2R = digit - 48;
      break;
    }
  }
  client.println("SRO?");
  while (input != '\n') {
    char digit = client.read();
    if (digit == 48 || digit == 49) {
      Serial.println(digit);
      orvcu.bit.SRO = digit - 48;
      break;
    }
  }
  client.println("IMN?");
  while (input != '\n') {
    char digit = client.read();
    if (digit == 48 || digit == 49) {
      Serial.println(digit);
      orvcu.bit.IMN = digit - 48;
      break;
    }
  }
  return orvcu;
}

int setTest(char input, EthernetClient client) {
  client.read();
  client.println("What is the current test number?");
  String testNumString;
  while (input != '\n') {
    char digit = client.read();
    if (digit >= 48 && digit <= 57) {
      testNumString.concat(digit);
    } else if (digit == '\n') {
      break;
    }
  }
  int testNum = testNumString.toInt();
  return testNum;
}

int setTime(char input, EthernetClient client) {
  client.read();
  client.println("Enter the number of seconds to set timer to:");
  String timeString;
  while (input != '\n') {
    char digit = client.read();
    if (digit >= 48 && digit <= 57) {
      timeString.concat(digit);
    } else if (digit == '\n') {
      break;
    }
  }
  int timeAmt = timeString.toInt();
  return timeAmt;
}

int manOver(char input, EthernetClient client) {
  int manOverStatus;
  client.read() ;
  client.println("Enter status of manual override:");
  while (input != '\n') {
    char digit = client.read();
    if (digit == 48 || digit == 49) {
      Serial.println(digit);
      manOverStatus = digit - 48;
      break;
    }
  }
  return manOverStatus;
}
