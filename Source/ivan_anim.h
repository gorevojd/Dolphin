#ifndef IVAN_ANIM_H

#define IVAN_ANIM_MAX_BONE_COUNT 256

struct translation_key_frame{
	vec3 Translation;

	float TimeStamp;
};

struct rotation_key_frame{
	quat Rotation;

	float TimeStamp;
};

struct scaling_key_frame{
	vec3 Scaling;

	float TimeStamp;
};

struct joint_animation{
	translation_key_frame* TranslationFrames;
	uint32 TranslationFramesCount;

	rotation_key_frame* RotationFrames;
	uint32 RotationFramesCount;

	scaling_key_frame* ScalingFrames;
	uint32 ScalingFramesCount;
};

enum skeletal_animation_type{
	SkAnimationType_Looping,
	SKAnimationType_StopAtEnd,
};

struct loaded_animation{
	joint_animation* JointAnims;
	uint32 JointAnimsCount;

	uint8 Type;

	float Length;
	float PlayCursorTime;
	float PlaybackSpeed;
  
    float TicksPerSecond;
};

inline mat4 InterpolateTranslation(float Time, joint_animation* JointAnim){
	vec3 Translation;

	if(JointAnim->TranslationFramesCount == 1){
		Translation = JointAnim->TranslationFrames[0].Translation;
	}
	else{
		uint32 CurrentFrameIndex = 0;
		for(uint32 FrameIndex = 0;
			FrameIndex < JointAnim->TranslationFramesCount - 1;
			FrameIndex++)
		{
			if(Time < JointAnim->TranslationFrames[FrameIndex + 1].TimeStamp){
				CurrentFrameIndex = FrameIndex;
				break;
			}
		}

		translation_key_frame* CurrentFrame = &JointAnim->TranslationFrames[CurrentFrameIndex];
		translation_key_frame* NextFrame = &JointAnim->TranslationFrames[CurrentFrameIndex + 1];

		float Delta = (Time - CurrentFrame->TimeStamp) / (NextFrame->TimeStamp - CurrentFrame->TimeStamp);

		Translation = Lerp(CurrentFrame->Translation, NextFrame->Translation, Delta);
	}

	mat4 ResultTransl = TranslationMatrix(Translation);

	return(ResultTransl);
}

inline mat4 InterpolateRotation(float Time, joint_animation* JointAnim){
	quat Rotation;

	if(JointAnim->RotationFramesCount == 1){
		Rotation = JointAnim->RotationFrames[0].Rotation;
	}
	else{
		uint32 CurrentFrameIndex = 0;
		for(uint32 FrameIndex = 0;
			FrameIndex < JointAnim->RotationFramesCount - 1;
			FrameIndex++)
		{
			if(Time < JointAnim->RotationFrames[FrameIndex + 1].TimeStamp){
				CurrentFrameIndex = FrameIndex;
				break;
			}
		}

		rotation_key_frame* CurrentFrame = &JointAnim->RotationFrames[CurrentFrameIndex];
		rotation_key_frame* NextFrame = &JointAnim->RotationFrames[CurrentFrameIndex + 1];

		Rotation = Slerp(CurrentFrame->Rotation, NextFrame->Rotation, Delta);

	}
	
	mat4 ResultRot = RotationMatrix(Rotation);

	return(ResultRot);
}

inline mat4 InterpolateScaling(float Time, joint_animation* JointAnim){
	vec3 Scaling;

	if(JointAnim->ScalingFramesCount == 1){
		Scaling = JointAnim->ScalingFrames[0].Scale;
	}
	else{
		uint32 CurrentFrameIndex = 0;
		for(uint32 FrameIndex = 0;
			FrameIndex < JointAnim->ScalingFramesCount - 1;
			FrameIndex++)
		{
			if(Time < JointAnim->ScalingFrames[FrameIndex + 1].TimeStamp){
				CurrentFrameIndex = FrameIndex;
				break;
			}
		}
		
		scaling_key_frame* CurrentFrame = &JointAnim->ScalingFrames[CurrentFrameIndex];
		scaling_key_frame* NextFrame = &JointAnim->ScalingFrames[CurrentFrameIndex + 1];

		Scaling = Lerp(CurrentFrame->Scale, NextFrame->Scale, Delta);
	}
	
	mat4 ResultScale = ScalingMatrix(Scaling);

	return(ResultScale);
}

inline mat4 InterpolateFrames(float Time, joint_animation* JointAnim){
	mat4 Transform;

	mat4 ResultTransl = InterpolateTranslation(Time, JoinAnim);
	mat4 ResultRot = InterpolateRotation(Time, JoinAnim);
	mat4 ResultScale = InterpolateScaling(Time, JoinAnim);

	Transform = ResultTransl * ResultRot * ResultScale;
	
	return(Transform);	
}

#define IVAN_ANIM_H
#endif