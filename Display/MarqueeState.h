#ifndef MARQUEE_STATE_H
#define MARQUEE_STATE_H

struct MarqueeLine //Total = 64 bytes
{
  enum {MAXTEXT = 58};

  char x, y;
  unsigned short font;
  uint16_t color;
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

struct Bitmap
{
  enum {WIDTH = 64, HEIGHT = 32};
  
  bool clearMap;
  char x, y;
  char szx, szy;
  short color;
  char bitmap[WIDTH*HEIGHT]; //max
  //char bitmap[(szx*szy)/8]; //actual
  
  Bitmap()
  : x(0)
  , y(0)
  , szx(0) //may just require that the dimensions match the size of the bitmap given
  , szy(0)
  , color(0)
  {
//    for (int i = 0; i < WIDTH; i++){
//      for (int j = 0; j < HEIGHT; j++){
//        { bitmap[i][j] = 0; }
//      }
//    }
      for (int i = 0; i < ((szx*szy)/8); i++){
           bitmap[i] = 0;
        }
  }
};

struct MarqueeState
{
  MarqueeLine line1, line2, line3, line4;
  Bitmap bitmap;
  
  MarqueeState() 
  : line1()
  , line2()
  , line3()
  , line4()
  , bitmap()
  {}
};

#endif // ndef MARQUEE_STATE_H
