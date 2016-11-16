#ifndef ORV_CTRL_STATE_H
#define ORV_CTRL_STATE_H

// Goal: 4-bytes, 32-bits
//    6-bits
//    2-bits reserved
//    6-bits R2E and SLT
//   10-bits TCV
//    3-bits TCR
//    4-bits HOST
struct ORVCtrlState
{
  // single-bit hardware fields
  unsigned char CVT:1;  // Control-Cabinet 24V Power, 1=on (white wire in CtrlCab)
  unsigned char ESP:1;  // Hardware ready | E-Stop, 1=ready to run (yellow wire in CtrlCab)
  unsigned char DRO:1;  // Door open, 0=closed, 1=open
  unsigned char SBT:1;  // Seat Belt, 1=latched
  unsigned char MBK:1;  // Mechanical Brake, 1=engaged
  unsigned char R2E:1;  // Request-To-Enter: 1=pressed
  unsigned char MOS:1;  // Manual Override Switch, 1=on
  unsigned char CMH:1;  // Commutate Position: 1=at-home

  // HOST fields
  unsigned char SLT:3;  // StopLight: Red, Yellow, Green
  unsigned char R2R:1;  // HOST:Software ready: 1=ready to run
  unsigned char SRO:1;  // HOST:Servo On: 1=on
  unsigned char IMN:1;  // HOST:In Motion: 1=moving
  unsigned char MOR:1;  // HOST:ManualOverride: 1=enabled
  unsigned char CMT:1;  // HOST:Commutate

  // multi-bit fields
  unsigned short TCV:10;  // Turn-Counter Value 0-1023
  unsigned short TCR:2;  // Turn-Counter: 0=unknown, 1=low-turn, 2=mid-turn, 3=max-turn
  unsigned short RES:2;  // Request-To-Enter Status: bit0=requested, bit1=diagnostics
};

union ORVCtrlUnion
{
  ORVCtrlState    bit;
  unsigned long  lval;
  unsigned short sval;
  unsigned char  bytes[4];
};

#endif  // ndef ORV_CTRL_STATE_H
