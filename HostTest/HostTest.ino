#include "CtrlState.h"
#include "ProgressState.h"
#include "MarqueeLine.h"
#include "MarqueeBitmap.h"
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>

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

#define BUFF_SIZE 256
#define VERSION   0.104

ORVCtrlUnion orvcu = {0, 0};
ProgressState prog;
MarqueeLine mline[4];
MarqueeBitmap mbmp;

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(192, 168, 0, 86); // Ethernet
#define IPPORT  1632    // as set in ORVuCtrlHost
EthernetServer server(IPPORT); // Ethernet

void setup() {  
  Ethernet.begin(mac, ip); // Ethernet
  initializePins();
  server.begin(); 
  Wire.begin();

#ifdef _DEBUG
  Serial.begin(9600);
  Serial.print("Server address is: "); // Ethernet
  Serial.print(Ethernet.localIP());
  Serial.print(" on port ");
  Serial.println(IPPORT);
#endif
}

void loop() {
#ifdef _DEBUG
  static boolean alreadyConnected = false;
#endif
  // inputs
  read_R2E();
  read_inputs();


  // wait for a new client:
  EthernetClient client = server.available();

  if (client) {
#ifdef _DEBUG
    if (!alreadyConnected) {
      // when host connects for the first time,
      Serial.println("New client is online");
      alreadyConnected = true;
    }
#endif
    char tcpip_recv_buffer[BUFF_SIZE];

    int bytecount = 0;
    while (client.available() > 0) {
      // read the bytes incoming from the host:
      tcpip_recv_buffer[bytecount++] = client.read();
    }

    if (bytecount) {
      if ( (tcpip_recv_buffer[0] == 'G') && (tcpip_recv_buffer[1] == 'S') )
        respond_to_status_request(client);
      else if ( (tcpip_recv_buffer[0] == 'X') && (tcpip_recv_buffer[1] == '$') )
        respond_to_exchange_state_request(client, *(unsigned long *)&tcpip_recv_buffer[2]);
      else if ( (tcpip_recv_buffer[0] == 'P') && (tcpip_recv_buffer[1] == 'D') ) 
        respond_to_set_progress_request(client, &tcpip_recv_buffer[2]); 
      else if ( (tcpip_recv_buffer[0] == 'M') && (tcpip_recv_buffer[1] == 'L') ) 
        respond_to_set_marquee_line(client, &tcpip_recv_buffer[2]); 
      else if ( (tcpip_recv_buffer[0] == 'B') && (tcpip_recv_buffer[1] == 'M') ) 
        respond_to_set_marquee_bitmap(client, &tcpip_recv_buffer[2]); 
      else if (  (tcpip_recv_buffer[0] == 'M') && (tcpip_recv_buffer[1] == 'S') ) 
        { Wire.beginTransmission(13);
          Wire.write(tcpip_recv_buffer,3);
          Wire.endTransmission();
          client.write("ms", 2);}
      else
        client.write('K');

      for (int i = 0; i < bytecount; i++)
        Serial.println(tcpip_recv_buffer[i], HEX);
    }
  }
  
  // outputs
  orvcu.bit.SLT = RYG_SET();
  if (orvcu.bit.SLT == SLT_Green) // clear orvcu.bit.RES bit0 Request 2 Enter
  led_control();
  // if diag bit of orvcu.bit.RES is high then wire CS to ProgressDisplay for diagnostics
  // manual override, 
  //   if not MOS then clear host MOR
  //   if MOS then light MOA LED
  //   if host MOR then set pinMOR high
  //   if MOR then light MOB LED
  // if R2E then light RQS LED pin

  // two ways to read a bit
  if ((orvcu.bit.RES>>1)&1)
  then this bit is set
  if ((orvcu.bit.RES)&(1<<1))
  then this bit is set
  if ((orvcu.bit.RES)&(1<<1))
  then this bit is set
  if (bitRead(orvcu.bit.RES,1)
  then this bit is set
  
  
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
  digitalWrite(MOApin, LOW); // LED off
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

}

void read_inputs()
{
  // R2E, CVT, ESP, DRS, MOR, SBT, and MBK are normally pulled-up
  // When switched close, they are pulled LOW.
  orvcu.bit.R2E = (digitalRead(R2Epin) == HIGH) ? 0 : 1;
  // Read CtrlCab 24V and Ready
  orvcu.bit.CVT = (digitalRead(CVTpin) == HIGH) ? 0 : 1;
  //CtrlCabReady
  orvcu.bit.ESP = (digitalRead(ESPpin) == HIGH) ? 0 : 1;
  // Read Door Secure
  orvcu.bit.DRO = (digitalRead(DROpin) == HIGH) ? 0 : 1;
  // Read Override Switch
  orvcu.bit.MOS = (digitalRead(MOSpin) == HIGH) ? 0 : 1;
  // Read SeatBelt & Brake
  orvcu.bit.SBT = (digitalRead(SBTpin) == HIGH) ? 0 : 1;
  orvcu.bit.MBK = (digitalRead(MBKpin) == HIGH) ? 0 : 1;
  
  // Read Turn-Counter
  orvcu.bit.TCV = analogRead(TCRpin);
  if(orvcu.bit.TCV < 256 ){
    orvcu.bit.TCR = 0;
  } else if (orvcu.bit.TCV >= 256 && orvcu.bit.TCV < 512){
    orvcu.bit.TCR = 1;
  } else if (orvcu.bit.TCV >= 512 && orvcu.bit.TCV < 768){
    orvcu.bit.TCR = 2;
  } else {
    orvcu.bit.TCR = 3;
  }
  return;
}

int  RYG_SET() {
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

void led_control() {
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

void respond_to_status_request(EthernetClient client)
{
  Serial.println("Responding to GS request");
  String ver("gs\nORVuCtrl v");
  ver += String(VERSION, 3);
  client.write((char*)ver.c_str(), ver.length());
}

void respond_to_exchange_state_request(EthernetClient client, unsigned long state)
{
  Serial.print("Responding to X$ request ");
  Serial.println(state, HEX);
  ORVCtrlUnion cu;
  cu.lval = state;

  //sets host-controlled parameters to send
  orvcu.bit.R2R = cu.bit.R2R;
  orvcu.bit.SRO = cu.bit.SRO;
  orvcu.bit.IMN = cu.bit.IMN;
  orvcu.bit.MOR = cu.bit.MOR;
  orvcu.bit.CMT = cu.bit.CMT;
  Serial.println("");
  
  char buf[7] = "x#";
  buf[2] = orvcu.bytes[0];
  buf[3] = orvcu.bytes[1];
  buf[4] = orvcu.bytes[2];
  buf[5] = orvcu.bytes[3];
  buf[6] = orvcu.bytes[0] + orvcu.bytes[1] + orvcu.bytes[2] + orvcu.bytes[3]; //Checksum
  client.write(buf, 7);
}


void respond_to_set_progress_request(EthernetClient client, char state[]) 
{
  Serial.println("Responding to PD request ");
  char buf[3] = "PD";
  client.write("pd",2);
  Wire.beginTransmission(13);
  Wire.write(buf,2);
  Wire.write(state,sizeof(ProgressState));
  Wire.endTransmission();
}

void respond_to_set_marquee_line(EthernetClient client, char state[]) 
{
  Serial.println("Responding to ML request ");
  char buf[3] = "ML"; 
  client.write("ml", 2);
  for(int n = 0; n < 3; n++){
    Wire.beginTransmission(13);
    Wire.write(buf,2);
    Wire.write(&state[n*30],30);
    Wire.endTransmission();
    delay(120);
  }
}

void respond_to_set_marquee_bitmap(EthernetClient client, char state[]) 
{
  Serial.println("Responding to BM request ");
  char buf[3] = "BM";
  client.write("bm", 2);
    for(int m = 0; m < 20; m++){ //up-to 512 bytes + all the headers
      Wire.beginTransmission(13);
      Wire.write(buf,2);
      Wire.write(&state[m*30],30);
      Wire.endTransmission();
      delay(100);
  }
}

void read_R2E(){
  static unsigned int cur_press = 0 ;
  static bool reset = false ;
  static bool prev = false;

  static unsigned long last_state_change_msecs = 0;

  bool cur = (digitalRead(R2Epin) == HIGH);

  if (cur != orvcu.bit.R2E) {
    orvcu.bit.R2E = cur;
    unsigned long curtime = millis()
    if (!cur)  { /// change from high to low, release of button
      if ((curtime - last_state_change_msecs) < 2000)
        // request to enter
        orvcu.bit.RES // bit0
        else
        orvcu.bit.RES // bit1
        // set diagnostic display
    }
    last_state_change_msecs = curtime;
  }
  
  orvcu.bit.R2E = (digitalRead(R2Epin) == HIGH) ? 0 : 1;
  if (orvcu.bit.R2E == HIGH && prev == false){
    cur_press ++;
    prev = true;
  } else if (orvcu.bit.R2E == LOW && prev == true){
    prev = false;
  }
  if ((cur_press % 2) == 1){
    reset = true;
    Serial.println("Responding to DH request ");
    char buf[3] = "DH";
    Wire.beginTransmission(13);
    Wire.write(buf,2);
    Wire.write(orvcu.bytes[0]);
    Wire.write(orvcu.bytes[1]);    
    Wire.write(orvcu.bytes[2]);
    Wire.write(orvcu.bytes[3]);
    Wire.endTransmission();
  } else if ( ((cur_press % 2) == 0) && (reset == true) ){
    reset = false;
    char buf[3] = "RS";
    Wire.beginTransmission(13);
    Wire.write(buf,2);
    Wire.endTransmission();
  }
}

