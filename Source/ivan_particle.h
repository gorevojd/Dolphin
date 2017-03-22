#ifndef IVAN_PARTICLE
#define IVAN_PARTICLE

#define MAX_PARTICLE_COUNT 1024
#define MAX_PARTICLE_COUNT_4 (MAX_PARTICLE_COUNT / 4)

struct particle_4x{
	vec3_4x P;
	vec3_4x dP;
	vec3_4x ddP;

	vec4_4x C;
	vec4_4x dC;
};

struct particle_system{
	particle_4x Particles[MAX_PARTICLE_COUNT_4];
	uint32 NextParticle4;
	bitmap_id BitmapID;
};

struct particle_cache{
	random_series ParticleEntropy;
	particle_system FontainSystem;
};

#endif