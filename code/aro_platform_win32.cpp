#include <Windows.h>
#include <stdint.h>
#include "aro_generic.h"
#include "aro_platform_win32.h"

FileReadResult readWholeFile(char* fileName) {
  FileReadResult result = {};
  HANDLE fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(fileHandle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER size;
    if(GetFileSizeEx(fileHandle, &size)) {
      uint32_t size32 = safeTruncateUInt64(size.QuadPart);
      result.contents = VirtualAlloc(0, size32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if(result.contents) {
        DWORD bytesRead;
        if(ReadFile(fileHandle, result.contents, size32, &bytesRead, 0) && (size32== bytesRead)) {
          result.contentsSize = size32;
        }
      }
      else {
        VirtualFree(result.contents, 0, MEM_RELEASE);
      }
    }
    else {
      InvalidCodePath;
    }
    CloseHandle(fileHandle);
  }
  else {
    InvalidCodePath;
  }
  return result;
}

Bitmap loadBMP(char* fileName) {
  FileReadResult readResult = readWholeFile(fileName);
  Bitmap result = {};
  if(readResult.contentsSize != 0){
    BitmapHeader* header = (BitmapHeader*) readResult.contents; 
    result.width = header->biWidth;
    result.height = header->biHeight;
    result.contents = (uint32_t*)((uint8_t*) readResult.contents + header->bfOffBits);
    //our BMPs from GIMP are loaded in memory order of RRGGBBAA, little endian stores in AABBGGRR
    //But we want GL_BGRA_EXT, so AARRGGBB in LE memory, so shuffle them 
    uint32_t* pixel = (uint32_t*) result.contents;
    uint32_t alphaMask = 0x000000FF;
    uint32_t alpha;
    for(int i = 0; i < header->biHeight; i++) {
      for(int j = 0; j < header->biWidth; j++) {
        alpha = (*pixel & alphaMask)<<24;
        if (alpha != 0) {
          int i = 0;
        }
        *pixel = ((*pixel)>>8) | alpha;
        pixel++;
      }
    }
  }
  return result;
}

bool freeBMP(Bitmap* bitmap) {
  return VirtualFree(bitmap->contents, 0, MEM_RELEASE);
}

Win32WindowDimensions win32GetWindowDimension(HWND window) {
  Win32WindowDimensions result;
  RECT clientRect;
  GetClientRect(window, &clientRect);
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;
  return(result);
}

void* getWin32GLFunc(const char *name) {
  void *p = (void *)wglGetProcAddress(name);
  if(p == 0 ||
    (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
    (p == (void*)-1) )
  {
    HMODULE module = LoadLibraryA("opengl32.dll");
    if(!module) {
      return NULL;
    }
    p = (void *)GetProcAddress(module, name);
  }
  return p;
}