#ifndef STATUS_PACKET_H
#define STATUS_PACKET_H

#include "CtrlState.h"

struct DiagnosticHeader {
  ORVCtrlState bit;
};

struct TextHeader {
  String displayText;
  int displayColor;
  int displayLine;
};

union HeaderUnion {
  DiagnosticHeader d;
  TextHeader t;
};

#endif
