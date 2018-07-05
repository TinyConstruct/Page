#include <Windows.h>
#include <gl/gl.h>
#include <math.h>
#include <stdint.h>
#include <vector>
#include <memory.h>  

#include "aro_platform_win32.h"
#include "aro_generic.h"
#include "aro_opengl.h"
#include "texture.h"
#include "aro_math.h"
#include "gamestate.h"

#include "rendering.h"

const char* vertexSourceUBUF = R"FOO(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 texCoordIn;
layout (location = 4) in vec3 tangentIn;
layout (location = 5) in vec3 bitangentIn;
layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};

uniform mat4 lightSpaceMatrix;

out vec3 fragPos;
out vec4 fragPosLightspace;
out vec3 texCoordOutFromVert;
out vec3 tangentLightPos;
out vec3 tangentViewPos;
out vec3 tangentFragPos;
out vec3 lightPos;
out vec3 tangentDirectionalLight;

void main()
{
  gl_Position =  projection * view * vec4(position.x, position.y, position.z, 1.0f);
  fragPos = position.xyz;
  texCoordOutFromVert = texCoordIn;
  lightPos  =vec3(.5, 4.0, .5);
  vec3 directionalLight = -(normalize(vec3(-.5, -4.0, -.5)));

  vec3 T = normalize(tangentIn);
  vec3 B = normalize(bitangentIn);
  vec3 N = normalize(normal);
  mat3 TBN = transpose(mat3(T, B, N));
  tangentLightPos = TBN * lightPos;
  tangentViewPos = TBN * playerPos;
  tangentFragPos = TBN * fragPos;
  tangentDirectionalLight = TBN * directionalLight;
  fragPosLightspace =  lightSpaceMatrix * vec4(fragPos, 1.0);
}
)FOO";

const char* fragmentSource = R"FOO(
#version 450 core
uniform sampler2DArray textureData;
uniform sampler2DArray bumpMap;
uniform sampler2D depthMap;

in vec3 fragPos;
in vec3 texCoordOutFromVert;
in vec3 tangentLightPos;
in vec3 tangentViewPos;
in vec3 tangentFragPos;
in vec3 lightPos;
in vec3 tangentDirectionalLight;
in vec4 fragPosLightspace;

layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};
out vec4 resultColor;

float shadowCalculation(vec4 fragPosLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(depthMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;
    return shadow;
}  

void main()
{
  vec3 normal = texture(bumpMap, texCoordOutFromVert.xyz).rgb;
  normal = normalize(normal * 2.0 - 1.0);
  vec3 texColor = texture(textureData, vec3(texCoordOutFromVert.xyz)).rgb;
  //texColor = texture(depthMap, texCoordOutFromVert.xy).rgb;
  vec3 lightcolor = vec3(1.0,1.0,1.0);
  
//point light stuff
  float distance = length(fragPos - lightPos);
  float attenuation = 1.0 / (1 + .07 * distance + .017 * (distance * distance));    

  //ambient
  float ambientIntensity = .1;
  vec3 ambient = ambientIntensity*texColor;
  //diffuse  
  vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * texColor;
  //specular
  vec3 viewerDir = normalize(tangentViewPos - tangentFragPos);
  vec3 reflectedDir = reflect(-lightDir, normal);
  vec3 halfwayDir = normalize(lightDir + viewerDir);  
  float specularIntensity = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
  vec3 specular = vec3(0.2) * specularIntensity;

  ambient  *= attenuation; 
  diffuse  *= attenuation;
  specular *= attenuation;   

//Directional light stuff
  //ambient
  ambientIntensity= .6;
  ambient = ambientIntensity*texColor;
  //diffuse  
  lightDir = tangentDirectionalLight;
  diff = max(dot(normal, lightDir), 0.0);
  diffuse = diff * texColor;
  //specular
  viewerDir = normalize(tangentViewPos - tangentFragPos);
  reflectedDir = reflect(-lightDir, normal);
  halfwayDir = normalize(lightDir + viewerDir);  
  specularIntensity = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
  specular = vec3(0.2) * specularIntensity;

  float shadow = shadowCalculation(fragPosLightspace);

  resultColor = vec4((ambient + (1.0-shadow) * (diffuse+ specular))*lightcolor, 1.0);
  //resultColor = vec4(ambient, 1.0);
  //resultColor = vec4((ambient + (diffuse + specular))*lightcolor, 1.0);
}
)FOO";


const char* noBumpVertexSourceUBUF = R"FOO(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 texCoordIn;
layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};

uniform mat4 lightSpaceMatrix;
uniform vec3 globalLight;

out vec3 fragPos;
out vec4 fragPosLightspace;
out vec3 texCoordOutFromVert;
out vec3 lightPos;
out vec3 normalOutFromVert;

void main()
{
  gl_Position =  projection * view * vec4(position.x, position.y, position.z, 1.0f);
  fragPos = position.xyz;
  texCoordOutFromVert = texCoordIn;
  lightPos  = globalLight;
  normalOutFromVert = normal;
  fragPosLightspace =  lightSpaceMatrix * vec4(fragPos, 1.0);
}
)FOO";

const char* noBumpFragmentSource = R"FOO(
#version 450 core
uniform sampler2DArray textureData;
uniform sampler2D depthMap;

in vec3 fragPos;
in vec3 texCoordOutFromVert;
in vec3 lightPos;
in vec4 fragPosLightspace;
in vec3 normalOutFromVert;

layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};
out vec4 resultColor;

float shadowCalculation(vec4 fragPosLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(depthMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;
    return shadow;
}  

void main()
{
  vec3 normal = normalize(normalOutFromVert);
  vec3 texColor = texture(textureData, vec3(texCoordOutFromVert.xyz)).rgb;
  //vec3 texColor = texture(depthMap, texCoordOutFromVert.xy).rgb;
  vec3 lightcolor = vec3(1.0,1.0,1.0);
  
//Directional light stuff
  //ambient
  float ambientIntensity= .6;
  vec3 ambient = ambientIntensity*texColor;
  //diffuse  
  vec3 lightDir = normalize(lightPos - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * texColor;
  //specular
  vec3 viewerDir = normalize(playerPos - fragPos);
  vec3 halfwayDir = normalize(lightDir + viewerDir);  
  float specularIntensity = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
  vec3 specular = vec3(0.2) * specularIntensity;

  float shadow = shadowCalculation(fragPosLightspace);
  resultColor = vec4(fragPosLightspace.x, fragPosLightspace.y, fragPosLightspace.z, 1.0);
  //resultColor = vec4(shadow, shadow, shadow, 1.0);
  resultColor = vec4((ambient + (1.0-shadow) * (diffuse+ specular))*lightcolor, 1.0);
  //resultColor = vec4(ambient, 1.0);
  //resultColor = vec4((ambient + (diffuse + specular))*lightcolor, 1.0);

}
)FOO";

const char* debugWhiteFragmentSource = R"FOO(
#version 450 core
in vec3 fragPos;
layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};
out vec4 resultColor;
void main()
{
  resultColor = vec4(1.0,1.0,1.0,1.0);
}
)FOO";

const char* depthMapVertSource = R"FOO(
#version 450 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = lightSpaceMatrix * vec4(aPos, 1.0);
}  
)FOO";

const char* depthMapFragSource = R"FOO(
#version 450 core
out vec4 resultColor;
void main()
{
  gl_FragDepth = gl_FragCoord.z;
  resultColor = vec4(gl_FragCoord.z,gl_FragCoord.z,gl_FragCoord.z,1.0);

}  
)FOO";

int texStrToID(char* str) {
  if(strcmpAny(str, "ceil1.bmp")) {
    return CEIL1;
  }
  else if(strcmpAny(str, "city2_2.bmp")) {
    return CITY2_2;
  }
  else if(strcmpAny(str, "city3_2.bmp")) {
    return CITY3_2;
  }
  else if(strcmpAny(str, "city4_1.bmp")) {
    return CITY4_1;
  }
  else if(strcmpAny(str, "floor1.bmp")) {
    return FLOOR1;
  }
  else if(strcmpAny(str, "floor2.bmp")) {
    return FLOOR2;
  }
  else if(strcmpAny(str, "floor3.bmp")) {
    return FLOOR3;
  }
  else if(strcmpAny(str, "metalbox.bmp")) {
    return METALBOX;
  }
  else if(strcmpAny(str, "wall1.bmp")) {
    return WALL1;
  }
  else if(strcmpAny(str, "wall2.bmp")) {
    return WALL2;
  }
  else {
    InvalidCodePath;
    return 0;
  }
}

int texNormStrToID(char* str) {
  if(strcmpAny(str, "n_ceil1.bmp")) {
    return N_CEIL1;
  }
  else if(strcmpAny(str, "n_city2_2.bmp")) {
    return N_CITY2_2;
  }
  else if(strcmpAny(str, "n_city3_2.bmp")) {
    return N_CITY3_2;
  }
  else if(strcmpAny(str, "n_city4_1.bmp")) {
    return N_CITY4_1;
  }
  else if(strcmpAny(str, "n_floor1.bmp")) {
    return N_FLOOR1;
  }
  else if(strcmpAny(str, "n_floor2.bmp")) {
    return N_FLOOR2;
  }
  else if(strcmpAny(str, "n_floor3.bmp")) {
    return N_FLOOR3;
  }
  else if(strcmpAny(str, "n_metalbox.bmp")) {
    return N_METALBOX;
  }
  else if(strcmpAny(str, "n_wall1.bmp")) {
    return N_WALL1;
  }
  else if(strcmpAny(str, "n_wall2.bmp")) {
    return N_WALL2;
  }
  else {
    InvalidCodePath;
    return 0;
  }
}

void compileShader(GLuint* shaderID, const char* vSrc, const char* fSrc) {
  GLuint vertShader, fragShader;
  int success;
  GLenum errorNum;
  errorNum = glGetError();

  vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertShader, 1, &vSrc, NULL);
  glCompileShader(vertShader);
  errorNum = glGetError();

  // print compile errors if any
  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    char buffer[512];
    glGetShaderInfoLog(vertShader, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }
  errorNum = glGetError();

  fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fSrc, NULL);
  glCompileShader(fragShader);
  // print compile errors if any
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    char buffer[512];
    glGetShaderInfoLog(fragShader, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }
  errorNum = glGetError();

  // shader Program
  *shaderID = glCreateProgram();
  glAttachShader(*shaderID, vertShader);

  errorNum = glGetError();
  glAttachShader(*shaderID, fragShader);
  errorNum = glGetError();
  glLinkProgram(*shaderID);
  errorNum = glGetError();

  // print linking errors if any
  glGetProgramiv(*shaderID, GL_LINK_STATUS, &success);
  if(!success)
  {
    char buffer[512];
    glGetProgramInfoLog(*shaderID, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }
  // delete the shaders as they're linked into our program now and no longer necessery
  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
}

void Renderer::initialize() {
  compileShader(&shaderID, noBumpVertexSourceUBUF, noBumpFragmentSource);
  GLuint uniformBlockIndex = glGetUniformBlockIndex(shaderID, "GlobalMatrices");
  glUniformBlockBinding(shaderID, uniformBlockIndex, 0);
  compileShader(&shadowShaderID, depthMapVertSource, depthMapFragSource);
  GLuint vao;
  glGenVertexArrays(1, &vao);  
  glBindVertexArray(vao);
    
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, level.size()*sizeof(Vertpcnu), level.data(), GL_DYNAMIC_DRAW);
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, levelElements.size()*sizeof(int), levelElements.data(), GL_DYNAMIC_DRAW);
  glGenBuffers(1, &uniformBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(m4x4) * 2 + sizeof(v3), NULL, GL_STREAM_DRAW);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer, 0, sizeof(m4x4) * 2 + sizeof(v3));

  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
  glEnableVertexAttribArray(posAttrib);
  //GLint color = glGetAttribLocation(shaderID, "color");
  //glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)));
  //glEnableVertexAttribArray(color);
  GLint normal = glGetAttribLocation(shaderID, "normal");
  glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*2));
  glEnableVertexAttribArray(normal);
  GLint texCoord = glGetAttribLocation(shaderID, "texCoordIn");
  glVertexAttribPointer(texCoord, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*3));
  glEnableVertexAttribArray(texCoord);

  textureLocation = glGetUniformLocation(shaderID, "textureData");
  normTextureLocation = glGetUniformLocation(shaderID, "bumpMap");
  depthMapTextureLocation = glGetUniformLocation(shaderID, "depthMap");

  glUseProgram(shaderID);
  glUniform1i(textureLocation, 0);
  glUniform1i(normTextureLocation, 1);
  glUniform1i(depthMapTextureLocation, 2);

  #if DEBUG_BUILD
    debugPlayerElements.push_back(0);
    debugPlayerElements.push_back(1);
    debugPlayerElements.push_back(1);
    debugPlayerElements.push_back(2);
    debugPlayerElements.push_back(2);
    debugPlayerElements.push_back(3);
    debugPlayerElements.push_back(3);
    debugPlayerElements.push_back(0);
    debugPlayerElements.push_back(4);
    debugPlayerElements.push_back(5);
    debugPlayerElements.push_back(5);
    debugPlayerElements.push_back(6);
    debugPlayerElements.push_back(6);
    debugPlayerElements.push_back(7);
    debugPlayerElements.push_back(7);
    debugPlayerElements.push_back(4);
    debugPlayerElements.push_back(4);
    debugPlayerElements.push_back(0);
    debugPlayerElements.push_back(7);
    debugPlayerElements.push_back(3);
    debugPlayerElements.push_back(5);
    debugPlayerElements.push_back(1);
    debugPlayerElements.push_back(6);
    debugPlayerElements.push_back(2);
    compileShader(&debugShaderID, vertexSourceUBUF, debugWhiteFragmentSource);
    renderDebug = false;
    glGenBuffers(1, &debugVbo);
    glGenBuffers(1, &debugEbo);
    glBindBuffer(GL_ARRAY_BUFFER, debugVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugEbo);
    glBufferData(GL_ARRAY_BUFFER, debugBoundingVerts.size()*sizeof(v3), debugBoundingVerts.data(), GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, debugBoundingElements.size()*sizeof(int), debugBoundingElements.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &debugPlayerVbo);
    glGenBuffers(1, &debugPlayerEbo);
    glBindBuffer(GL_ARRAY_BUFFER, debugPlayerVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugPlayerEbo);
    glBufferData(GL_ARRAY_BUFFER, debugPlayerVerts.size()*sizeof(v3), debugPlayerVerts.data(), GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, debugPlayerElements.size()*sizeof(int), debugPlayerElements.data(), GL_DYNAMIC_DRAW);
  #endif

  //begin shadowmap setup
  glGenFramebuffers(1, &depthMapFBO);
  glGenTextures(1, &depthMap);
  glBindTexture(GL_TEXTURE_2D, depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
  //glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);

  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);  

  //end shadowmap setup

  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  wglSwapInterval(1);
}

void Renderer::renderShadowMap() {
  
}

void Renderer::draw() {
  glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
  //Render for shadow map
  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glClear(GL_DEPTH_BUFFER_BIT);
  
  m4x4 lightProjection;
  getOrthoProjMatrix(-10.0f, 10.0f, -10.0f, 10.0f, .25f, 20.0f, lightProjection);
  //aroPerspective(lightProjection, 60.0f, 1, 0.1f, 1000.0f);
  m4x4 lightView = aroLookat(globalLight, V3(0.0f,0.0f,0.0f));
  m4x4 lightSpaceMatrix =  (transpose(lightProjection) * transpose(lightView));
  glUseProgram(shadowShaderID);

  GLint lightSpaceMatrixAttrib = glGetUniformLocation(shadowShaderID, "lightSpaceMatrix");
  glUniformMatrix4fv(lightSpaceMatrixAttrib, 1, GL_FALSE, (GLfloat*) lightSpaceMatrix.n);
  
  glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
  glActiveTexture(GL_TEXTURE0); 
  glBindTexture(GL_TEXTURE_2D_ARRAY, texTable[1].glTextureNum);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  GLint shadowPosAttrib = glGetAttribLocation(shadowShaderID, "aPos");
  glVertexAttribPointer(shadowPosAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
  glEnableVertexAttribArray(shadowPosAttrib);
  if(!(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)) {
    InvalidCodePath;
  }
  glDrawElements(GL_TRIANGLES, levelElements.size(), GL_UNSIGNED_INT, 0);
    
  //Normal render
  glClearColor(1.0,0.0,1.0,1.0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);  
  glViewport(0, 0, windowDimensions.width, windowDimensions.height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(shaderID);

//glEnable(GL_FRAMEBUFFER_SRGB); 
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  lightSpaceMatrixAttrib = glGetUniformLocation(shaderID, "lightSpaceMatrix");
  glUniformMatrix4fv(lightSpaceMatrixAttrib, 1, GL_FALSE, (GLfloat*) lightSpaceMatrix.n);
  GLint globalLightAttrib = glGetUniformLocation(shaderID, "globalLight");
  glUniform3fv(globalLightAttrib, 1, (GLfloat*) &globalLight);

  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
  glEnableVertexAttribArray(posAttrib);
  GLint normal = glGetAttribLocation(shaderID, "normal");
  glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*2));
  glEnableVertexAttribArray(normal);
  GLint texCoord = glGetAttribLocation(shaderID, "texCoordIn");
  glVertexAttribPointer(texCoord, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*3));
  glEnableVertexAttribArray(texCoord);
  //GLint tangent = glGetAttribLocation(shaderID, "tangentIn");
  //glVertexAttribPointer(tangent, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*4));
  //glEnableVertexAttribArray(tangent);
  //GLint bitangent = glGetAttribLocation(shaderID, "bitangentIn");
  //glVertexAttribPointer(bitangent,3 , GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*5));
  //glEnableVertexAttribArray(bitangent);

  glActiveTexture(GL_TEXTURE0 + 0); 
  glBindTexture(GL_TEXTURE_2D_ARRAY, texTable[1].glTextureNum);
  glActiveTexture(GL_TEXTURE0 + 1); 
  glBindTexture(GL_TEXTURE_2D_ARRAY, texNormTable[1].glTextureNum);
  glActiveTexture(GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, depthMap);

  //If we want to render the scene from the light's perspective:
  //glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(m4x4), lightView.n);
  //glBufferSubData(GL_UNIFORM_BUFFER, sizeof(m4x4), sizeof(m4x4), lightProjection.n);
  
  glDrawElements(GL_TRIANGLES, levelElements.size(), GL_UNSIGNED_INT, 0);
  #if DEBUG_BUILD
    if(renderDebug){
      glUseProgram(debugShaderID);
      glBindBuffer(GL_ARRAY_BUFFER, debugVbo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugEbo);
      posAttrib = glGetAttribLocation(debugShaderID, "position");
      glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(v3), 0);
      glEnableVertexAttribArray(posAttrib);
      glDrawElements(GL_LINES, debugBoundingElements.size(), GL_UNSIGNED_INT, 0);

      glBindBuffer(GL_ARRAY_BUFFER, debugPlayerVbo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugPlayerEbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v3)*debugPlayerVerts.size(), debugPlayerVerts.data());
      posAttrib = glGetAttribLocation(debugShaderID, "position");
      glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(v3), 0);
      glEnableVertexAttribArray(posAttrib);
      glDrawElements(GL_LINES, debugBoundingElements.size(), GL_UNSIGNED_INT, 0);
    }
  #endif
}

void Renderer::addDebugVolume(const v3& center, const v3& rad) {
  v3 newVert, offset;
  int startingIndex = debugBoundingVerts.size();
  offset = rad;
  //front plane
  newVert = V3(center.x - offset.x, center.y + offset.y, center.z + offset.z);//0
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x - offset.x, center.y - offset.y, center.z + offset.z);//1
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y - offset.y, center.z + offset.z);//2
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y + offset.y, center.z + offset.z);//3
  debugBoundingVerts.push_back(newVert);
  //back plane
  newVert = V3(center.x - offset.x, center.y + offset.y, center.z - offset.z);//4
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x - offset.x, center.y - offset.y, center.z - offset.z);//5
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y - offset.y, center.z - offset.z);//6
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y + offset.y, center.z - offset.z);//7
  debugBoundingVerts.push_back(newVert);
  //front
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+0);
  //back
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+4);
  //top
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+3);
  //bottom
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+2);
}

void Renderer::addDebugVolume(v3& center, v3 axes[3], v3& halfW) {

  v3 newVert, offset;
  int startingIndex = debugBoundingVerts.size();
  offset = halfW;

  //front plane
  //newVert = V3(center.x - offset.x, center.y + offset.y, center.z + offset.z);//0
  newVert = center - halfW.x*axes[0] + halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x - offset.x, center.y - offset.y, center.z + offset.z);//1
  newVert = center - halfW.x*axes[0] - halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y - offset.y, center.z + offset.z);//2
  newVert = center + halfW.x*axes[0] - halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y + offset.y, center.z + offset.z);//3
  newVert = center + halfW.x*axes[0] + halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  
  //back plane
  //newVert = V3(center.x - offset.x, center.y + offset.y, center.z - offset.z);//4
  newVert = center - halfW.x*axes[0] + halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x - offset.x, center.y - offset.y, center.z - offset.z);//5
  newVert = center - halfW.x*axes[0] - halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y - offset.y, center.z - offset.z);//6
  newVert = center + halfW.x*axes[0] - halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y + offset.y, center.z - offset.z);//7
  newVert = center + halfW.x*axes[0] + halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);

  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+0);
  //back
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+4);
  //top
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+3);
  //bottom
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+2);  
}

void Renderer::addPlayerDebugVolume(v3& center, v3 axes[3], v3& halfW) {
  v3 newVert, offset;
  int startingIndex = 0;
  offset = halfW;
  //front plane
  //newVert = V3(center.x - offset.x, center.y + offset.y, center.z + offset.z);//0
  newVert = center - halfW.x*axes[0] + halfW.y*axes[1] + halfW.z*axes[2];
  debugPlayerVerts[0] = newVert;
  //newVert = V3(center.x - offset.x, center.y - offset.y, center.z + offset.z);//1
  newVert = center - halfW.x*axes[0] - halfW.y*axes[1] + halfW.z*axes[2];
  debugPlayerVerts[1] = newVert;
  //newVert = V3(center.x + offset.x, center.y - offset.y, center.z + offset.z);//2
  newVert = center + halfW.x*axes[0] - halfW.y*axes[1] + halfW.z*axes[2];
  debugPlayerVerts[2] = newVert;
  //newVert = V3(center.x + offset.x, center.y + offset.y, center.z + offset.z);//3
  newVert = center + halfW.x*axes[0] + halfW.y*axes[1] + halfW.z*axes[2];
  debugPlayerVerts[3] = newVert;
  //back plane
  //newVert = V3(center.x - offset.x, center.y + offset.y, center.z - offset.z);//4
  newVert = center - halfW.x*axes[0] + halfW.y*axes[1] - halfW.z*axes[2];
  debugPlayerVerts[4] = newVert;
  //newVert = V3(center.x - offset.x, center.y - offset.y, center.z - offset.z);//5
  newVert = center - halfW.x*axes[0] - halfW.y*axes[1] - halfW.z*axes[2];
  debugPlayerVerts[5] = newVert;
  //newVert = V3(center.x + offset.x, center.y - offset.y, center.z - offset.z);//6
  newVert = center + halfW.x*axes[0] - halfW.y*axes[1] - halfW.z*axes[2];
  debugPlayerVerts[6] = newVert;
  //newVert = V3(center.x + offset.x, center.y + offset.y, center.z - offset.z);//7
  newVert = center + halfW.x*axes[0] + halfW.y*axes[1] - halfW.z*axes[2];
  debugPlayerVerts[7] = newVert;
}

GLuint loadTexture(Texture* texture, char* bmpPath) {
  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  texture->data = loadBMP(bmpPath);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->data.width, texture->data.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texture->data.contents);
  if(!freeBMP(&texture->data)) {
    InvalidCodePath; //Failed to free bitmap
  }
  return texID;
}

void loadTextureIntoArray(char* bmpPath, TextureHandle* th, int width, int height) {
  Texture texture;
  texture.data = loadBMP(bmpPath);
  assert(height == texture.data.height);
  assert(width == texture.data.width);
  
  if(!freeBMP(&texture.data)) {
    InvalidCodePath; //Failed to free bitmap
  }
}

GLuint Renderer::createTextureArrayBuffer(int width, int height) {
  assert(width == height);
  int layerCount = 2048/height;
  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texID);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, layerCount);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  return texID;
}

void Renderer::loadTextureArray512(TextureHandle texTable[],int aID, char* aPath, int bID, char* bPath, int cID, char* cPath, int dID, char* dPath){
  GLuint texID = createTextureArrayBuffer(512,512);
  Bitmap aBMP, bBMP, cBMP, dBMP;
  int cpyOffset = 512*512*4;
  char* buffer = (char*)VirtualAlloc(0, 512*512*4*4, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  char* cpyDest = buffer;
  if(buffer && aPath!=NULL) {
    aBMP = loadBMP(aPath);
    memcpy(cpyDest, (char*)aBMP.contents, cpyOffset);
    cpyDest += cpyOffset;
    freeBMP(&aBMP);
    texTable[aID].glTextureNum = texID;
    texTable[aID].texLayer = 0;
  }
  else {
    InvalidCodePath;
  }
  if(bPath!=NULL) {
    bBMP = loadBMP(bPath);
    memcpy(cpyDest, bBMP.contents, cpyOffset);
    cpyDest += cpyOffset;
    freeBMP(&bBMP);
    texTable[bID].glTextureNum = texID;
    texTable[bID].texLayer = 1;
  }
  if(cPath!=NULL) {
    cBMP = loadBMP(cPath);
    memcpy(cpyDest, cBMP.contents, cpyOffset);
    cpyDest += cpyOffset;
    freeBMP(&cBMP);
    texTable[cID].glTextureNum = texID;
    texTable[cID].texLayer = 2;
  }
  if(dPath!=NULL) {
    dBMP = loadBMP(dPath);
    memcpy(cpyDest, dBMP.contents, cpyOffset);
    freeBMP(&dBMP);
    texTable[dID].glTextureNum = texID;
    texTable[dID].texLayer = 3;
  }
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 512, 512, 4, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);
  VirtualFree(buffer, 0, MEM_RELEASE);
}

TextureHandle Renderer::getGLTexID(int texID) {
  TextureHandle th = texTable[texID];
  return th;
}

TextureHandle Renderer::getGLNormTexID(int texID) {
  TextureHandle th = texNormTable[texID];
  return th;
}

void Renderer::addTri(Vertpcnu& a, Vertpcnu& b, Vertpcnu& c) {
  v3 normal;
  v3 first = a.position - b.position;
  v3 second = c.position - b.position;
  normal = normalize(cross(second, first));
  a.normal = normal;
  b.normal = normal;
  c.normal = normal;
  v3 deltaPos1 = b.position-a.position;
  v3 deltaPos2 = c.position-a.position;
  v2 deltaUV1, deltaUV2;
  deltaUV1.x = b.uv.x-a.uv.x;
  deltaUV1.y = b.uv.y-a.uv.y;
  deltaUV2.x = c.uv.x-a.uv.x;
  deltaUV2.y = c.uv.y-a.uv.y;

  float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
  v3 tangent = r*(deltaUV2.y * deltaPos1 - deltaUV1.y * deltaPos2);
  tangent = normalize(tangent);
  v3 bitangent = r*(deltaUV1.x * deltaPos2 - deltaUV2.x * deltaPos1);
  bitangent = normalize(bitangent);

  a.tangent = tangent;
  a.bitangent = bitangent;
  b.tangent = tangent;
  b.bitangent = bitangent;
  c.tangent = tangent;
  c.bitangent = bitangent;

  //debugDrawLine(a.position, a.position + .5*a.normal);
  //debugDrawLine(a.position, a.position + .5*a.tangent);
  //debugDrawLine(a.position, a.position + .5*a.bitangent);
  //debugDrawLine(b.position, b.position + .5*b.normal);
  //debugDrawLine(b.position, b.position + .5*b.tangent);
  //debugDrawLine(b.position, b.position + .5*b.bitangent);
  //debugDrawLine(c.position, c.position + .5*c.normal);
  //debugDrawLine(c.position, c.position + .5*c.tangent);
  //debugDrawLine(c.position, c.position + .5*c.bitangent);

  int index = levelElements.size();
  level.push_back(a);
  levelElements.push_back(index++);
  level.push_back(b);
  levelElements.push_back(index++);
  level.push_back(c);
  levelElements.push_back(index++);
}

void Renderer::debugDrawLine(v3& start, v3& end) {
  int startingIndex = debugBoundingVerts.size();
  debugBoundingVerts.push_back(start);
  debugBoundingVerts.push_back(end);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
}