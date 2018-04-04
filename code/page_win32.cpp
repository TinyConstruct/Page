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
static Win32WindowDimensions globalWindowDimensions;
static Win32WindowLocation globalWindowLocation;
static Player player;
const float playerDistPerSec = .25;
static LevelGeometry levelGeo;
static LevelData levelData;
static Renderer renderer;
KeyboardState keyboardState;
static TextureHandle texTable[MAX_TEX];

bool isColliding(AABB a, AABB b) {
  if (abs(a.center.x - b.center.x) > (a.rad.x + b.rad.x)) return false;
  if (abs(a.center.y - b.center.y) > (a.rad.y + b.rad.y)) return false;
  if (abs(a.center.z - b.center.z) > (a.rad.z + b.rad.z)) return false;
  return true;
}

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
          keyboardState.pause = !keyboardState.pause;
          SetCursorPos(globalWindowLocation.centerX, globalWindowLocation.centerY);
        }
        else {
          keyboardState.pause = !keyboardState.pause;
        }
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
      case WM_SIZE: {
        renderer.aspectRatio = (float)globalWindowDimensions.width / (float) globalWindowDimensions.height;
      }
      case WM_MOVE: {
        globalWindowDimensions = win32GetWindowDimension(window);
        POINT upperLeft;
        upperLeft.x = 0;
        upperLeft.y = 0;
        ClientToScreen(window, &upperLeft);
        globalWindowLocation.upperLeftX = upperLeft.x;
        globalWindowLocation.upperLeftY = upperLeft.y;
        globalWindowLocation.centerX = globalWindowLocation.upperLeftX + (globalWindowDimensions.width/2);
        globalWindowLocation.centerY = globalWindowLocation.upperLeftY + (globalWindowDimensions.height/2);
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
  ShowCursor(FALSE);
  windowClass.lpszClassName = "Page";
  if(RegisterClassA(&windowClass))
  {
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

      //load assets
      renderer.loadTextureArray512(FLOOR1, "data/floor1.bmp", FLOOR2, "data/floor2.bmp", 
        WALL2, "data/wall2.bmp", CEIL1, "data/ceil1.bmp");
      //load level geometry/data
      levelData.initialize(&levelGeo, &renderer);
      levelGeo.initialize();
      levelData.bakeTestLevel();

      for(int i = 0; i < levelGeo.boundingBoxes.size(); i++) {
        renderer.addDebugVolume(levelGeo.boundingBoxes[i].center, levelGeo.boundingBoxes[i].rad);
      }

      renderer.initialize();
      keyboardState.initialize();
      player.initialize();
      POINT mousePoint;
      SetCursorPos(globalWindowLocation.centerX, globalWindowLocation.centerY);
      float time = 0;
      renderer.aspectRatio = (float)globalWindowDimensions.width / (float) globalWindowDimensions.height;

      v3 gravity = V3(0.0, -9.86, 0.0);
      player.vel = V3(0.0,0.0,0.0);
      player.acc = V3(0.0,0.0,0.0);
      float mouseLookDampen = .5;
      v3 playerOffset = V3(0.0,1.75,0.0);

      //createTextureArrayBuffer(10,10);
//// Main Game Loop ////////
      while(globalRunning)
      {
        //update timer
        LARGE_INTEGER beginCounter;
        QueryPerformanceCounter(&beginCounter);
        float dif = beginCounter.QuadPart - lastCounter.QuadPart;
        float lastFrameMS = (1000 * dif) / (float)counterFreq.QuadPart;
        float lastFrameSec = dif/counterFreq.QuadPart;
        float fps = (float) counterFreq.QuadPart / (float)dif;
        lastCounter.QuadPart = beginCounter.QuadPart;

        time += lastFrameSec;

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

        v3 viewDir;
        float pitch = degToRad(player.pitch);
        float yaw = degToRad(player.yRotation);
        yaw = normalizeDeg(yaw);

        if(!keyboardState.pause){
        //get mouse and keyboard movement if not paused
          GetCursorPos(&mousePoint);
          int mouseDifX = mousePoint.x - globalWindowLocation.centerX;
          int mouseDifY = mousePoint.y - globalWindowLocation.centerY;
          player.pitch -= mouseDifY*mouseLookDampen;
          player.yRotation += mouseDifX*mouseLookDampen;
          if(player.pitch > 89){ 
            player.pitch = 89;
          }
          else if(player.pitch < -89){
            player.pitch = -89;
          }
          SetCursorPos(globalWindowLocation.centerX, globalWindowLocation.centerY);
          viewDir.x = cos(yaw) * cos(pitch);
          viewDir.y = sin(pitch);
          viewDir.z = sin(yaw) * cos(pitch);
          viewDir = normalize(viewDir);
          v3 side = cross(viewDir, V3(0.0, 1.0, 0.0));
          v3 upDir = cross(normalize(side), viewDir);
          v3 movementDir = V3(0.0, 0.0, 0.0);
          if (keyboardState.moveForward) {
            movementDir.x = movementDir.x + viewDir.x;
            movementDir.z = movementDir.z + viewDir.z;
          }
          if (keyboardState.moveBackward) {
            movementDir.x = movementDir.x - viewDir.x;
            movementDir.z = movementDir.x - viewDir.z;
          }
          if (keyboardState.strafeRight) {
            movementDir = movementDir + side;
          }
          if (keyboardState.strafeLeft) {
            movementDir = movementDir - side;
          }
          if (keyboardState.jump && player.onGround) {
            player.vel.y = sqrt(-.6*gravity.y); // vf*vf = -g*.6
            player.onGround = false;
          }
          if (keyboardState.duck) {
            movementDir = movementDir;
          }

          player.acc.y +=  lastFrameSec*gravity.y;
          player.vel = player.vel + lastFrameSec*player.acc;
          player.center = lastFrameSec*player.vel + player.center;
          
          AABB playerBox;
          movementDir = normalize(movementDir);
          playerBox.center.x = player.center.x + (player.walkingSpeed * lastFrameSec * movementDir.x);
          playerBox.center.y = player.center.y + (player.walkingSpeed * lastFrameSec * movementDir.y);
          playerBox.center.z = player.center.z + (player.walkingSpeed * lastFrameSec * movementDir.z);
          player.viewDir = viewDir;
          bool noCols = true;

          //for(int i = 0; i < levelGeo.boundingBoxes.size(); i++) {
          //  if(isColliding(playerBox, levelGeo.boundingBoxes[i])) {
          //    noCols = false;
          //    break;
          //  }
          //}
          //if(noCols) {
            player.center = playerBox.center;
          //}
          if(player.center.y < .5) {
            player.center.y = .5;
            player.acc.y = 0.0;
            player.vel.y = 0.0;
            player.onGround = true;
          }
        }
        else {
          //game is paused, draw pause screen/menu/whatever
        }

        glClearColor(1.0,0.0,1.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(renderer.shaderID);

        v3 cameraPosition = V3(player.center.x, player.center.y, player.center.z) + playerOffset;
        
        char buffer[512];
        sprintf_s(buffer, "Center: x %f y %f z %f\n", player.center.x, player.center.y, player.center.z);
        OutputDebugStringA(buffer);
        
        sprintf_s(buffer, "Player view: x %f y %f z %f\n", viewDir.x, viewDir.y, viewDir.z);
        OutputDebugStringA(buffer);

        m4x4 modelMat = identity();
        GLint modelID = glGetUniformLocation(renderer.shaderID, "model");
        glUniformMatrix4fv(modelID, 1, GL_FALSE, (float*)modelMat.n);
        
        m4x4 projMat; 
        aroPerspective(projMat, 45.0f, renderer.aspectRatio, 0.1f, 100.0f);
        m4x4 viewMat = aroLookat(cameraPosition, cameraPosition + player.viewDir);

        GLint viewID = glGetUniformLocation(renderer.shaderID, "view");
        glUniformMatrix4fv(viewID, 1, GL_TRUE, (float*) viewMat.n);

        GLint projID = glGetUniformLocation(renderer.shaderID, "projection");
        glUniformMatrix4fv(projID, 1, GL_FALSE, (float*)projMat.n);

        GLint lightID = glGetUniformLocation(renderer.shaderID, "lightPos");
        glUniform3f(lightID, player.center.x, player.center.y, player.center.z);

        GLint playerPosID = glGetUniformLocation(renderer.shaderID, "playerPos");
        glUniform3f(playerPosID, player.center.x, player.center.y, player.center.z);

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

