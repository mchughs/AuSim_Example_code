#ifndef MARQUEE_BITMAP_H
#define MARQUEE_BITMAP_H

struct MarqueeBitmap
{
  enum {WIDTH = 16, HEIGHT = 16};

  bool clearMap;
  char x, y;
  char szx, szy;
  short color;
  char bitmap[WIDTH*HEIGHT]; //max possible

  MarqueeBitmap()
  : x(0)
  , y(0)
  , szx(0) //may just require that the dimensions match the size of the bitmap given
  , szy(0)
  , color(0)
  {
   for (int i = 0; i < WIDTH*HEIGHT; i++){\
       bitmap[i] = 0;
     }
  }
};

#endif // ndef MARQUEE_BITMAP_H
