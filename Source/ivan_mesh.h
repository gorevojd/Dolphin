#ifndef IVAN_MESH_H
#define IVAN_MESH_H

struct mesh_vertex{
	vec3 P;
	vec2 UV;
	vec3 N;
	vec3 T;
};

struct mesh_skinned_vertex{
	vec3 P;
	vec2 UV;
	vec3 N;
	vec3 T;

	float BoneWeights[DDA_MAX_INFLUENCE_BONE_COUNT];
	uint8 BoneIDs[DDA_MAX_INFLUENCE_BONE_COUNT];
};

struct loaded_mesh{
	mesh_vertex* Vertices;
	uint32 VerticesCount;

	uint32* Indices;
	uint32 IndicesCount;
};

struct loaded_skinned_mesh{
	mesh_skinned_vertex* Vertices;
	uint32 VerticesCount;

	uint32* Indices;
	uint32 IndicesCount;
};

#endif