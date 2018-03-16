#ifndef __ARO_PLATFORM__
#define __ARO_PLATFORM__

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

#endif // __ARO_PLATFORM__