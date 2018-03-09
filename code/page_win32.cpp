#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include <gl/gl.h>
#include <math.h>

#include <intrin.h>
#include <immintrin.h>

#include <stdlib.h>
#include <time.h>

#include "aro_generic.h"
#include "aro_math.h"
#include "rendering.h"
#include "aro_opengl.h"
#include "aro_platform_win32.h"
#include "gamestate.h"
#include "page_win32.h"
static bool globalRunning;
static Win32WindowDimensions globalWindowDimensions;
static Player player;
const float playerDistPerSec = .25;
bool movingLeft = false;
bool movingRight = false;
LRESULT CALLBACK
win32MainWindowCallback(HWND window, UINT message, WPARAM WParam, LPARAM LParam) {       
  LRESULT result = 0;
  if(message == WM_KEYDOWN) {
    switch(WParam) {
      case 'W': {
      }
      break;
      case 'A': {
        movingRight = false;
        movingLeft = true; 
      }
      break;
      case 'S': {}
      break;
      case 'D': {
        movingLeft = false;
        movingRight = true;
      }
      break;
      case VK_ESCAPE: {
        globalRunning = false;
      } break;
      default: {
        return result;
      } break;
    }
  }
  else {
    switch(message)
    {
      case WM_CLOSE: {
        globalRunning = false;
      } break;
      case WM_ACTIVATEAPP: {
        if(WParam == WA_INACTIVE) {
          OutputDebugStringA("App sent to background\n");
        }
        else {
          OutputDebugStringA("App sent to foreground\n");
        }
      } break;
      case WM_DESTROY: {
        globalRunning = false;
      } break;
      case WM_PAINT: {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(window, &Paint);
        globalWindowDimensions = win32GetWindowDimension(window);
        EndPaint(window, &Paint);
        glViewport(0, 0, globalWindowDimensions.width, globalWindowDimensions.height);
      } break;
      default: {
        result = DefWindowProc(window, message, WParam, LParam);
      } break;
    }
  }
  return(result);
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
  srand(time(NULL));
  WNDCLASS windowClass = {};
  windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  windowClass.lpfnWndProc = win32MainWindowCallback;
  windowClass.hInstance = instance;
  //  windowClass.hIcon = ;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW); 
  //ShowCursor(FALSE);
  windowClass.lpszClassName = "Page";
  if(RegisterClassA(&windowClass))
  {
  //drawable area 1280x800
    RECT clientRect;
    clientRect.left = 80;
    clientRect.right = clientRect.left + 1279;
    clientRect.top = 80;
    clientRect.bottom = clientRect.top + 799;
    if(!AdjustWindowRectEx(&clientRect, WS_OVERLAPPEDWINDOW|WS_VISIBLE, FALSE, NULL)){ 
      InvalidCodePath;
    }
    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Page",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, clientRect.left, clientRect.top,
        clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
        0, 0, instance, 0);
    if(window) {
      //TODO: make this not broken
      RECT mouseClipRect;           // new area for ClipCursor
      RECT oldMouseClipRect;        // previous area for ClipCursor
      // Record the area in which the cursor can move. 
      GetClipCursor(&oldMouseClipRect); 
      // Get the dimensions of the application's window. 
      GetWindowRect(window, &mouseClipRect); 
      // Confine the cursor to the application's window. 
      ClipCursor(&mouseClipRect); 
      // Restore the cursor to its previous area. 
      ClipCursor(&oldMouseClipRect); 

      win32InitOpenGL(window); //find a pixel format, wglCreateContext, etc.
      //All OpenGL functions are now loaded. 

      HDC dc = GetDC(window);
      globalRunning = true;

      LARGE_INTEGER lastCounter, counterFreq;
      QueryPerformanceCounter(&lastCounter);
      QueryPerformanceFrequency(&counterFreq);

      float t = 0.0f;
      float spriteFlipCounter = 0.0f;

      Renderer renderer;
      renderer.initialize();
      player.center.x = 0;
      player.center.y = 0;
      player.center.z = 0;

      while(globalRunning)
      {
        LARGE_INTEGER beginCounter;
        QueryPerformanceCounter(&beginCounter);
        //TODO: tracking short durations is okay...BUT LET'S NOT USE FLOATS FOR TOTAL GAME TIME THAT'S BAD
        float dif = beginCounter.QuadPart - lastCounter.QuadPart;
        float lastFrameMS = (1000 * dif) / (float)counterFreq.QuadPart;
        float lastFrameSec = dif/counterFreq.QuadPart;
        //sprintf_s(strTime, 40, "turn:%f\n", gameState.turn);
        //OutputDebugString(strTime);

        float fps = (float) counterFreq.QuadPart / (float)dif;
        //sprintf_s(strTime, 40, "fps:%f\n", fps);
        //OutputDebugString(strTime);
        lastCounter.QuadPart = beginCounter.QuadPart;

        //poll for keyboard input
        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          if(message.message == WM_QUIT)
          {
            globalRunning = false;
          }
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }

        //updatePlayerMovement(&player, lastFrameSec);
        //frameUpdateCleanup();
        float moveDir = 0.0;
        if(movingLeft) {
          moveDir = -1.0f;
        }
        if(movingRight) {
          moveDir = 1.0f;
        }
        player.center.x += moveDir*playerDistPerSec*lastFrameSec;
        
        glClearColor(1.0,0.0,1.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(renderer.shaderID);
        t += lastFrameSec/2.0f; 
        v3 xmov = V3(sin(t),0.0f,0.0f);
        GLuint color = glGetUniformLocation(renderer.shaderID, "colorIn");
        glUniform3f(color,1,0,0);
        /* Currently no use for a per-quad model matrix
        4x4 modelMat = identity();
        GLuint modelID = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelID, 1, GL_TRUE, (float*) modelMat.n);*/

        v3 cameraDir = V3(0,0,-1);
        v3 cameraUp = V3(0,1,0);
        v3 cameraRight = normalize(cross(cameraUp, cameraDir));


        m4x4 projMat;
        aroFrustrum(player.center.y - 2, player.center.y + 2, 
          player.center.x - 2, player.center.x + 2, 0.0f, 10.0f, projMat);
        GLuint projID = glGetUniformLocation(renderer.shaderID, "projection");
        glUniformMatrix4fv(projID, 1, GL_FALSE, (float*) projMat.n);
        renderer.draw();
        glFinish();
        SwapBuffers(dc);
      }    
    }
    else
    {
        // TODO: Window creation failed, log?
    }
  }
  else
  {
      // TODO: registering window class failed, log?
  }
    
  return(0);
}

