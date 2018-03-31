#ifndef __ARO_PLATFORM__
#define __ARO_PLATFORM__

#include <stdint.h>

struct Win32WindowDimensions
{
    int width;
    int height;
};

struct Win32WindowLocation {
  int upperLeftX, upperLeftY;
  int centerX, centerY;
};

void* getWin32GLFunc(const char *name);
Win32WindowDimensions win32GetWindowDimension(HWND window);

struct FileReadResult {
  unsigned int contentsSize;
  void *contents;
};

struct Bitmap {
  int32_t width, height;
  void* contents;
};

//This should only be taken as me wanting to know how things work, it's not 
//really a good/safe implementation by any stretch of imagination
//Force byte alignment so header offests stay intact
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

FileReadResult readWholeFile(char* fileName);
Bitmap loadBMP(char* fileName);
bool freeBMP(Bitmap* bitmap);

#endif // __ARO_PLATFORM__