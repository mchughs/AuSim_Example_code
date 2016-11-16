#ifndef MARQUEE_STATE_H
#define MARQUEE_STATE_H

struct MarqueeLine //Total = 64 bytes
{
  enum {MAXCHAR = 58};

  char x, y;
  char font;
  uint16_t color;
  char text[MAXCHAR];
  
  MarqueeLine()
     : x(0)
     , y(0)
     , font(0)
     , color(0)
     {
       for (int i = 0; i < MAXCHAR; i++){
         { text[i] = 0; }
       }
     }
};

struct MarqueeBitmap
{
  enum {WIDTH = 64, HEIGHT = 32};
  
  bool clearMap;
  char x, y;
  char szx, szy;
  char bitmap[WIDTH][HEIGHT];
  
  MarqueeBitmap()
  : x(0)
  , y(0)
  {
    for (int i = 0; i < WIDTH; i++){
      for (int j = 0; j < HEIGHT; j++){
        { bitmap[i][j] = 0; }
      }
    }
  }
};

struct MarqueeState
{
  MarqueeLine line1, line2, line3, line4;
  //TODO MarqueeBitmap bitmap;
  
  MarqueeState() 
  : line1()
  , line2()
  , line3()
  , line4()
  //TODO , bitmap()
  {}
};

#endif // ndef MARQUEE_STATE_H
