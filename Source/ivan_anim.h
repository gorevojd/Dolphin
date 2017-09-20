#ifndef IVAN_ANIM_H

struct mesh_vertex{
	vec3 P;
	vec2 UV;
	vec3 N;
	vec3 T;
};

#define MAX_BONE_COUNT 64
#define MAX_INFLUENCE_BONE_COUNT 4
struct mesh_skinned_vertex{
	vec3 P;
	vec2 UV;
	vec3 N;
	vec3 T;

	float BoneWeights[MAX_INFLUENCE_BONE_COUNT];
	uint8 BoneIDs[MAX_INFLUENCE_BONE_COUNT];
};

struct joint_transform{
	vec3 Translation;
	quat Rotation;
	vec3 Scale;
};

struct key_frame{
	joint_transform Transform;

	float TimeStamp;
};

struct joint_animation{
	key_frame* Frames;
	uint32 FramesCount;
};

struct skeletal_animation{

	joint_animation* JointAnims;
	uint32 JointAnimsCount;

	float Length;
};

inline mat4 InterpolateFrames(float Time, joint_animation* JointAnim){
	
	mat4 Transform;

	vec3 Translation;
	quat Rotation;
	vec3 Scaling;

	if(JointAnim->FramesCount == 1){
		Translation = JointAnim->Frames[0].Transform.Translation;
		Rotation = JointAnim->Frames[0].Transform.Rotation;
		Scaling = JointAnim->Frames[0].Transform.Scale;
	}
	else{
		uint32 CurrentFrameIndex = 0;
		for(uint32 FrameIndex = 0;
			FrameIndex < JointAnim->FramesCount - 1;
			FrameIndex++)
		{
			if(Time < JointAnim->Frames[FrameIndex + 1].TimeStamp){
				CurrentFrameIndex = FrameIndex;
				break;
			}
		}

		key_frame* CurrentFrame = &JointAnim->Frames[CurrentFrameIndex];
		key_frame* NextFrame = &JointAnim->Frames[CurrentFrameIndex + 1];

		float Delta = (Time - CurrentFrame->TimeStamp) / (NextFrame->TimeStamp - CurrentFrame->TimeStamp);

		joint_transform* CurrentTransform = &CurrentFrame->Transform;
		joint_transform* NextTransform = &NextFrame->Transform;

		Translation = Lerp(CurrentTransform->Translation, NextTransform->Translation, Delta);
		Rotation = Slerp(CurrentTransform->Rotation, NextTransform->Rotation, Delta);
		Scaling = Lerp(CurrentTransform->Scale, NextTransform->Scale, Delta);
	}

	mat4 ResultTransl = TranslationMatrix(Translation);
	mat4 ResultRot = RotationMatrix(Rotation);
	mat4 ResultScale = ScalingMatrix(Scaling);

	Transform = ResultTransl * ResultRot * ResultScale;
	return(Transform);	
}

#define IVAN_ANIM_H
#endif