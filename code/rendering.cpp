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

const char* screenQuadVertSrc = R"FOO(
#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;

void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  
)FOO";

const char* screenQuadFragSrc = R"FOO(
#version 450 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D screenTexture;

void main(){
    vec3 col = texture(screenTexture, TexCoords).rgb;

    FragColor = vec4(col, 1.0);
} 
)FOO";


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
uniform vec3 globalLight;

out vec3 fragPos;
out vec4 fragPosLightspace;
out vec3 texCoordOutFromVert;
out vec3 tangentLightPos;
out vec3 tangentViewPos;
out vec3 tangentFragPos;
out vec3 lightPos;
out vec3 tangentDirectionalLight;
out vec3 plainNormalOut;

void main()
{
  gl_Position =  projection * view * vec4(position.x, position.y, position.z, 1.0f);
  fragPos = position.xyz;
  texCoordOutFromVert = texCoordIn;
  lightPos  = globalLight;
  
  vec3 T = normalize(tangentIn);
  vec3 B = normalize(bitangentIn);
  vec3 N = normalize(normal);
  mat3 TBN = transpose(mat3(T, B, N));
  tangentLightPos = TBN * lightPos;
  tangentViewPos = TBN * playerPos;
  tangentFragPos = TBN * fragPos;
  tangentDirectionalLight = TBN * normalize(lightPos);
  fragPosLightspace =  lightSpaceMatrix * vec4(fragPos, 1.0);
  plainNormalOut = normal;
}
)FOO";

const char* fragmentSource = R"FOO(
#version 450 core
uniform sampler2DArray textureData;
uniform sampler2DArray bumpMap;

uniform sampler2D depthMap;
uniform samplerCube depthCubeMap;
uniform float far_plane;

in vec3 fragPos;
in vec3 texCoordOutFromVert;
in vec3 tangentLightPos;
in vec3 tangentViewPos;
in vec3 tangentFragPos;
in vec3 lightPos;
in vec3 tangentDirectionalLight;
in vec4 fragPosLightspace;
in vec3 plainNormalOut;

layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};

out vec4 resultColor;

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);  

float shadowCubeCalculation(vec3 fragPos) {
  // get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    // use the light to fragment vector to sample from the depth map    
    //float closestDepth = texture(depthCubeMap, fragToLight).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    //closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // now test for shadows
    
    float shadow = 0.0;
    float bias   = 0.15;
    int samples  = 20;
    float viewDistance = length(playerPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
      float closestDepth = texture(depthCubeMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
      closestDepth *= far_plane;   // Undo mapping [0;1]
      if(currentDepth - bias > closestDepth)
        shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}

void main()
{
  vec3 normal = texture(bumpMap, texCoordOutFromVert.xyz).rgb;
  normal = normalize(normal * 2.0 - 1.0);
  vec3 texColor = texture(textureData, vec3(texCoordOutFromVert.xyz)).rgb;
  
  vec3 lightColor = vec3(0.9);

  //attenuated for light distance of 160
  float distance = length(lightPos - fragPos);
  float attenuation = 1.0 / (1.0 + 0.14 * distance +  0.07 * (distance * distance));   

  // ambient
  vec3 ambient = 0.1 * texColor;
  // diffuse
  vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
  float diff = max(dot(lightDir, normal), 0.0);
  vec3 diffuse = diff * lightColor * texColor;
  // specular
  vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
  vec3 reflectDir = reflect(-lightDir, normal);
  vec3 halfwayDir = normalize(lightDir + viewDir);  
  float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
  vec3 specular = 0.2 * spec * lightColor;    
  float shadow = shadowCubeCalculation(fragPos);
  attenuation = 1.0;
  ambient  *= attenuation; 
  diffuse  *= attenuation;
  specular *= attenuation;   

resultColor = vec4((ambient + (diffuse + specular) * (1-shadow)), 1.0);


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

const char* depthCubeMapVertSource = R"FOO(
#version 450 core
layout (location = 0) in vec3 aPos;

//uniform mat4 model;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}  
)FOO";

const char* depthCubeMapGeoSource = R"FOO(
#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main() {
  for(int face = 0; face < 6; ++face) {
    gl_Layer = face; // built-in variable that specifies to which face we render.
    for(int i = 0; i < 3; ++i) {
      FragPos = gl_in[i].gl_Position;
      gl_Position = shadowMatrices[face] * FragPos;
      EmitVertex();
    }    
    EndPrimitive();
  }
}  
)FOO";

const char* depthCubeMapFragSource= R"FOO(
#version 450 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(FragPos.xyz - lightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / far_plane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
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

void compileShader(GLuint* shaderID, const char* vSrc, const char* fSrc, const char* gSrc = NULL) {
  GLuint vertShader, fragShader, geoShader;
  int success;
  
  vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertShader, 1, &vSrc, NULL);
  glCompileShader(vertShader);
  
  // print compile errors if any
  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    char buffer[512];
    glGetShaderInfoLog(vertShader, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }
  
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

  if(gSrc != NULL) {
    geoShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geoShader, 1, &gSrc, NULL);
    glCompileShader(geoShader);
    // print compile errors if any
    glGetShaderiv(geoShader, GL_COMPILE_STATUS, &success);
    if(!success) {
      char buffer[512];
      glGetShaderInfoLog(geoShader, 512, NULL, buffer);
      OutputDebugString(buffer);
      InvalidCodePath;
    }    
  }
  
  // shader Program
  *shaderID = glCreateProgram();
  glAttachShader(*shaderID, vertShader);
  glAttachShader(*shaderID, fragShader);
  if(gSrc != NULL) {
    glAttachShader(*shaderID, geoShader);
  }
  glLinkProgram(*shaderID);
  
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
  if(gSrc != NULL) {
    glDeleteShader(geoShader);
  }
}

void Renderer::initialize() {
  float quadVertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
  // positions   // texCoords
  -1.0f,  1.0f,  0.0f, 1.0f,
  -1.0f, -1.0f,  0.0f, 0.0f,
  1.0f, -1.0f,  1.0f, 0.0f,

  -1.0f,  1.0f,  0.0f, 1.0f,
  1.0f, -1.0f,  1.0f, 0.0f,
  1.0f,  1.0f,  1.0f, 1.0f
  };
  glGenFramebuffers(1, &mainFrameBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0); //screen quad corners
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1); //screen quad UVs
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

  compileShader(&shaderID, vertexSourceUBUF, fragmentSource);
  compileShader(&screenShaderID, screenQuadVertSrc, screenQuadFragSrc);
  GLuint uniformBlockIndex = glGetUniformBlockIndex(shaderID, "GlobalMatrices");
  glUniformBlockBinding(shaderID, uniformBlockIndex, 0);
  compileShader(&shadowShaderID, depthCubeMapVertSource, depthCubeMapFragSource, depthCubeMapGeoSource);
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
  GLint texCoord = glGetAttribLocation(shaderID, "texCoordIn");
  glVertexAttribPointer(texCoord, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*3));
  glEnableVertexAttribArray(texCoord);

  textureLocation = glGetUniformLocation(shaderID, "textureData");
  normTextureLocation = glGetUniformLocation(shaderID, "bumpMap");
  depthMapTextureLocation = glGetUniformLocation(shaderID, "depthMap");
  depthCubeMapLocation = glGetUniformLocation(shaderID, "depthCubeMap");

  glUseProgram(shaderID);
  glUniform1i(textureLocation, 0);
  glUniform1i(normTextureLocation, 1);
  glUniform1i(depthMapTextureLocation, 2);
  GLenum error = glGetError();
  glUniform1i(depthCubeMapLocation, 3);
  
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
    debugPlayerElements.push_back(4);\
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
  
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);  

  pointShadowProjMatFarPlane = 30.0f;
  aroPerspective(pointShadowProjMat, 90.0f, SHADOW_ASPECT, 0.5f, pointShadowProjMatFarPlane);
  pointShadowProjMat = transpose(pointShadowProjMat);
  //end shadowmap setup

  addPointLight(&pointLights, globalLight);
  //addPointLight(&pointLights, globalLight);

  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

// MSAA setup
  quadFramebuffer = 0;
  numMSAASamples = 4;

  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer); 

  glGenTextures(1, &multisampleTextureLocation);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampleTextureLocation);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numMSAASamples, GL_RGB, windowDimensions.width, windowDimensions.height, GL_TRUE);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, multisampleTextureLocation, 0);
  
  glGenRenderbuffers(1, &multisampleRBO);
  glBindRenderbuffer(GL_RENDERBUFFER, multisampleRBO);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, numMSAASamples, GL_DEPTH24_STENCIL8, windowDimensions.width, windowDimensions.height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, multisampleRBO);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    InvalidCodePath;

  //post-processing framebuffer
  glGenFramebuffers(1, &intermediateFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
  // create a color attachment texture
  
  glGenTextures(1, &screenQuadTexture);
  glBindTexture(GL_TEXTURE_2D, screenQuadTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowDimensions.width, windowDimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenQuadTexture, 0);  // we only need a color buffer

  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);

  glUseProgram(screenShaderID);
  glUniform1i(glGetUniformLocation(screenShaderID, "screenTexture"), 0);

  glUseProgram(shaderID);
  wglSwapInterval(1);
}

void Renderer::reconfigureMSAA() {
  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer); 

  glGenTextures(1, &multisampleTextureLocation);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampleTextureLocation);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numMSAASamples, GL_RGB, windowDimensions.width, windowDimensions.height, GL_TRUE);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, multisampleTextureLocation, 0);
  
  glGenRenderbuffers(1, &multisampleRBO);
  glBindRenderbuffer(GL_RENDERBUFFER, multisampleRBO);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, numMSAASamples, GL_DEPTH24_STENCIL8, windowDimensions.width, windowDimensions.height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, multisampleRBO);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    InvalidCodePath;

  //post-processing framebuffer
  glGenFramebuffers(1, &intermediateFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
  // create a color attachment texture
  
  glGenTextures(1, &screenQuadTexture);
  glBindTexture(GL_TEXTURE_2D, screenQuadTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowDimensions.width, windowDimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenQuadTexture, 0);  // we only need a color buffer

  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);

  glUseProgram(screenShaderID);
  multisampleTextureLocation = glGetUniformLocation(screenShaderID, "screenTexture");
  glUniform1i(multisampleTextureLocation, 0);
}
// 3.2 0.5 -1.6
void Renderer::renderPointLights(PointLightArray* plr){
  glUseProgram(shadowShaderID);
  for(int i = 0; i<plr->numLights; i++){
    GLuint lightTexid = plr->lightTexIds[i];
    GLuint lightFBOid = plr->lightFBOIds[i];
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, lightFBOid);
    int shadowTransBase =  i*6;
    m4x4 shadowProjView;
    v3 lightPos = plr->positions[i];
    shadowProjView = pointShadowProjMat*transpose(aroLookat(lightPos, lightPos + V3(1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f)));
    plr->perspViews[shadowTransBase] = shadowProjView;
    shadowProjView = pointShadowProjMat*transpose(aroLookat(lightPos, lightPos + V3(-1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f)));
    plr->perspViews[shadowTransBase+1] = shadowProjView;
    shadowProjView = pointShadowProjMat*transpose(aroLookat(lightPos, lightPos + V3(0.0f, 1.0f, 0.0f), V3(0.0f, 0.0f, 1.0f)));
    plr->perspViews[shadowTransBase+2] = shadowProjView;
    shadowProjView = pointShadowProjMat*transpose(aroLookat(lightPos, lightPos + V3(0.0f, -1.0f, 0.0f), V3(0.0f, 0.0f, -1.0f)));
    plr->perspViews[shadowTransBase+3] = shadowProjView;
    shadowProjView = pointShadowProjMat*transpose(aroLookat(lightPos, lightPos + V3(0.0f, 0.0f, 1.0f), V3(0.0f, -1.0f, 0.0f)));
    plr->perspViews[shadowTransBase+4] = shadowProjView;
    shadowProjView = pointShadowProjMat*transpose(aroLookat(lightPos, lightPos + V3(0.0f, 0.0f, -1.0f), V3(0.0f, -1.0f, 0.0f)));
    plr->perspViews[shadowTransBase+5] = shadowProjView;
    glUniformMatrix4fv(glGetUniformLocation(shadowShaderID, "shadowMatrices"), 6*pointLights.numLights, GL_FALSE, &pointLights.perspViews[0].n[0][0]);
    
    GLint shadowPosAttrib = glGetAttribLocation(shadowShaderID, "aPos");
    glVertexAttribPointer(shadowPosAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
    GLuint fp = glGetUniformLocation(shadowShaderID, "far_plane");
    glUniform1f(fp, pointShadowProjMatFarPlane); 
    GLuint lp = glGetUniformLocation(shadowShaderID, "lightPos");
    glUniform3fv(lp, pointLights.numLights, (GLfloat*) &pointLights.positions[0]);
    
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, levelElements.size(), GL_UNSIGNED_INT, 0);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);  
}

void Renderer::addPointLight(PointLightArray* array, v3 position) {
  array->numLights++;
  int n = array->numLights-1;
  array->lightFBOIds.push_back(0);
  array->lightTexIds.push_back(0);
  array->positions.push_back(position);
  m4x4 iden = identity();
  array->perspViews.insert(array->perspViews.end(), 6, iden);
  glGenFramebuffers(1, &array->lightFBOIds[n]);
  glGenTextures(1, &array->lightTexIds[n]);
  glBindTexture(GL_TEXTURE_CUBE_MAP, array->lightTexIds[n]);
  for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glBindFramebuffer(GL_FRAMEBUFFER, array->lightFBOIds[n]);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, array->lightTexIds[n], 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  //diagnoseFramebuffer(array->lightFBOIds[n]);
  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);  
}

void Renderer::draw() {
  glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, mainFrameBuffer);  

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  //Render for directional light shadow map
  //glCullFace(GL_FRONT);
  /*
  m4x4 lightProjection;
  getOrthoProjMatrix(-10.0f, 10.0f, -10.0f, 10.0f, .25f, 20.0f, lightProjection);
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
  */
  //point light render
  glUseProgram(shadowShaderID);
  renderPointLights(&pointLights);

  //Normal render
  //glCullFace(GL_BACK);
  glClearColor(1.0,0.0,1.0,1.0);
  glViewport(0, 0, windowDimensions.width, windowDimensions.height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(shaderID);

//glEnable(GL_FRAMEBUFFER_SRGB); 
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  //lightSpaceMatrixAttrib = glGetUniformLocation(shaderID, "lightSpaceMatrix");
  //glUniformMatrix4fv(lightSpaceMatrixAttrib, 1, GL_FALSE, (GLfloat*) lightSpaceMatrix.n);
  glUniform1f(glGetUniformLocation(shaderID, "far_plane"), pointShadowProjMatFarPlane); 
  GLint globalLightAttrib = glGetUniformLocation(shaderID, "globalLight");
  glUniform3fv(globalLightAttrib, 1, (GLfloat*) &globalLight);

  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
  glEnableVertexAttribArray(posAttrib);

  GLint normalAttrib = glGetAttribLocation(shaderID, "normal");
  glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*2));
  glEnableVertexAttribArray(normalAttrib);

  GLint texCoord = glGetAttribLocation(shaderID, "texCoordIn");
  glVertexAttribPointer(texCoord, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*3));
  glEnableVertexAttribArray(texCoord);

  GLint tangent = glGetAttribLocation(shaderID, "tangentIn");
  glVertexAttribPointer(tangent, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*4));
  glEnableVertexAttribArray(tangent);

  GLint bitangent = glGetAttribLocation(shaderID, "bitangentIn");
  glVertexAttribPointer(bitangent,3 , GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*5));
  glEnableVertexAttribArray(bitangent);

  glActiveTexture(GL_TEXTURE0 + 0); 
  glBindTexture(GL_TEXTURE_2D_ARRAY, texTable[1].glTextureNum);
  glActiveTexture(GL_TEXTURE0 + 1); 
  glBindTexture(GL_TEXTURE_2D_ARRAY, texNormTable[1].glTextureNum);
  glActiveTexture(GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, depthMap);

  glActiveTexture(GL_TEXTURE0+3);
  glBindTexture(GL_TEXTURE_CUBE_MAP, pointLights.lightTexIds[0]);

  //If we want to render the scene from the light's perspective:
  //glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(m4x4), lightView.n);
  //glBufferSubData(GL_UNIFORM_BUFFER, sizeof(m4x4), sizeof(m4x4), lightProjection.n);

  //glCopyImageSubData(pointLights.lightTexIds[0], GL_TEXTURE_CUBE_MAP, 0, 0, 0, 5, 
  //  depthMap, GL_TEXTURE_2D, 0, 0, 0, 0, SHADOW_WIDTH, SHADOW_HEIGHT, 1);
  
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

  // 2. blit to normal colorbuffer of intermediate FBO
  glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFrameBuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
  glBlitFramebuffer(0, 0, windowDimensions.width, windowDimensions.height, 0, 0, windowDimensions.width, windowDimensions.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // 3. now render quad with scene's visuals as its texture image
  glBindFramebuffer(GL_FRAMEBUFFER, quadFramebuffer);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);

  // draw Screen quad
  glUseProgram(screenShaderID);
  glBindVertexArray(quadVAO);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, screenQuadTexture); // use the now resolved color attachment as the quad's texture
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glEnable(GL_DEPTH_TEST);
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
  //NOTE: this is the old, non-array way of doing this
  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
  //Note: currently using 4 mipmap levels
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 4, GL_RGBA8, width, height, layerCount);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
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
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
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

void Renderer::debugDrawLine(const v3& start, const v3& end) {
  int startingIndex = debugBoundingVerts.size();
  debugBoundingVerts.push_back(start);
  debugBoundingVerts.push_back(end);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
}
