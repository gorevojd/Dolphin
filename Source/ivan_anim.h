#ifndef IVAN_ANIM_H

#define IVAN_ANIM_MAX_BONE_COUNT 256

struct loaded_animation;

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

enum playing_animation_type{
	PlayingAnimation_Looping,
	PlayingAnimation_StopAtEnd,
};

struct playing_animation{
	loaded_animation* Animation;

	float PlayCursorTime;
	float PlaybackSpeed;	

	uint8 Type;

	animation_id ID;
};

#define TEMP_ANIMATIONS_COUNT 32

struct temp_animation_slot{
	volatile uint32 InUse;

	loaded_animation* Animation;
};

struct animation_node{
	playing_animation Animation;

	animation_node* NextInList;
	animation_node* PrevInList;
};

struct animator_controller{
	struct game_assets* Assets;

	memory_arena Arena;

	animation_node* FirstSentinel;
	animation_node* FirstFreeSentinel;

	uint32 GenerationID;
};

inline void InsertAnimationNodeAtFront(animator_controller* Controller, animation_node* Node){
	Node->NextInList = Controller->FirstSentinel->NextInList;
	Node->PrevInList = Controller->FirstSentinel;

	Node->NextInList->PrevInList = Node;
	Node->PrevInList->NextInList = Node;
}

inline void RemoveAnimationNode(animator_controller* Controller, animation_node* Node){
	/*Removing from animations list*/
	Node->NextInList->PrevInList = Node->PrevInList;
	Node->PrevInList->NextInList = Node->NextInList;

	/*Adding node to free animation list*/
	Node->NextInList = Controller->FirstFreeSentinel->NextInList;
	Node->PrevInList = Controller->FirstFreeSentinel;

	Node->NextInList->PrevInList = Node;
	Node->PrevInList->NextInList = Node;
}

#define IVAN_ANIM_H
#endif