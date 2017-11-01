#include "ivan_anim.h"

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

INTERNAL_FUNCTION mat4 InterpolateFrames(float Time, joint_animation* JointAnim){
	mat4 Transform;

	mat4 ResultTransl = InterpolateTranslation(Time, JoinAnim);
	mat4 ResultRot = InterpolateRotation(Time, JoinAnim);
	mat4 ResultScale = InterpolateScaling(Time, JoinAnim);

	Transform = ResultTransl * ResultRot * ResultScale;
	
	return(Transform);	
}

INTERNAL_FUNCTION void AnimateModel(
	animator_controller* Controller, 
	playing_animation* PlayingAnimation,
	skinned_mesh* Mesh, 
	float DeltaTime)
{
	PlayingAnimation->PlayCursorTime += DeltaTime * PlayingAnimation->PlaybackSpeed;

	if (PlayingAnimation->PlayCursorTime >= PlayingAnimation->Animation->Length){

		if(PlayingAnimation->Type == PlayingAnimation_Looping){
			PlayingAnimation->PlayCursorTime = 0.0f;
		}
		else{
			Assert(PlayingAnimation->Type == PlayingAnimation_StopAtEnd);
		}
	}

	float CurrentTime = PlayingAnimation->PlayCursorTime;

    PlayingAnimation->Animation = GetAnimation(Group->Assets, PlayingAnimation->ID, Controller->GenerationID);
    if(PlayingAnimation->Animation){

    else{
        LoadAnimationAsset(Controller->Assets, PlayingAnimation->ID, false);
    }
}

INTERNAL_FUNCTION animation_node* PlayAnimation(animator_controller* Controller, animation_id ID){
	animation_node* Result;

	if(Controller->FirstFreeSentinel->NextInList == Controller->FirstFreeSentinel){
		/*Free list is empty. Animation needs to be allocated*/
		Result = PushStruct(&Controller->Arena, animation_node);
	}
	else{
		/*Free list is not empty. Use existing slot*/
		Result = Controller->FirstFreeSentinel->Next;

		/*Remove slot from free list*/
		Result->NextInList->PrevInList = Result->PrevInList;
		Result->PrevInList->NextInList = Result->NextInList;
	}


	/*Push slot to playing list*/
	Result->NextInList = Controller->FirstSentinel->NextInList;
	Result->PrevInList = Controller->FirstSentinel;

	Result->NextInList->PrevInList = Result;
	Result->PrevInList->NextInList = Result;

	playing_animation* RequestedAnim = &Result->Animation;

	RequestedAnim->Animation = 0;
	RequestedAnim->PlayCursorTime = 0;
	RequestedAnim->PlaybackSpeed = 1;
	RequestedAnim->Type = PlayingAnimation_Looping;
	RequestedAnim->ID = ID;

	return(Result);
}

INTERNAL_FUNCTION void UpdateAnimatorController(
	animator_controller* Controller,
	real32  DeltaTime)
{
	Controller->GenerationID = BeginGeneration(Controller->Assets);

	for(animation_node* Node = Controller->FirstSentinel->Next;
		Node != Controller->FirstSentinel;)
	{
		animation_node* TempNextNode = Node->NextInList;
		animation_node* TempPrevNode = Node->PrevInList;

		if(Node->Animation->PlayCursorTime >= Node->Animation.Animation->Length){
			RemoveAnimationNode(Controller, Node);
		}
		else{
			AnimateModel(Controller, &Node->PlayingAnimation, Mesh???, DeltaTime);
		}

		Node = TempNextNode;
	}

	EndGeneration(Controller->Assets, Controller->GenerationID);
}

INTERNAL_FUNCTION void InitializeAnimatorController(
	animator_controller* Animator, 
	memory_arena* PermanentArena,
	game_assets* Assets)
{
	Animator->TranState = TranState;
	Animator->Assets = Assets;

	SubArena(&Animator->Arena, PermanentArena, IVAN_KILOBYTES(100));

	Animator->FirstSentinel = PushStruct(&Animator->Arena, animaion_node);
	Animator->FirstSentinel->NextInList = Animator->FirstSentinel;
	Animator->FirstSentinel->PrevInList = Animator->FirstSentinel

	Animator->FirstFreeSentinel = PushStruct(&Animator->Arena, animation_node);
	Animator->FirstFreeSentinel->NextInList = Animator->FirstFreeSentinel;
	Animator->FirstFreeSentinel->PrevInList = Animator->FirstFreeSentinel;

	return(Animator);
}