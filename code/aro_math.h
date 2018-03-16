/*
General purpose games-math library.
Vectors: I didn't want to have class-like structs, and I'm not personally a fan of numerical indexes into vectors. 
Fourth component (w) of v4s is 0 by default
Vector functions receiving values instead of refs is somewhat intentional, since they are small, so the tradeoff of passing by reference is hard to assess. 
Matrices are row-major order.
Points are vectors where the fourth component (w) is 1 by default
*/
#ifndef __ARO_MATH__
#define __ARO_MATH__

#define M_PI  3.14159265358979323846f
#define M_PI_2  3.14159265358979323846f/2.0f

#define degToRad(a)  ((a)*(M_PI/180))
#define radToDeg(a)  ((a)*(180/M_PI))

struct v2 {
  float x,y;
};

struct v2i {
  int x,y;
};

struct v3 {
  float x,y,z;
};

struct v4 {
  float x,y,z,w;
};

struct p3 {
  float x,y,z;
};

struct m4x4 {
    //Row major!
    float n[4][4];
};

int clamp(int in, int min, int max);
float clamp(float in, float min, float max);

//V2 ////////////////////////////////
v2 V2(float X, float Y);
v2 operator+(v2 a, v2 b);
v2 operator-(v2 a, v2 b);
v2 operator*(float a, v2 v);
v2 operator*=(v2 &v, float a);
v2 operator/(v2 v, float a);
v2 operator/=(v2 &v, float a);
float magnitude(const v2& v);
v2 normalize(const v2& v);
v2i V2i(int X, int Y);
v2i operator+(v2i a, v2i b);
v2i operator-(v2i a, v2i b);

//V3 ////////////////////////////////

v3 V3(float X, float Y, float Z);
v3 operator+(v3 a, v3 b);
v3 operator-(v3 a, v3 b);
v3 operator*(float a, v3 v);
v3 operator*=(v3 &v, float a);
v3 operator/(v3 v, float a);
v3 operator/=(v3 &v, float a);
v3 V3(v2 XY, float Z);
float magnitude(const v3& v);
v3 normalize(const v3& v);


//V4 ////////////////////////////////
v4 V4(float X, float Y, float Z, float W);
v4 operator+(v4 a, v4 b);
v4 operator-(v4 a, v4 b);
v4 operator*(float a, v4 v);
v4 operator*=(v4 &v, float a);
v4 operator/(v4 v, float a);
v4 operator/=(v4 &v, float a);
v4 V4(v3 XYZ, float W);
v4 V4(v3 XYZ);


//P3 ////////////////////////////////
p3 P3(float X, float Y, float Z);
v3 operator-(p3 a, p3 b);

//Matrix 4x4 ////////////////////////////////
m4x4 M4x4(float n00,float n01,float n02,float n03,
          float n10, float n11,float n12,float n13,
          float n20, float n21,float n22,float n23,
          float n30, float n31,float n32,float n33);

m4x4 rows4x4(v3 a, v3 b, v3 c);
m4x4 cols4x4(v3 a, v3 b, v3 c);
m4x4 identity(void);
v3 col(m4x4& a, unsigned int c);
v3 row(m4x4& a, unsigned int r);
v4 operator*(m4x4& m, v4 v);
v4 operator*(m4x4& m, v3 v);
v4 operator*(m4x4& m, p3 v);

m4x4 operator*(m4x4& a, m4x4& b);

//Vector operations //////////////////////////////////////
float dot(v2 a, v2 b);
float dot(v3 a, v3 b);
float dot(v4 a, v4 b);
v3 cross(v3 a, v3 b);
v3 project(v3 a, v3 b);

//Transformation matrix functions ///////////////////////////
m4x4 makeRotX(float t);
m4x4 makeRotY(float t);
m4x4 makeRotZ(float t);
m4x4 translate(v3 v);
m4x4 translate(m4x4& m, v3 v);

void getOrthoProjMatrix(const float &b, const float &t, const float &l, const float &r, const float &n, const float &f, m4x4& M);
void aroInfFrustrum(const float &b, const float &t, const float &l, const float &r, const float &n, m4x4& M);
void aroFrustrum(m4x4& M, const float &left, const float &right, const float &bottom, const float &top, const float &znear, const float &zfar);
void aroPerspective(m4x4& M, float fovy, float aspect, float near, float far);
m4x4 aroLookatc(v3* eyePosition, v3* viewCenter);
m4x4 aroLookatb(v3* eyePosition, v3* viewCenter);
m4x4 aroLookat(v3 &eyePosition, v3 &viewCenter);

#endif // __ARO_MATH__