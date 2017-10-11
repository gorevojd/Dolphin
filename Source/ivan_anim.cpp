#include "ivan_anim.h"

INTERNAL_FUNCTION void ANIM_AnimateModel(skinned_mesh* Mesh, skeletal_animation* Animation, float DeltaTime){
	Animation->PlayCursorTime += DeltaTime;

	if (Animation->PlayCursorTime >= Animation->Length){

		if(Animation->Type == SkAnimationType_Looping){
			Animation->PlayCursorTime = 0.0f;
		}
		else{
			Assert(Animation->Type == SkAnimationType_StopAtEnd);
			
		}
	}

	float CurrentTime = Animation->PlayCursorTime;

	
}