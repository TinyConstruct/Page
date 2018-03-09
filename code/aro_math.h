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
//TODO: Sigh...add all of the prototypes here.
int clamp(int in, int min, int max);
v3 V3(float X, float Y, float Z);

void getOrthoProjMatrix(const float &b, const float &t, const float &l, const float &r,const float &n, const float &f, m4x4& M);
void aroInfFrustrum( const float &b, const float &t, const float &l, const float &r, const float &n, m4x4& M);
void aroFrustrum(const float &b, const float &t, const float &l, const float &r,const float &n, const float &f, m4x4& M);
v3 cross(v3 a, v3 b);
float magnitude(const v3& v);
v3 normalize(const v3& v);
v3 operator/(v3 v, float a);

#endif // __ARO_MATH__