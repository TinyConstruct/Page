#ifndef __BITMAP__
#define __BITMAP__
#include <stdint.h>

struct Bitmap {
  int32_t width, height;
  void* contents;
};

//This should only be taken as me wanting to know how things work, it's not 
//really a good/safe implementation by any stretch of imagination
//Force byte alignment so header offsets stay intact
#pragma pack(push,1)
struct BitmapHeader{
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
  uint32_t biSize;
  int32_t biWidth;
  int32_t  biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
};
#pragma pack(pop)
#endif