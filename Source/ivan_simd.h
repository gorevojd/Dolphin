#ifndef IVAN_SIMD
#define IVAN_SIMD

struct vec3_4x{
	union{
		__m128 x;
		__m128 r;
	};

	union{
		__m128 y;
		__m128 g;
	};

	union{
		__m128 z;
		__m128 b;
	};
};

struct vec4_4x{
	union{
		__m128 x;
		__m128 r;
	};

	union{
		__m128 y;
		__m128 g;
	};

	union{
		__m128 z;
		__m128 b;
	};

	union{
		__m128 w;
		__m128 a;
	};
};

#define mmSquare(value) _mm_mul_ps(value, value)
#define M(a, i) ((float*)(&a))[i]
#define Mi(a, i) ((int*)(&a))[i]

/*vec3_4x operator overloadings*/
inline vec3_4x operator*(float Scalar, vec3_4x B){
	vec3_4x Result;

	__m128 A = _mm_set1_ps(Scalar);
	Result.x = _mm_mul_ps(A, B.x);
	Result.y = _mm_mul_ps(A, B.y);
	Result.z = _mm_mul_ps(A, B.z);

	return(Result);
}

inline vec3_4x operator+(vec3_4x A, vec3_4x B){
	vec3_4x Result;

	Result.x = _mm_add_ps(A.x, B.x);
	Result.y = _mm_add_ps(A.y, B.y);
	Result.z = _mm_add_ps(A.z, B.z);

	return(Result);
}

inline vec3_4x& operator+=(vec3_4x& A, vec3_4x B){
	A.x = _mm_add_ps(A.x, B.x);
	A.y = _mm_add_ps(A.y, B.y);
	A.z = _mm_add_ps(A.z, B.z);

	return(A);
}

/*vec4_4x operator overloadings*/
inline vec4_4x operator*(float Scalar, vec4_4x B){
	vec4_4x Result;

	__m128 A = _mm_set1_ps(Scalar);
	Result.x = _mm_mul_ps(A, B.x);
	Result.y = _mm_mul_ps(A, B.y);
	Result.z = _mm_mul_ps(A, B.z);
	Result.w = _mm_mul_ps(A, B.w);

	return(Result);
}

inline vec4_4x operator+(vec4_4x A, vec4_4x B){
	vec4_4x Result;

	Result.x = _mm_add_ps(A.x, B.x);
	Result.y = _mm_add_ps(A.y, B.y);
	Result.z = _mm_add_ps(A.z, B.z);
	Result.w = _mm_add_ps(A.w, B.w);

	return(Result);
}

inline vec4_4x& operator+=(vec4_4x& A, vec4_4x B){
	A.x = _mm_add_ps(A.x, B.x);
	A.y = _mm_add_ps(A.y, B.y);
	A.z = _mm_add_ps(A.z, B.z);
	A.w = _mm_add_ps(A.w, B.w);

	return(A);
}

inline vec3_4x Vec3ToWide(vec3 A){
	vec3_4x Result;

	Result.x = _mm_set1_ps(A.x);
	Result.y = _mm_set1_ps(A.y);
	Result.z = _mm_set1_ps(A.z);

	return(Result);
}
#endif