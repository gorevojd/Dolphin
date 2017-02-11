INTERNAL_FUNCTION playing_sound*
PlaySound(audio_state* AudioState, sound_id SoundID){
    if(!AudioState->FirstFreePlayingSound){
        AudioState->FirstFreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);
        AudioState->FirstFreePlayingSound->Next = 0;
    }

    playing_sound* PlayingSound = AudioState->FirstFreePlayingSound;
    AudioState->FirstFreePlayingSound = PlayingSound->Next;

    PlayingSound->SamplesPlayed = 0;
    PlayingSound->CurrentVolume = volume_v2_init(1.0f, 1.0f);
    PlayingSound->dCurrentVolume = volume_v2_init(0.0f, 0.0f);
    PlayingSound->TargetVolume = volume_v2_init(0.0f, 0.0f);
    PlayingSound->ID = SoundID;
    PlayingSound->dSample = 1.0f;

    PlayingSound->Next = AudioState->FirstPlayingSound;
    AudioState->FirstPlayingSound = PlayingSound;

    return(PlayingSound);
}

INTERNAL_FUNCTION void 
ChangeVolume(
    audio_state* AudioState,
    playing_sound* Sound,
    real32 FadeDurationInSeconds,
    volume_v2 Volume)
{
    if(FadeDurationInSeconds <= 0.0f){
        Sound->TargetVolume = Volume;
        Sound->CurrentVolume = Sound->TargetVolume;
    }
    else{
        real32 InvFade = 1.0f / FadeDurationInSeconds;
        Sound->TargetVolume = Volume;
        Sound->dCurrentVolume = InvFade * (Sound->TargetVolume - Sound->CurrentVolume);
    }
}

INTERNAL_FUNCTION void
ChangePitch(
    audio_state* AudioState, 
    playing_sound* Sound, 
    real32 dSample)
{
    Sound->dSample = dSample;
}

INTERNAL_FUNCTION void
OutputPlayingSounds(
	audio_state* AudioState,
	game_sound_output_buffer* SoundOutput,
	game_assets* Assets,
	memory_arena* TempArena)
{
	temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

    real32* RealChannel0 = PushArray(TempArena, SoundOutput->SampleCount, real32);
    real32* RealChannel1 = PushArray(TempArena, SoundOutput->SampleCount, real32);

    real32 SecondsPerSample = 1.0f / (real32)SoundOutput->SamplesPerSecond;

#define AUDIO_STATE_OUTPUT_CHANNEL_COUNT 2

    /*Clearing out channels*/
    {
        real32* Dest0 = RealChannel0;
        real32* Dest1 = RealChannel1;
        for(int SampleIndex = 0;
            SampleIndex < SoundOutput->SampleCount;
            SampleIndex++)
        {
            *Dest0++ = 0.0f;
            *Dest1++ = 0.0f;
        }
    }

    /*Summing the sounds*/
    for(playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound;
        *PlayingSoundPtr;)
    {
        playing_sound* PlayingSound = *PlayingSoundPtr;
        bool32 SoundFinished = false;

        uint32 TotalSamplesToMix = SoundOutput->SampleCount;
        real32* Dest0 = RealChannel0;
        real32* Dest1 = RealChannel1;

        while(TotalSamplesToMix && !SoundFinished){

            loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID);
            if(LoadedSound){
                asset_sound_info* Info = GetSoundInfo(Assets, PlayingSound->ID);
                PrefetchSound(Assets, Info->NextIDToPlay);

                volume_v2 Volume = PlayingSound->CurrentVolume;
                volume_v2 dVolume = SecondsPerSample * PlayingSound->dCurrentVolume;
                real32 dSample = PlayingSound->dSample;

                Assert(PlayingSound->SamplesPlayed >= 0);

                uint32 SamplesToMix = TotalSamplesToMix;
                //uint32 SamplesRemaining = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
                real32 RealSamplesRemainingInSound = 
                    (LoadedSound->SampleCount - (int32)(PlayingSound->SamplesPlayed + 0.5f)) / dSample;
                uint32 SamplesRemainingInSound = (int32)(RealSamplesRemainingInSound + 0.5f);
                if(SamplesToMix > SamplesRemainingInSound){
                    SamplesToMix = SamplesRemainingInSound;
                }

                bool32 VolumeEnded[AUDIO_STATE_OUTPUT_CHANNEL_COUNT] = {};
                for(uint32 ChannelIndex = 0;
                    ChannelIndex < ArrayCount(VolumeEnded);
                    ChannelIndex++)
                {
                    if(dVolume.Data[ChannelIndex] != 0.0f){
                        real32 DeltaVolume = 
                            (PlayingSound->TargetVolume.Data[ChannelIndex] - Volume.Data[ChannelIndex]);
                        uint32 VolumeSampleCount = (uint32)((DeltaVolume / dVolume.Data[ChannelIndex]) + 0.5f);
                        if(SamplesToMix > VolumeSampleCount){
                            SamplesToMix = VolumeSampleCount;
                            VolumeEnded[ChannelIndex] = true;
                        }
                    }
                }

                real32 SamplePosition = PlayingSound->SamplesPlayed;
                for(int RunIndex = PlayingSound->SamplesPlayed;
                    RunIndex < PlayingSound->SamplesPlayed + SamplesToMix;
                    RunIndex++)
                {
#if 0
                    uint32 SampleIndex = (int32)(SamplePosition + 0.5f);
                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
#else
                    uint32 SampleIndex = FloorReal32ToInt32(SamplePosition);
                    real32 FractionalPart = SamplePosition - (real32)SampleIndex;
                    real32 Sample0 = (real32)LoadedSound->Samples[0][SampleIndex];
                    real32 Sample1 = (real32)LoadedSound->Samples[0][SampleIndex + 1];
                    real32 SampleValue = Sample0 + (Sample1 - Sample0) * FractionalPart;
#endif
                    *Dest0++ += AudioState->MasterVolume.Data[0] * Volume.Data[0] * SampleValue;
                    *Dest1++ += AudioState->MasterVolume.Data[1] * Volume.Data[1] * SampleValue;

                    Volume += dVolume;
                    SamplePosition += dSample;
                }

                PlayingSound->CurrentVolume = Volume;

                for(uint32 ChannelIndex = 0;
                    ChannelIndex < ArrayCount(VolumeEnded);
                    ChannelIndex++)
                {
                    if(VolumeEnded[ChannelIndex]){
                        PlayingSound->CurrentVolume.Data[ChannelIndex] = 
                            PlayingSound->TargetVolume.Data[ChannelIndex];
                        PlayingSound->dCurrentVolume.Data[ChannelIndex] = 0.0f;
                    }
                }

                Assert(TotalSamplesToMix >= SamplesToMix);
                PlayingSound->SamplesPlayed  = SamplePosition;
                TotalSamplesToMix -= SamplesToMix;

                if((uint32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount){
                    if(IsValid(Info->NextIDToPlay)){
                        PlayingSound->ID = Info->NextIDToPlay;
                        PlayingSound->SamplesPlayed = 0;
                    }
                    else{
                        SoundFinished = true;
                    }
                }
                else{
                    Assert(TotalSamplesToMix == 0);
                }
            }
            else{
                LoadSoundAsset(Assets, PlayingSound->ID);
                break;
            }
        }

        if(SoundFinished){
            *PlayingSoundPtr = PlayingSound->Next;
            PlayingSound->Next = AudioState->FirstFreePlayingSound;
            AudioState->FirstFreePlayingSound = PlayingSound;
        }
        else{
            PlayingSoundPtr = &PlayingSound->Next;
        }
    }    

    /*Outputing 16-bits values*/
    {
        real32* Source0 = RealChannel0;
        real32* Source1 = RealChannel1;

        int16* SampleOut = SoundOutput->Samples;
        for(int SampleIndex = 0;
            SampleIndex < SoundOutput->SampleCount;
            SampleIndex++)
        {
            *SampleOut++ = (int16)(*Source0++ + 0.5f);
            *SampleOut++ = (int16)(*Source1++ + 0.5f);
        }
    }

    EndTemporaryMemory(MixerMemory);	
}

INTERNAL_FUNCTION void
InitializeAudioState(audio_state* AudioState, memory_arena* PermArena){
	AudioState->PermArena = PermArena;
	AudioState->FirstPlayingSound = 0;
	AudioState->FirstFreePlayingSound = 0;

    AudioState->MasterVolume = volume_v2_init(1.0f, 1.0f);
}