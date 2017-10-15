#ifndef IVAN_MATH_H
#define IVAN_MATH_H

#include <math.h>
#include <stdint.h>

#ifndef IVAN_MATH_LERP
#define IVAN_MATH_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#endif

#ifndef IVAN_MATH_MIN
#define IVAN_MATH_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef IVAN_MATH_MAX
#define IVAN_MATH_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef IVAN_MATH_ABS
#define IVAN_MATH_ABS(value) ((value) >= 0 ? (value) : -(value))
#endif

#ifndef IVAN_MATH_SIGN
#define IVAN_MATH_SIGN(value) (((value) >= 0) ? (1) : -(1))
#endif

#ifndef IVAN_MATH_CLAMP
#define IVAN_MATH_CLAMP(value, lower, upper) (IVAN_MATH_MIN(IVAN_MATH_MAX(value, lower), upper))
#endif

#ifndef IVAN_MATH_CLAMP01
#define IVAN_MATH_CLAMP01(value) (IVAN_MATH_CLAMP(value, 0, 1))
#endif

union vec2{
	struct{
		float x, y;
	};
	float E[2];
};

union vec3{
    struct{
        float x, y, z;
    };
    struct{
        float r, g, b;
    };
    vec2 xy;
    float E[3];
};

union vec4{
    struct{ float x, y, z, w;};
    struct{ vec3 xyz; float w; };
    struct{ float A, B, C, D;};
    struct{
        union{
            vec3 rgb;
            struct{
                float r, g, b;
            };
        };
        float a; 
    };
    struct{ vec2 xy; vec2 zw; };
    float E[4];
};

struct quat{
	union{
		struct {
			float x, y, z, w;
		};
		vec3 xyz;
		vec4 xyzw;
	};
};

struct rectangle2{
	vec2 Min;
	vec2 Max;
};

struct rectangle2i{
	int32 MinX, MinY;
	int32 MaxX, MaxY;
};

struct rectangle3{
	vec3 Min;
	vec3 Max;
};

struct mat4{
	union{
		float E[16];
		vec4 Rows[4];
	};
};

#ifndef IVAN_MATH_CONSTANTS
#define IVAN_MATH_EPSILON 1.19209290e-7f
#define IVAN_MATH_ZERO 0.0f
#define IVAN_MATH_ONE 1.0f
#define IVAN_MATH_TWO_THIRDS 0.666666666666666666666666666666666666667f

#define IVAN_MATH_TAU 6.28318530717958647692528676655900576f
#define IVAN_MATH_PI 3.14159265358979323846264338327950288f
#define IVAN_MATH_ONE_OVER_TAU 0.636619772367581343075535053490057448f
#define IVAN_MATH_ONE_OVER_PI 0.318309886183790671537767526745028724f
#define IVAN_MATH_PI_OVER_180 0.017453292519943295769236907684886127f

#define IVAN_MATH_TAU_OVER_2 3.14159265358979323846264338327950288f
#define IVAN_MATH_TAU_OVER_4 1.570796326794896619231321691639751442f
#define IVAN_MATH_TAU_OVER_8 0.785398163397448309615660845819875721f

#define IVAN_MATH_E 2.71828182845904523536f
#define IVAN_MATH_SQRT_TWO 1.41421356237309504880168872420969808f
#define IVAN_MATH_ONE_OVER_SQRT_TWO 0.7071067811865475244008443621048490392f
#define IVAN_MATH_SQRT_THREE 1.73205080756887729352744634150587236f
#define IVAN_MATH_SQRT_FIVE 2.23606797749978969640917366873127623f

#define IVAN_MATH_LOG_TWO 0.693147180559945309417232121458176568f
#define IVAN_MATH_LOG_TEN 2.30258509299404568401799145468436421f

#define IVAN_DEG_TO_RAD 0.0174532925f
#define IVAN_RAD_TO_DEG 57.2958f

#define IVAN_MATH_CONSTANTS
#endif

/*Simple operations*/
inline float Sqrt(float Value){
	float Result;
	Result = sqrtf(Value);
	return(Result);
}

inline float RSqrt(float Value){
	float Result;
	Result = 1.0f / sqrtf(Value);
	return(Result);
}

inline float Sin(float Rad){
	float Result = sinf(Rad);
	return(Result);
}

inline float Cos(float Rad){
	float Result = cosf(Rad);
	return(Result);
}

inline float Tan(float Rad){
	float Result = tanf(Rad);
	return(Result);
}

inline float ASin(float Value){
	float Result = asinf(Value);
	return(Result);
}

inline float ACos(float Value){
	float Result = acosf(Value);
	return(Result);
}

inline float ATan(float Value){
	float Result = atan(Value);
	return(Result);
}

inline float ATan2(float Y, float X){
	float Result = atan2f(Y, X);
	return(Result);
}

inline float Exp(float Value){
	float Result = expf(Value);
	return(Result);
}

inline float Log(float Value){
	float Result = logf(Value);
	return(Result);
}

inline float Pow(float a, float b){
	float Result = powf(a, b);
	return(Result);
}

/*vec2 constructors*/
inline vec2 Vec2(float Value){
	vec2 Result;
	Result.x = Value;
	Result.y = Value;
	return(Result);
}

inline vec2 Vec2(float x, float y){
	vec2 Result;
	Result.x = x;
	Result.y = y;
	return(Result);
}

/*vec3 constructors*/
inline vec3 Vec3(float Value){
	vec3 Result;
	Result.x = Value;
	Result.y = Value;
	Result.z = Value;
	return(Result);
}

inline vec3 Vec3(float x, float y, float z){
	vec3 Result;
	Result.x = x;
	Result.y = y;
	Result.z = z;
	return(Result);
}

inline vec3 Vec3(vec2 InitVector, float z){
	vec3 Result;
	Result.x = InitVector.x;
	Result.y = InitVector.y;
	Result.z = z;
	return(Result);
}

/*vec4 constructors*/
inline vec4 Vec4(float Value){
	vec4 Result;
	Result.x = Value;
	Result.y = Value;
	Result.z = Value;
	Result.w = Value;
	return(Result);
}

inline vec4 Vec4(float x, float y, float z, float w){
	vec4 Result;
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;
	return(Result);
}

inline vec4 Vec4(vec3 InitVector, float w){
	vec4 Result;
	Result.x = InitVector.x;
	Result.y = InitVector.y;
	Result.z = InitVector.z;
	Result.w = w;
	return(Result);
}

/*Quaternion constructors and operations*/
inline quat Quat(vec3 axis, float theta){
	quat Result;

	float HalfTheta = theta * 0.5f;
	float SinScalar = Sin(HalfTheta);
	float OneOverAxisLen = 1.0f / Sqrt((axis.x * axis.x + axis.y * axis.y + axis.z * axis.z));
	Result.x = OneOverAxisLen * axis.x * SinScalar;
	Result.y = OneOverAxisLen * axis.y * SinScalar;
	Result.z = OneOverAxisLen * axis.z * SinScalar;
	Result.w = Cos(HalfTheta);

	return(Result);
}

inline quat Quat(float x, float y, float z, float theta){
	quat Result;

	float HalfTheta = theta * 0.5f;
	float SinScalar = Sin(HalfTheta);
	float OneOverAxisLen = 1.0f / Sqrt((x * x + y * y + z * z));
	Result.x = x * OneOverAxisLen * SinScalar;
	Result.y = y * OneOverAxisLen * SinScalar;
	Result.z = z * OneOverAxisLen * SinScalar;
	Result.w = Cos(HalfTheta);

	return(Result);
}

inline quat Mul(quat q1, quat q2){
    quat q;
    q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    q.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    q.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    return(q);
}

inline quat Mul(quat Q, float s){
	quat Result;

	Result.x = Q.x * s;
	Result.y = Q.y * s;
	Result.z = Q.z * s;
	Result.w = Q.w * s;

	return(Result);
}

inline quat Quat(float Yaw, float Pitch, float Roll){
	quat q1 = Quat(1.0f, 0.0f, 0.0f, Pitch);
	quat q2 = Quat(0.0f, 1.0f, 0.0f, Yaw);
	quat q3 = Quat(0.0f, 0.0f, 1.0f, Roll);

	quat Result = Mul(q2, q1);
	Result = Mul(Result, q3);
	return(Result);
}

inline quat QuatIdentity(){
	quat Result;

	Result.x = 0.0f;
	Result.y = 0.0f;
	Result.z = 0.0f;
	Result.w = 1.0f;

	return(Result);
}

inline quat Conjugate(quat q){
	quat Result;

	Result.x = -q.x;
	Result.y = -q.y;
	Result.z = -q.z;
	Result.w = q.w;

	return(Result);
}

inline float Dot(quat q1, quat q2){
	float Result = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

	return(Result);
}

inline quat Add(quat A, quat B){
	quat Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	Result.w = A.w + B.w;

	return(Result);
}

inline quat Sub(quat A, quat B){
	quat Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	Result.w = A.w - B.w;

	return(Result);
}

inline quat Div(quat q, float s){
	quat Result;
	float OneOverS = 1.0f / s;
	Result.x = q.x * OneOverS;
	Result.y = q.y * OneOverS;
	Result.z = q.z * OneOverS;
	Result.w = q.w * OneOverS;

	return(Result);
}

inline float Length(quat q){
	float Result = Sqrt(Dot(q, q));
	return(Result);
}

inline quat Normalize(quat q){
	quat Result;

	float Len = Length(q);
	float OneOverLen = 1.0f / Len;

	Result.w = q.w * OneOverLen;
	Result.x = q.x * OneOverLen;
	Result.y = q.y * OneOverLen;
	Result.z = q.z * OneOverLen;

	return(Result);
}

inline quat Inverse(quat q){
	quat Result;

	Result = Conjugate(q);
	Result = Div(Result, Dot(q, q));

	return(Result);
}

inline mat4 RotationMatrix(quat Q){
	
	mat4 Result;

	float xy = Q.x * Q.y;
	float xz = Q.x * Q.z;
	float xw = Q.x * Q.w;
	float yz = Q.y * Q.z;
	float yw = Q.y * Q.w;
	float zw = Q.z * Q.w;
	float xSquared;
	float ySquared;
	float zSquared;

	Result.E[0] = 1.0f - 2.0f * (ySquared * zSquared);
	Result.E[1] = 2.0f * (xy - zw);
	Result.E[2] = 2.0f * (xz + yw);
	Result.E[3] = 0.0f;

	Result.E[4] = 2.0f * (xy + zw);
	Result.E[5] = 1.0f - 2.0f * (xSquared + zSquared);
	Result.E[6] = 2.0f * (yz - xw);
	Result.E[7] = 0.0f;

	Result.E[8] = 2.0f * (xz - yw);
	Result.E[9] = 2.0f * (yz + xw);
	Result.E[10] = 1.0f - 2.0f * (xSquared + ySquared);
	Result.E[11] = 0.0f;

	Result.E[8] = 0.0f;
	Result.E[8] = 0.0f;
	Result.E[8] = 0.0f;
	Result.E[8] = 1.0f;

	return(Result);
}

/*Add operation*/
inline vec2 Add(vec2 a, vec2 b){
	a.x += b.x;
	a.y += b.y;
	return(a);
}

inline vec3 Add(vec3 a, vec3 b){
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return(a);
}

inline vec4 Add(vec4 a, vec4 b){
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return(a);
}

/*Subtract operation*/
inline vec2 Sub(vec2 a, vec2 b){
	a.x -= b.x;
	a.y -= b.y;
	return(a);
}

inline vec3 Sub(vec3 a, vec3 b){
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return(a);
}

inline vec4 Sub(vec4 a, vec4 b){
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return(a);
}

/*Multiply operation*/
inline vec2 Mul(vec2 a, float s){
	a.x *= s;
	a.y *= s;
	return(a);
}

inline vec3 Mul(vec3 a, float s){
	a.x *= s;
	a.y *= s;
	a.z *= s;
	return(a);
}

inline vec4 Mul(vec4 a, float s){
	a.x *= s;
	a.y *= s;
	a.z *= s;
	a.w *= s;
	return(a);
}

/*Divide operation*/
inline vec2 Div(vec2 a, float s){
	float OneOverS = 1.0f / s;
	a.x *= OneOverS;
	a.y *= OneOverS;
	return(a);
}

inline vec3 Div(vec3 a, float s){
	float OneOverS = 1.0f / s;
	a.x *= OneOverS;
	a.y *= OneOverS;
	a.z *= OneOverS;
	return(a);
}

inline vec4 Div(vec4 a, float s){

	float OneOverS = 1.0f / s;
	a.x *= OneOverS;
	a.y *= OneOverS;
	a.z *= OneOverS;
	a.w *= OneOverS;
	return(a);
}

/*vec2 operator overloading*/
inline bool operator==(vec2 a, vec2 b) { return((a.x == b.x) && (a.y == b.y)); }
inline bool operator!=(vec2 a, vec2 b){ return((a.x != b.x) || (a.y != b.y)); }

inline vec2 operator+(vec2 a){ return(a); }
inline vec2 operator-(vec2 a){ vec2 r = { -a.x, -a.y }; return(r); }

inline vec2 operator+(vec2 a, vec2 b){ return Add(a, b); }
inline vec2 operator-(vec2 a, vec2 b){ return Sub(a, b); }

inline vec2 operator*(vec2 a, float s){ return Mul(a, s); }
inline vec2 operator*(float s, vec2 a){ return Mul(a, s); }
inline vec2 operator/(vec2 a, float s){ return Div(a, s); }

inline vec2 operator*(vec2 a, vec2 b){vec2 r = {a.x * b.x, a.y * b.y}; return(r);}
inline vec2 operator/(vec2 a, vec2 b){vec2 r = {a.x / b.x, a.y / b.y}; return(r);}

inline vec2 &operator+=(vec2& a, vec2 b){return(a = a + b);}
inline vec2 &operator-=(vec2& a, vec2 b){return(a = a - b);}
inline vec2 &operator*=(vec2& a, float s){return(a = a * s);}
inline vec2 &operator/=(vec2& a, float s){return(a = a / s);}

/*vec3 operator overloading*/
inline bool operator==(vec3 a, vec3 b){return((a.x == b.x) && (a.y == b.y) && (a.z == b.z));}
inline bool operator!=(vec3 a, vec3 b){return((a.x != b.x) || (a.y != b.y) || (a.z != b.z));}

inline vec3 operator+(vec3 a){return(a);}
inline vec3 operator-(vec3 a){vec3 r = {-a.x, -a.y, -a.z}; return(r);}

inline vec3 operator+(vec3 a, vec3 b){ return Add(a, b); }
inline vec3 operator-(vec3 a, vec3 b){ return Sub(a, b); }

inline vec3 operator*(vec3 a, float s){ return Mul(a, s); }
inline vec3 operator*(float s, vec3 a){ return Mul(a, s); }
inline vec3 operator/(vec3 a, float s){ return Div(a, s); }

inline vec3 operator*(vec3 a, vec3 b){vec3 r = {a.x * b.x, a.y * b.y, a.z * b.z}; return(r);}
inline vec3 operator/(vec3 a, vec3 b){vec3 r = {a.x / b.x, a.y / b.y, a.z / b.z}; return(r);}

inline vec3 &operator+=(vec3& a, vec3 b){return(a = a + b);}
inline vec3 &operator-=(vec3& a, vec3 b){return(a = a - b);}
inline vec3 &operator*=(vec3& a, float s){return(a = a * s);}
inline vec3 &operator/=(vec3& a, float s){return(a = a / s);}

/*vec4 operator overloading*/
inline bool operator==(vec4 a, vec4 b) { return((a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w)); }
inline bool operator!=(vec4 a, vec4 b){ return((a.x != b.x) || (a.y != b.y) || (a.z != b.z) || (a.w != b.w)); }

inline vec4 operator+(vec4 a){ return(a); }
inline vec4 operator-(vec4 a){ vec4 r = { -a.x, -a.y }; return(r); }

inline vec4 operator+(vec4 a, vec4 b){ return Add(a, b); }
inline vec4 operator-(vec4 a, vec4 b){ return Sub(a, b); }

inline vec4 operator*(vec4 a, float s){ return Mul(a, s); }
inline vec4 operator*(float s, vec4 a){ return Mul(a, s); }
inline vec4 operator/(vec4 a, float s){ return Div(a, s); }

inline vec4 operator*(vec4 a, vec4 b){vec4 r = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; return(r);}
inline vec4 operator/(vec4 a, vec4 b){vec4 r = {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; return(r);}

inline vec4 &operator+=(vec4& a, vec4 b){return(a = a + b);}
inline vec4 &operator-=(vec4& a, vec4 b){return(a = a - b);}
inline vec4 &operator*=(vec4& a, float s){return(a = a * s);}
inline vec4 &operator/=(vec4& a, float s){return(a = a / s);}

/*quat operator overloading*/
inline quat operator+(quat A, quat B) {Add(A, B);}
inline quat operator-(quat A, quat B) {Sub(A, B);}
inline quat operator*(quat A, quat B) {Mul(A, B);}
inline quat operator*(quat A, float S) {Mul(A, S);}
inline quat operator/(quat A, float S) {Div(A, S);}

/*Dot product*/
inline float Dot(vec2 v0, vec2 v1){ return v0.x * v1.x + v0.y * v1.y; }
inline float Dot(vec3 v0, vec3 v1){ return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z; }
inline float Dot(vec4 v0, vec4 v1){ return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w; }

/*Cross product*/
inline float Cross(vec2 v0, vec2 v1){ return v0.x * v1.y - v1.x * v0.y; }
inline vec3 Cross(vec3 v0, vec3 v1){
    vec3 v;
    v.x = v0.y * v1.z - v1.y * v0.z;
    v.y = v0.z * v1.x - v1.z * v0.x;
    v.z = v0.x * v1.y - v1.x * v0.y;
    return(v);
}

/*Hadamard product*/
inline vec2 Hadamard(vec2 v0, vec2 v1){ return (Vec2(v0.x * v1.x, v0.y * v1.y));}
inline vec3 Hadamard(vec3 v0, vec3 v1){ return (Vec3(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z));}
inline vec4 Hadamard(vec4 v0, vec4 v1){ return (Vec4(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z, v0.w * v1.w));}

/*Magnitude of the vector*/
inline float Magnitude(vec2 v){ return(Sqrt(Dot(v, v)));}
inline float Magnitude(vec3 v){ return(Sqrt(Dot(v, v)));}
inline float Magnitude(vec4 v){ return(Sqrt(Dot(v, v)));}

/*Squared magnitude*/
inline float SqMagnitude(vec2 v){ return(Dot(v, v)); }
inline float SqMagnitude(vec3 v){ return(Dot(v, v)); }
inline float SqMagnitude(vec4 v){ return(Dot(v, v)); }

/*Normalization of the vector*/
inline vec2 Normalize(vec2 v){ return(Mul(v, RSqrt(Dot(v, v)))); }
inline vec3 Normalize(vec3 v){ return(Mul(v, RSqrt(Dot(v, v)))); }
inline vec4 Normalize(vec4 v){ return(Mul(v, RSqrt(Dot(v, v)))); }

inline vec2 Normalize0(vec2 v){ float sqmag = Dot(v, v); return((sqmag) == 0.0f ? Vec2(0.0f) : v * RSqrt(sqmag)); }
inline vec3 Normalize0(vec3 v){ float sqmag = Dot(v, v); return((sqmag) == 0.0f ? Vec3(0.0f) : v * RSqrt(sqmag)); }
inline vec4 Normalize0(vec4 v){ float sqmag = Dot(v, v); return((sqmag) == 0.0f ? Vec4(0.0f) : v * RSqrt(sqmag)); }

/*Reflection of the vector*/
inline vec2 Reflect(vec2 i, vec2 n){return(Sub(i, Mul(n, 2.0f * Dot(n, i))));}
inline vec3 Reflect(vec3 i, vec3 n){return(Sub(i, Mul(n, 2.0f * Dot(n, i))));}

vec2 Refract(vec2 i, vec2 n, float theta){
    vec2 r, a, b;
    float dv, k;
    dv = Dot(n, i);
    k = 1.0f - theta * theta * (1.0f - dv * dv);
    a = Mul(i, theta);
    b = Mul(n, theta * dv * Sqrt(k));
    r = Sub(a, b);
    r = Mul(r, (float)(k >= 0.0f));
    return(r);
}

vec3 Refract(vec3 i, vec3 n, float theta){
    vec3 r, a, b;
    float dv, k;
    dv = Dot(n, i);
    k = 1.0f - theta * theta * (1.0f - dv * dv);
    a = Mul(i, theta);
    b = Mul(n, theta * dv * Sqrt(k));
    r = Sub(a, b);
    r = Mul(r, (float)(k >= 0.0f));
    return(r);
}

float AspectRatio(vec2 v){ return ((v.y < 0.0001f) ? 0.0f : v.x / v.y); }

/*Plane operations*/
inline vec4 NormalizePlane(vec4 Pl){
	vec4 Result;

	float InvMag = 1.0f / Sqrt(Pl.A * Pl.A + Pl.B * Pl.B + Pl.C * Pl.C);
	Result.A = Pl.A * InvMag;
	Result.B = Pl.B * InvMag;
	Result.C = Pl.C * InvMag;
	Result.D = Pl.D * InvMag;

	return(Result);
}

inline float DistanceToPoint(vec4 Plane, vec3 P){
	float Result = Plane.A * P.x + Plane.B * P.y + Plane.C * P.z + Plane.D;

	return(Result);
}

/*Interpolations*/

inline float Lerp(float a, float b, float t){
	float Result = a * (1.0f - t) + b * t;
	return(Result);
}

#define IVAN_VECTOR_LERP(N, a, b, t)	\
	vec##N Res = Sub(b, a);				\
	Res = Mul(Res, t);					\
	Res = Add(Res, a);					\
	return(Res);

inline vec2 Lerp(vec2 a, vec2 b, float delta){ IVAN_VECTOR_LERP(2, a, b, delta); }
inline vec3 Lerp(vec3 a, vec3 b, float delta){ IVAN_VECTOR_LERP(3, a, b, delta); }
inline vec4 Lerp(vec4 a, vec4 b, float delta){ IVAN_VECTOR_LERP(4, a, b, delta); }
#undef IVAN_VECTOR_LERP

inline quat Slerp(quat A, quat B, float Delta){
	quat Result;

	float dot = Dot(A, B);
	float InvDelta = 1.0f - Delta;

	if(dot < 0.0f){
		B = Quat(-B.x, -B.y -B.z, -B.w);
		dot = -dot;
	}

	float k0, k1;
	if(IVAN_MATH_ABS(dot) > 0.9999f){
		k0 = 1.0f - Delta;
		k1 = Delta;
	}
	else{
		float SinTheta = Sqrt(1.0f - dot * dot);

		float Theta = ATan2(SinTheta, dot);
		float OneOverSinTheta = 1.0f / SinTheta;

		k0 = Sin((1 - Delta) * Theta) * OneOverSinTheta;
		k1 = Sin(Delta * Theta) * OneOverSinTheta;
	}

	Result.x = A.x * k0 + B.x * k1;
	Result.y = A.y * k0 + B.y * k1;
	Result.z = A.z * k0 + B.z * k1;
	Result.w = A.w * k0 + B.w * k1;

	return(Result);
}

/*Rectangle Functions*/

inline rectangle2
RectMinMax(vec2 Min, vec2 Max){
	rectangle2 Result;
	Result.Min = Min;
	Result.Max = Max;
	return(Result);
}

inline rectangle2
AddRadiusTo(rectangle2 A, vec2 Radius){
	rectangle2 Result;
	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;
	return(Result);
}

inline rectangle2
Offset(rectangle2 A, vec2 Offset){
	rectangle2 Result;

	Result.Min = A.Min + Offset;
	Result.Max = A.Max + Offset;

	return(Result);
}

inline bool32 
IsInRectangle(rectangle2 Rectangle, vec2 Test){
	bool32 Result = 
		((Test.x >= Rectangle.Min.x) &&
		(Test.y >= Rectangle.Min.y) && 
		(Test.x < Rectangle.Max.x) &&
		(Test.y < Rectangle.Max.y));

	return(Result);
}

inline vec2 GetMinCorner(rectangle2 Rect){
	vec2 Result = Rect.Min;
	return(Result);
}

inline vec2 GetMaxCorner(rectangle2 Rect){
	vec2 Result = Rect.Max;
	return(Result);
}

inline vec2 
GetDim(rectangle2 Rect){
	vec2 Result = Rect.Max - Rect.Min;
	return(Result);
}

inline vec2
GetCenter(rectangle2 Rect){
	vec2 Result = 0.5f * (Rect.Min + Rect.Max);
	return(Result);
}

inline rectangle2
InvertedInfinityRectangle(){
    rectangle2 Result;

    Result.Min.x = Result.Min.y = Real32Maximum;
    Result.Max.x = Result.Max.y = -Real32Maximum;

    return(Result);
}

inline rectangle2
Union(rectangle2 A, rectangle2 B){
	rectangle2 Result;

    Result.Min.x = (A.Min.x < B.Min.x) ? A.Min.x : B.Min.x;
    Result.Min.y = (A.Min.y < B.Min.y) ? A.Min.y : B.Min.y;
    Result.Max.x = (A.Max.x > B.Max.x) ? A.Max.x : B.Max.x;
    Result.Max.y = (A.Max.y > B.Max.y) ? A.Max.y : B.Max.y;

    return(Result);
}

inline int32
GetWidth(rectangle2i A){
	int32 Result = A.MaxX - A.MinX;
	return(Result);
}

inline int32 
GetHeight(rectangle2i A){
	int32 Result = A.MaxY - A.MinY;
	return(Result);
}

inline int32 
GetClampedRectArea(rectangle2 A){
	float Width = (A.Max.x - A.Min.x);
	float Height = (A.Max.y - A.Min.y);

	float Result = 0.0f;
	if((Width > 0) && (Height > 0)){
		Result = Width * Height;
	}

	return(Result);
}

inline rectangle2i
AspectRatioFit(
	uint32 RenderWidth, uint32 RenderHeight,
	uint32 WindowWidth, uint32 WindowHeight)
{
	rectangle2i Result = {};

	if((RenderWidth > 0) &&
		(RenderHeight > 0) && 
		(WindowWidth > 0) && 
		(WindowHeight > 0))
	{
		real32 OptimalWindowWidth = (real32)WindowHeight * ((real32)RenderWidth / (real32)RenderHeight);
		real32 OptimalWindowHeight = (real32)WindowWidth * ((real32)RenderHeight / (real32)RenderWidth);

		if(OptimalWindowWidth > (real32)WindowWidth){
			Result.MinX = 0;
			Result.MaxX = WindowWidth;

			real32 Empty = (real32)WindowHeight - OptimalWindowHeight;
			int32 HalfEmpty = roundf(0.5f * Empty);
			int32 UseHeight = roundf(OptimalWindowHeight);

			Result.MinY = HalfEmpty;
			Result.MaxY = Result.MinY + UseHeight;
		}
		else{
			Result.MinY = 0;
			Result.MaxY = WindowHeight;

			real32 Empty = (real32)WindowWidth - OptimalWindowWidth;
			int32 HalfEmpty = roundf(0.5f * Empty);
			int32 UseWidth = roundf(OptimalWindowWidth);

			Result.MinX = HalfEmpty;
			Result.MaxX = Result.MinX + UseWidth;
		}
	}

	return(Result);
}

/*Matrix 4x4 functions and operators*/
inline mat4 Multiply(mat4 M1, mat4 M2){
	mat4 Result = {};

	Result.E[0] = M1.E[0] * M2.E[0] + M1.E[1] * M2.E[4] + M1.E[2] * M2.E[8] + M1.E[3] * M2.E[12];
	Result.E[1] = M1.E[0] * M2.E[1] + M1.E[1] * M2.E[5] + M1.E[2] * M2.E[9] + M1.E[3] * M2.E[13];
	Result.E[2] = M1.E[0] * M2.E[2] + M1.E[1] * M2.E[6] + M1.E[2] * M2.E[10] + M1.E[3] * M2.E[14];
	Result.E[3] = M1.E[0] * M2.E[3] + M1.E[1] * M2.E[7] + M1.E[2] * M2.E[11] + M1.E[3] * M2.E[15];

	Result.E[4] = M1.E[4] * M2.E[0] + M1.E[5] * M2.E[4] + M1.E[6] * M2.E[8] + M1.E[7] * M2.E[12];
	Result.E[5] = M1.E[4] * M2.E[1] + M1.E[5] * M2.E[5] + M1.E[6] * M2.E[9] + M1.E[7] * M2.E[13];
	Result.E[6] = M1.E[4] * M2.E[2] + M1.E[5] * M2.E[6] + M1.E[6] * M2.E[10] + M1.E[7] * M2.E[14];
	Result.E[7] = M1.E[4] * M2.E[3] + M1.E[5] * M2.E[7] + M1.E[6] * M2.E[11] + M1.E[7] * M2.E[15];

	Result.E[8] = M1.E[8] * M2.E[0] + M1.E[9] * M2.E[4] + M1.E[10] * M2.E[8] + M1.E[11] * M2.E[12];
	Result.E[9] = M1.E[8] * M2.E[1] + M1.E[9] * M2.E[5] + M1.E[10] * M2.E[9] + M1.E[11] * M2.E[13];
	Result.E[10] = M1.E[8] * M2.E[2] + M1.E[9] * M2.E[6] + M1.E[10] * M2.E[10] + M1.E[11] * M2.E[14];
	Result.E[11] = M1.E[8] * M2.E[3] + M1.E[9] * M2.E[7] + M1.E[10] * M2.E[11] + M1.E[11] * M2.E[15];

	Result.E[12] = M1.E[12] * M2.E[0] + M1.E[13] * M2.E[4] + M1.E[14] * M2.E[8] + M1.E[15] * M2.E[12];
	Result.E[13] = M1.E[12] * M2.E[1] + M1.E[13] * M2.E[5] + M1.E[14] * M2.E[9] + M1.E[15] * M2.E[13];
	Result.E[14] = M1.E[12] * M2.E[2] + M1.E[13] * M2.E[6] + M1.E[14] * M2.E[10] + M1.E[15] * M2.E[14];
	Result.E[15] = M1.E[12] * M2.E[3] + M1.E[13] * M2.E[7] + M1.E[14] * M2.E[11] + M1.E[15] * M2.E[15];

	return(Result);
}

inline vec4 Multiply(mat4 M, vec4 V){
	vec4 Result;

	Result.E[0] = V.E[0] * M.E[0] + V.E[0] * M.E[1] + V.E[0] * M.E[2] + V.E[0] * M.E[3];
	Result.E[1] = V.E[1] * M.E[4] + V.E[1] * M.E[5] + V.E[1] * M.E[6] + V.E[1] * M.E[7];
	Result.E[2] = V.E[2] * M.E[8] + V.E[2] * M.E[9] + V.E[2] * M.E[10] + V.E[2] * M.E[11];
	Result.E[3] = V.E[3] * M.E[12] + V.E[3] * M.E[13] + V.E[3] * M.E[14] + V.E[3] * M.E[15];

	return(Result);
}

inline mat4 Identity(){
	mat4 Result;

	Result.Rows[0] = {1.0f, 0.0f, 0.0f, 0.0f};
	Result.Rows[1] = {0.0f, 1.0f, 0.0f, 0.0f};
	Result.Rows[2] = {0.0f, 0.0f, 1.0f, 0.0f};
	Result.Rows[3] = {0.0f, 0.0f, 0.0f, 1.0f};

	return(Result);
}

inline mat4 Transpose(mat4 M){
	mat4 Result;

	for(int RowIndex = 0; RowIndex < 4; RowIndex++){
		for(int ColumtIndex = 0; ColumtIndex < 4; ColumtIndex++){
			Result.E[ColumtIndex * 4 + RowIndex] = M.E[RowIndex * 4 + ColumtIndex];
		}
	}

	return(Result);
}

inline mat4 TranslationMatrix(vec3 Translation){
	mat4 Result = Identity();

	Result.E[3] = Translation.x;
	Result.E[7] = Translation.y;
	Result.E[11] = Translation.z;

	return(Result);
}

inline mat4 RotationMatrix(vec3 R, float Angle){
	mat4 Result;

	float CosT = Cos(Angle);
	float SinT = Sin(Angle);
	float InvCosT = 1.0f - CosT;

	float RxRyInvCos = R.x * R.y * InvCosT;
	float RxRzInvCos = R.x * R.z * InvCosT;
	float RyRzInvCos = R.y * R.z * InvCosT;

	Result.E[0] = CosT + R.x * R.x * InvCosT;
	Result.E[1] = RxRyInvCos - R.z * SinT;
	Result.E[2] = RxRzInvCos + R.y * SinT;
	Result.E[3] = 0;

	Result.E[4] = RxRyInvCos + R.z * SinT;
	Result.E[5] = CosT * R.y * R.y * InvCosT;
	Result.E[6] = RyRzInvCos - R.x * SinT;
	Result.E[7] = 0.0f;

	Result.E[8] = RxRzInvCos - R.y * SinT;
	Result.E[9] = RyRzInvCos + R.x * SinT;
	Result.E[10] = CosT + R.z * R.z * InvCosT;
	Result.E[11] = 0.0f;

	Result.E[12] = 0.0f;
	Result.E[13] = 0.0f;
	Result.E[14] = 0.0f;
	Result.E[15] = 1.0f;

	return(Result);
}

inline mat4 ScalingMatrix(vec3 Scale){
	mat4 Result = Identity();
	Result.E[0] = Scale.x;
	Result.E[5] = Scale.y;
	Result.E[10] = Scale.z;

	return(Result);
}

inline mat4 Translate(mat4 M, vec3 P){
	mat4 Result = M;

	Result.E[3] += P.x;
	Result.E[7] += P.y;
	Result.E[11] += P.z;

	return(Result);
}

inline mat4 operator*(mat4 M1, mat4 M2){
	return(Multiply(M1, M2));
}

inline vec4 operator*(mat4 M1, vec4 V){
	return(Multiply(M1, V));
}

inline mat4 LookAt(vec3 Pos, vec3 TargetPos, vec3 WorldUp){
	mat4 Result;

	vec3 Fwd = TargetPos - Pos;
	Fwd = Normalize0(Fwd);

	vec3 Left = Normalize(Cross(WorldUp, Fwd));
	vec3 Up = Normalize(Cross(Fwd, Left));

	vec3 Eye = Pos;

	Result.E[0] = Left.x;
	Result.E[1] = Left.y;
	Result.E[2] = Left.z;
	Result.E[3] = -Dot(Left, Eye);

	Result.E[4] = Up.x;
	Result.E[5] = Up.y;
	Result.E[6] = Up.z;
	Result.E[7] = -Dot(Up, Eye);

	Result.E[8] = -Fwd.x;
	Result.E[9] = -Fwd.y;
	Result.E[10] = -Fwd.z;
	Result.E[11] = Dot(Fwd, Eye);

	Result.E[12] = 0.0f;
	Result.E[13] = 0.0f;
	Result.E[14] = 0.0f;
	Result.E[15] = 1.0f;

	return(Result);
}

inline mat4 PerspectiveProjection(uint32 Width, uint32 Height, float FOV, float Far, float Near)
{
	mat4 Result = {};

	float AspectRatio = (float)Width / (float)Height;

	float S = 1.0f / (Tan(FOV * 0.5f * IVAN_DEG_TO_RAD));
	float A = S / AspectRatio;
	float B = S;
	float OneOverFarMinusNear = 1.0f / (Far - Near);
	Result.E[0] = A;
	Result.E[5] = B;
	Result.E[10] = -(Far + Near) * OneOverFarMinusNear;
	Result.E[11] = -(2.0f * Far * Near) * OneOverFarMinusNear;
	Result.E[14] = -1.0f;

	return(Result);
}

inline mat4 OrthographicProjection(
	uint32 Right, uint32 Left,
	uint32 Top, uint32 Bottom,
	float Far, float Near)
{
	mat4 Result = {};

	float OneOverRmL = 1.0f / ((float)Right - (float)Left);
	float OneOverTmB = 1.0f / ((float)Top - (float)Bottom);
	float OneOverFmN = 1.0f / (Far - Near);

	Result.E[0] = 2.0f * OneOverRmL;
	Result.E[3] = - (float)(Right + Left) * OneOverRmL;
	Result.E[5] = 2.0f * OneOverTmB;
	Result.E[7] = - (float)(Top + Bottom) * OneOverTmB;
	Result.E[10] = -2.0f * OneOverFmN;
	Result.E[11] = -(Far + Near) * OneOverFmN;
	Result.E[15] = 1.0f;

	return(Result);
}

inline mat4 OrthographicProjection(
	uint32 Width, uint32 Height,
	float Far, float Near)
{
	mat4 Result = {};

	float OneOverFmN = 1.0f / (Far - Near);
	Result.E[0] = 2.0f / (float)Width;
	Result.E[3] = -1.0f;
	Result.E[5] = 2.0f / (float)Height;
	Result.E[7] = -1.0f;
	Result.E[10] = -2.0f * OneOverFmN;
	Result.E[11] = -(Far + Near) * OneOverFmN;
	Result.E[15] = 1.0f;

	return(Result);
}

inline mat4 OrthographicProjection(uint32 Width, uint32 Height){
	mat4 Result = {};

	Result.E[0] = 2.0f / (float)Width;
	Result.E[3] = -1.0f;
	Result.E[5] = 2.0f / (float)Height;
	Result.E[7] = -1.0f;
	Result.E[10] = 1.0f;
	Result.E[15] = 1.0f;

	return(Result);
}

inline mat4 CameraTransform(
	vec3 P, 
	vec3 Left,
	vec3 Up,
	vec3 Front)
{
	mat4 Result = LookAt(P, P + Front, Vec3(0.0f, 1.0f, 0.0f));

	return(Result);
}

inline real32 Clamp(real32 Value, real32 Min, real32 Max){
	real32 Result = Value;

	if(Result < Min){
		Result = Min;
	}
	else if(Result > Max){
		Result = Max;
	}
	
	return(Result);
}

inline real32 Clamp01(real32 Value){
	real32 Result = Clamp(Value, 0.0f, 1.0f);

	return(Result);
}

inline real32 Clamp01MapToRange(real32 Min, real32 Max, real32 t){
	real32 Result = 0.0f;

	real32 Range = Max - Min;
	if(Range != 0.0f){
		Result = Clamp01((t - Min) / Range);
	}

	return(Result);
}

#endif
