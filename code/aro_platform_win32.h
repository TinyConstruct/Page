#ifndef __ARO_PLATFORM__
#define __ARO_PLATFORM__
#include "bitmap.h"

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

FileReadResult readWholeFile(char* fileName);
Bitmap loadBMP(char* fileName);
bool freeBMP(Bitmap* bitmap);

#endif // __ARO_PLATFORM__