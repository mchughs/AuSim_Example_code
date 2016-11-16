#ifndef MARQUEE_LINE_H
#define MARQUEE_LINE_H

struct MarqueeLine //Total = 64 bytes
{
  enum {MAXTEXT = 58};

  char x, y;
  unsigned char font;
  unsigned short color;
  char text[MAXTEXT];

  MarqueeLine()
     : x(0)
     , y(0)
     , font(0)
     , color(0)
     {
       for (int i = 0; i < MAXTEXT; i++){
         { text[i] = 0; }
       }
     }
};

#endif // ndef MARQUEE_LINE_H
