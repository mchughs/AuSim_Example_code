#ifndef MARQUEE_BITMAP_H
#define MARQUEE_BITMAP_H

struct MarqueeBitmap
{
  enum {WIDTH = 16, HEIGHT = 32};

  char x, y;
  char szx, szy;
  short color1;
  short color2;
  short color3;
  char bitmap[WIDTH*HEIGHT]; //max possible

  MarqueeBitmap()
  : x(0)
  , y(0)
  , szx(0) //may just require that the dimensions match the size of the bitmap given
  , szy(0)
  , color1(0)
  , color2(0)
  , color3(0)
  {
   for (int i = 0; i < WIDTH * HEIGHT; i++){
       bitmap[i] = 0;
     }
  }
  void reset() {
    x,y,szx,szy,color1,color2,color3 = 0;
    for (int i = 0; i < WIDTH * HEIGHT; i++){
       bitmap[i] = 0;
     }
   }
};

#endif // ndef MARQUEE_BITMAP_H
