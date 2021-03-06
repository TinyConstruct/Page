#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

#include <gl/gl.h>
#include <math.h>

#include <intrin.h>
#include <immintrin.h>

#include <stdlib.h>
#include <time.h>

#include "aro_generic.h"
#include "aro_math.h"
#include "texture.h"
#include "rendering.h"
#include "level.h"
#include "aro_opengl.h"
#include "aro_platform_win32.h"
#include "gamestate.h"
#include "page_win32.h"

static bool globalRunning;
static Win32WindowLocation globalWindowLocation;
static HWND globalWindowHandle;
static Player player;
static LevelGeometry levelGeo;
static LevelData levelData;
static Renderer renderer;
static KeyboardState keyboardState;
static Camera freeCamera;
#if GL_DEBUG
  static GLDebugLog glDebugLog;
#endif

LRESULT CALLBACK
win32MainWindowCallback(HWND window, UINT message, WPARAM WParam, LPARAM LParam) {       
  LRESULT result = 0;
  if(message == WM_KEYDOWN) {
    switch(WParam) {
      #if DEBUG_BUILD
      case 'L': {
        renderer.renderDebug = !renderer.renderDebug;
      }
      break;
      #endif
      case 'W': {
        keyboardState.moveForward = true;
      }
      break;
      case 'A': {
        keyboardState.strafeLeft = true; 
      }
      break;
      case 'S': {
        keyboardState.moveBackward = true;
      }
      break;
      case 'D': {
        keyboardState.strafeRight = true;
      }
      break;
      case 'P': {
        if(keyboardState.pause) {
          ShowCursor(FALSE);
          keyboardState.pause = !keyboardState.pause;
          SetCursorPos(globalWindowLocation.centerX, globalWindowLocation.centerY);
        }
        else {
          ShowCursor(TRUE);
          keyboardState.pause = !keyboardState.pause;
        }
      }
      break;
      case 'C': {
        keyboardState.cameraLock = !keyboardState.cameraLock;
        freeCamera.center = player.center;
        freeCamera.yRotation = player.yRotation;
        freeCamera.pitch = player.pitch;
        freeCamera.viewDir = player.viewDir;
      }
      break;
      case 'H': {
        if(renderer.numMSAASamples == 1) {
          renderer.numMSAASamples = 4;
        }
        else if (renderer.numMSAASamples == 4){
          renderer.numMSAASamples = 8;
        }
        else {
          renderer.numMSAASamples = 1;
        }
      renderer.reconfigureMSAA();
      }
      break;
      case VK_ESCAPE: {
        globalRunning = false;
      } break;
      case VK_LEFT: {
        keyboardState.turnLeft = true;
      } break;
      case VK_RIGHT: {
        keyboardState.turnRight = true;
      } break;
      case VK_SPACE: {
        keyboardState.jump = true;
      } break;
      case VK_CONTROL: {
        keyboardState.duck = true;
      } break;
      default: {
        return result;
      } break;
    }
  }
  else if (message == WM_KEYUP) {
    switch (WParam){
      case 'W': {
        keyboardState.moveForward = false;
      }
      break;
      case 'A': {
       keyboardState.strafeLeft = false; 
      }
      break;
      case 'S': {
        keyboardState.moveBackward = false;
      }
      break;
      case 'D': {
        keyboardState.strafeRight = false;
      }
      break;
      case VK_LEFT: {
        keyboardState.turnLeft = false;
      } break;
      case VK_RIGHT: {
        keyboardState.turnRight = false;
      } break;
      case VK_SPACE: {
        keyboardState.jump = false;
      } break;
      case VK_CONTROL: {
        keyboardState.duck = false;
      } break;
    }
  }
  else {
    switch(message) {
      case WM_CLOSE: {
        globalRunning = false;
      } break;
      case WM_ACTIVATEAPP: {
        if(WParam == WA_INACTIVE) {
          OutputDebugStringA("App sent to background\n");
          if(renderer.clipMouse) {
            unclipMouse();
          }
        }
        else {
          if(renderer.clipMouse) {
            clipMouseToWindow(globalWindowHandle);
          }
          OutputDebugStringA("App sent to foreground\n");
        }
      } break;
      case WM_DESTROY: {
        globalRunning = false;
      } break;
      case WM_SIZE: {
        renderer.windowDimensions = win32GetWindowDimension(window);
        renderer.aspectRatio = (float)renderer.windowDimensions.width / (float) renderer.windowDimensions.height;
        if(renderer.clipMouse) {
          clipMouseToWindow(globalWindowHandle);
        }
		if (renderer.initDone) {
			//re-create multisample texture
			//glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderer.multisampleTextureLocation);
			//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, renderer.windowDimensions.width, renderer.windowDimensions.height, GL_TRUE);
			//glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
			//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, renderer.multisampleTextureLocation, 0);
      renderer.reconfigureMSAA();
		}
      }
      case WM_MOVE: {
        renderer.windowDimensions = win32GetWindowDimension(window);
        POINT upperLeft;
        upperLeft.x = 0;
        upperLeft.y = 0;
        ClientToScreen(window, &upperLeft);
        globalWindowLocation.upperLeftX = upperLeft.x;
        globalWindowLocation.upperLeftY = upperLeft.y;
        globalWindowLocation.centerX = globalWindowLocation.upperLeftX + (renderer.windowDimensions.width/2);
        globalWindowLocation.centerY = globalWindowLocation.upperLeftY + (renderer.windowDimensions.height/2);
        if(renderer.clipMouse) {
          clipMouseToWindow(globalWindowHandle);
        }
      } break;
      case WM_PAINT: {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(window, &Paint);
        renderer.windowDimensions = win32GetWindowDimension(window);
        EndPaint(window, &Paint);
        glViewport(0, 0, renderer.windowDimensions.width, renderer.windowDimensions.height);
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
  srand((unsigned int)time(NULL));
  renderer.clipMouse = FALSE;
  renderer.initDone = FALSE;
  WNDCLASS windowClass = {};
  windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  windowClass.lpfnWndProc = win32MainWindowCallback;
  windowClass.hInstance = instance;
  //  windowClass.hIcon = ;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW); 
  ShowCursor(FALSE);
  windowClass.lpszClassName = "Page";
  if(RegisterClassA(&windowClass)) {
  //drawable area 1280x800
    RECT clientRect;
    //AdjustWindowRectEx expects a RECT that contains the upper left and bottom right of the desired area, but it's exclusive for the greater bound, 
    //so for 1280 pixels wide it should be "+1280," not "+1279"
    clientRect.left = 80;
    clientRect.right = clientRect.left + 1280;
    clientRect.top = 80;
    clientRect.bottom = clientRect.top + 720;
    if(!AdjustWindowRectEx(&clientRect, WS_OVERLAPPEDWINDOW|WS_VISIBLE, FALSE, NULL)){ 
      InvalidCodePath;
    }
    globalWindowHandle = CreateWindowExA(0, windowClass.lpszClassName, "Page",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, clientRect.left, clientRect.top,
        clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
        0, 0, instance, 0);
    if(globalWindowHandle) {
      if(renderer.clipMouse){
        clipMouseToWindow(globalWindowHandle);
      }

      win32InitOpenGL(globalWindowHandle); //find a pixel format, wglCreateContext, etc.
      //All OpenGL functions are now loaded. 
	  renderer.initDone = TRUE;
      HDC dc = GetDC(globalWindowHandle);
      globalRunning = true;

      LARGE_INTEGER lastCounter, counterFreq;
      QueryPerformanceCounter(&lastCounter);
      QueryPerformanceFrequency(&counterFreq);

      float t = 0.0f;
      float spriteFlipCounter = 0.0f;

      //load level geometry/data
      renderer.globalLight = V3(0.0f, 2.0, -0.5);
      levelData.initialize(&levelGeo, &renderer);
      levelGeo.initialize();

      levelData.bakeTestLevel();
      for(size_t i = 0; i < levelGeo.AABBs.size(); i++) {
        renderer.addDebugVolume(levelGeo.AABBs[i].center, levelGeo.AABBs[i].rad);
      }
      renderer.addDebugVolume(renderer.globalLight, V3(.15,.15,.15));
      renderer.debugDrawLine(renderer.globalLight, V3(0,0,0));
      for (size_t i = 0; i < levelGeo.OBBs.size(); i++) {
        renderer.addDebugVolume(levelGeo.OBBs[i].c, levelGeo.OBBs[i].u, levelGeo.OBBs[i].width);
      }
      renderer.initialize();
      keyboardState.initialize();
      player.initialize();
      POINT mousePoint;
      SetCursorPos(globalWindowLocation.centerX, globalWindowLocation.centerY);
      float time = 0.0f;
      renderer.aspectRatio = (float) renderer.windowDimensions.width / (float) renderer.windowDimensions.height;
      player.vel = V3(0.0f,0.0f,0.0f);
      player.acc = V3(0.0f,0.0f,0.0f);
      v3 playerOffset = V3(0.0f,0.0f,0.0f);

      static LARGE_INTEGER beginProfGL, endProfGL;
      LARGE_INTEGER lastStutter;
      QueryPerformanceCounter(&lastStutter);
//// Main Game Loop ////////
      while(globalRunning) {
        //update timer
        LARGE_INTEGER beginCounter;
        static LARGE_INTEGER beginProfMsg, endProfMsg;
        static LARGE_INTEGER beginProfMove, endProfMove;
        assert(0 != QueryPerformanceCounter(&beginCounter));
        LONGLONG dif = (beginCounter.QuadPart - lastCounter.QuadPart);
        float lastFrameMS = (float) (1000 * dif) / (float)counterFreq.QuadPart;
        float lastFrameSec = (float)dif / (float) counterFreq.QuadPart;
        //assert((time < 5 ) || (dif < 100000));
        float fps = (float) counterFreq.QuadPart / (float)dif;
        lastCounter.QuadPart = beginCounter.QuadPart;
        time += lastFrameSec;

        
        QueryPerformanceCounter(&beginProfMsg);
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
        QueryPerformanceCounter(&endProfMsg);
        
        //These are local in order to support multiple cameras
        v3 cameraPosition, viewDir;
        if(!keyboardState.pause){
        //get mouse and keyboard movement if not paused
        GetCursorPos(&mousePoint);
        float yRotationDif = (mousePoint.x - globalWindowLocation.centerX)*mouseLookDampen;
        float pitchDif = -(mousePoint.y - globalWindowLocation.centerY)*mouseLookDampen;
        SetCursorPos(globalWindowLocation.centerX, globalWindowLocation.centerY);
          if(keyboardState.cameraLock) { //camera is locked to player
            QueryPerformanceCounter(&beginProfMove);
            levelGeo.movePlayer(player, keyboardState, pitchDif, yRotationDif, lastFrameSec);
            renderer.addPlayerDebugVolume(levelGeo.playerOBB.c, levelGeo.playerOBB.u, levelGeo.playerOBB.width);
            QueryPerformanceCounter(&endProfMove);
            cameraPosition = V3(player.center.x, player.center.y, player.center.z) + playerOffset;
            viewDir = player.viewDir;
          }
          else { //currently using free cam
            levelGeo.moveFreeCam(freeCamera, keyboardState, pitchDif, yRotationDif, lastFrameSec);
            cameraPosition = freeCamera.center;
            viewDir = freeCamera.viewDir;
          }
        }
        else {
          //game is paused, draw pause screen/menu
          if(keyboardState.cameraLock) { //camera is locked to player
            cameraPosition = V3(player.center.x, player.center.y, player.center.z) ;
            viewDir = player.viewDir;
          }
          else { //currently using free cam
            viewDir = freeCamera.viewDir;
            cameraPosition = freeCamera.center;
          }
        }
        
        m4x4 viewMat = aroLookat(cameraPosition, cameraPosition + viewDir);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(m4x4), viewMat.n);

        m4x4 projMat; 
        aroPerspective(projMat, 60.0f, renderer.aspectRatio, 0.1f, 1000.0f);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(m4x4), sizeof(m4x4), projMat.n);

        GLint playerPosID = glGetUniformLocation(renderer.shaderID, "playerPos");
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(m4x4)*2, sizeof(v3), &cameraPosition);

        renderer.draw();
        glFinish();
        SwapBuffers(dc);
        char buffer[512];
        sprintf_s(buffer, "Player: %f, %f, %f\n", player.center.x, player.center.y, player.center.z);
        OutputDebugStringA(buffer);
        #if GL_DEBUG
          glGetDebugMessageLog(100, 50000, glDebugLog.sources, glDebugLog.types, glDebugLog.ids, glDebugLog.severities, glDebugLog.lengths, glDebugLog.buffer);
          int debugTestBreakpoint = 0;
          debugTestBreakpoint++;
        #endif
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

