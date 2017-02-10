INTERNAL_FUNCTION playing_sound*
PlaySound(audio_state* AudioState, sound_id SoundID){
    if(!AudioState->FirstFreePlayingSound){
        AudioState->FirstFreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);
        AudioState->FirstFreePlayingSound->Next = 0;
    }

    playing_sound* PlayingSound = AudioState->FirstFreePlayingSound;
    AudioState->FirstFreePlayingSound = PlayingSound->Next;

    PlayingSound->SamplesPlayed = 0;
    PlayingSound->Volume[0] = 1.0f;
    PlayingSound->Volume[1] = 1.0f;
    PlayingSound->ID = SoundID;

    PlayingSound->Next = AudioState->FirstPlayingSound;
    AudioState->FirstPlayingSound = PlayingSound;

    return(PlayingSound);
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

                real32 Volume0 = PlayingSound->Volume[0];
                real32 Volume1 = PlayingSound->Volume[1];

                Assert(PlayingSound->SamplesPlayed >= 0);

                uint32 SamplesToMix = TotalSamplesToMix;
                uint32 SamplesRemaining = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
                if(SamplesToMix > SamplesRemaining){
                    SamplesToMix = SamplesRemaining;
                }

                for(int SampleIndex = PlayingSound->SamplesPlayed;
                    SampleIndex < PlayingSound->SamplesPlayed + SamplesToMix;
                    SampleIndex++)
                {
                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                    *Dest0++ += Volume0 * SampleValue;
                    *Dest1++ += Volume1 * SampleValue;
                }


                Assert(TotalSamplesToMix >= SamplesToMix);
                PlayingSound->SamplesPlayed += SamplesToMix;
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
}