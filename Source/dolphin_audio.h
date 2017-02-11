#ifndef DOLPHIN_AUDIO_H

struct volume_v2{
	float Data[2];
};

inline volume_v2 volume_v2_init(float Volume0, float Volume1){
	volume_v2 Result;
	Result.Data[0] = Volume0;
	Result.Data[1] = Volume1;
	return(Result);
}

inline volume_v2 volume_v2_add(volume_v2 A, volume_v2 B){
	volume_v2 Result;
	Result.Data[0] = A.Data[0] + B.Data[0];
	Result.Data[1] = A.Data[1] + B.Data[1];
	return(Result);
}

inline volume_v2 volume_v2_sub(volume_v2 A, volume_v2 B){
	volume_v2 Result;
	Result.Data[0] = A.Data[0] - B.Data[0];
	Result.Data[1] = A.Data[1] - B.Data[1];
	return(Result);
}

inline volume_v2 volume_v2_mulf(volume_v2 Vol, float S){
	Vol.Data[0] = Vol.Data[0] * S;
	Vol.Data[1] = Vol.Data[1] * S;
	return(Vol);
}

inline volume_v2 volume_v2_divf(volume_v2 Vol, float S){
	float InvS = 1.0f / S;
	Vol.Data[0] = Vol.Data[0] * InvS;
	Vol.Data[1] = Vol.Data[1] * InvS;
	return(Vol);
}

inline volume_v2 operator+(volume_v2 A, volume_v2 B){
	volume_v2 Result;
	Result = volume_v2_add(A, B);
	return(Result);
}

inline volume_v2& operator+=(volume_v2& A, volume_v2 B){
	A = volume_v2_add(A, B);
	return(A);
}

inline volume_v2 operator-(volume_v2 A, volume_v2 B){
	volume_v2 Result;
	Result = volume_v2_sub(A, B);
	return(Result);
}

inline volume_v2 operator*(volume_v2 Vol, float S){
	volume_v2 Result;
	Result = volume_v2_mulf(Vol, S);
	return(Result);
}

inline volume_v2 operator*(float S, volume_v2 Vol){
	volume_v2 Result;
	Result = volume_v2_mulf(Vol, S);
	return(Result);
}

inline volume_v2 operator/(volume_v2 Vol, float S){
	volume_v2 Result;
	Result = volume_v2_divf(Vol, S);
	return(Result);
}

struct playing_sound{
	volume_v2 CurrentVolume;
	volume_v2 dCurrentVolume;
	volume_v2 TargetVolume;

	real32 dSample;

	sound_id ID;
	real32 SamplesPlayed;
	playing_sound* Next;
};

struct audio_state{
	memory_arena* PermArena;
	playing_sound* FirstPlayingSound;
	playing_sound* FirstFreePlayingSound;

	volume_v2 MasterVolume;
};

#define DOLPHIN_AUDIO_H
#endif