INTERNAL_FUNCTION void
OutputSineWave(game_sound_output_buffer* SoundBuffer, int Frequency, volume_v2 Volume){

    LOCAL_PERSIST real32 Phase = 0;
    real32 Freq = (real32)Frequency;
    real32 WaveFrequency = (real32)Freq / (real32)SoundBuffer->SamplesPerSecond;

    //FIXED(Dima): BUG(Dima): Sound changing it's frequency every 3 - 5 seconds

    int16* SampleOut = (int16*)SoundBuffer->Samples;
    for (int SampleIndex = 0;
        SampleIndex < SoundBuffer->SampleCount;
        ++SampleIndex)
    {

        real32 Omega = 2.0f * DOLPHIN_MATH_PI * WaveFrequency;
        
        int16 SampleValue0 = (int16)(sinf(Phase) * Volume.Data[0]);
        int16 SampleValue1 = (int16)(sinf(Phase) * Volume.Data[1]);

        *SampleOut++ = SampleValue0;
        *SampleOut++ = SampleValue1;

        Phase += Omega;
        if (Phase > 2.0f * DOLPHIN_MATH_PI){
            Phase -= 2.0f * DOLPHIN_MATH_PI;
        }
    }
}

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
#define AUDIO_STATE_OUTPUT_CHANNEL_COUNT 2

    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

    Assert((SoundOutput->SampleCount & 3) == 0);
    uint32 ChunkCount = SoundOutput->SampleCount / 4;
    real32 SecondsPerSample = 1.0f / (real32)SoundOutput->SamplesPerSecond;

    __m128 *RealChannel0 = PushArray(TempArena, ChunkCount, __m128, 16);
    __m128 *RealChannel1 = PushArray(TempArena, ChunkCount, __m128, 16);

    __m128 One = _mm_set1_ps(1.0f);
    __m128 Zero = _mm_set1_ps(0.0f);


    /*Clearing out channels*/
    {
        __m128* Dest0 = RealChannel0;
        __m128* Dest1 = RealChannel1;

        for(uint32 SampleIndex = 0;
            SampleIndex < ChunkCount;
            SampleIndex++)
        {
            _mm_store_ps((float *)Dest0++, Zero);
            _mm_store_ps((float *)Dest1++, Zero);
        }
    }

    /*Summing the sounds*/
    for(playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound;
        *PlayingSoundPtr;)
    {
        playing_sound* PlayingSound = *PlayingSoundPtr;
        bool32 SoundFinished = false;

        uint32 TotalChunksToMix = ChunkCount;

        __m128* Dest0 = RealChannel0;
        __m128* Dest1 = RealChannel1;


        while(TotalChunksToMix && !SoundFinished){
            loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID);
            if(LoadedSound){
                dda_sound* Info = GetSoundInfo(Assets, PlayingSound->ID);
                PrefetchSound(Assets, Info->NextIDToPlay);

                volume_v2 Volume = PlayingSound->CurrentVolume;
                volume_v2 dVolume = SecondsPerSample * PlayingSound->dCurrentVolume;
                volume_v2 dVolumeChunk = 4.0f * dVolume;
                real32 dSample = PlayingSound->dSample;
                real32 dSampleChunk = 4.0f * dSample;

                __m128 MasterVolume0 = _mm_set1_ps(AudioState->MasterVolume.Data[0]);
                __m128 Volume0 = _mm_setr_ps(Volume.Data[0] + 0.0f * dVolume.Data[0],
                                            Volume.Data[0] + 1.0f * dVolume.Data[0],
                                            Volume.Data[0] + 2.0f * dVolume.Data[0],
                                            Volume.Data[0] + 3.0f * dVolume.Data[0]);
                __m128 dVolume0 = _mm_set1_ps(dVolume.Data[0]);
                __m128 dVolumeChunk0 = _mm_set1_ps(dVolumeChunk.Data[0]);

                __m128 MasterVolume1 = _mm_set1_ps(AudioState->MasterVolume.Data[1]);
                __m128 Volume1 = _mm_setr_ps(Volume.Data[1] + 0.0f * dVolume.Data[1],
                                            Volume.Data[1] + 1.0f * dVolume.Data[1],
                                            Volume.Data[1] + 2.0f * dVolume.Data[1],
                                            Volume.Data[1] + 3.0f * dVolume.Data[1]);
                __m128 dVolume1 = _mm_set1_ps(dVolume.Data[1]);
                __m128 dVolumeChunk1 = _mm_set1_ps(dVolumeChunk.Data[1]);

                Assert(PlayingSound->SamplesPlayed >= 0);

                uint32 ChunksToMix = TotalChunksToMix;

                real32 RealChunksRemainingInSound = 
                    (LoadedSound->SampleCount - (int32)(PlayingSound->SamplesPlayed + 0.5f)) / dSampleChunk;
                uint32 ChunksRemainingInSound = (int32)(RealChunksRemainingInSound + 0.5f);
                if(ChunksToMix > ChunksRemainingInSound){
                    ChunksToMix = ChunksRemainingInSound;
                }

                uint32 VolumeEndsAt[AUDIO_STATE_OUTPUT_CHANNEL_COUNT] = {};
                for(uint32 ChannelIndex = 0;
                    ChannelIndex < ArrayCount(VolumeEndsAt);
                    ChannelIndex++)
                {
                    if(dVolume.Data[ChannelIndex] != 0.0f){
                        real32 DeltaVolume = 
                            (PlayingSound->TargetVolume.Data[ChannelIndex] - Volume.Data[ChannelIndex]);
                        uint32 VolumeChunkCount = (uint32)((DeltaVolume / dVolume.Data[ChannelIndex]) + 0.5f);
                        if(ChunksToMix > VolumeChunkCount){
                            ChunksToMix = VolumeChunkCount;
                            VolumeEndsAt[ChannelIndex] = VolumeChunkCount;
                        }
                    }
                }

                real32 BeginSamplePosition = PlayingSound->SamplesPlayed;
                real32 EndSamplePosition = BeginSamplePosition + ChunksToMix * dSampleChunk;
                real32 LoopIndexC = (EndSamplePosition - BeginSamplePosition) / (real32)ChunksToMix;
                for(int RunIndex = 0;
                    RunIndex < ChunksToMix;
                    RunIndex++)
                {
                    real32 SamplePosition = BeginSamplePosition + LoopIndexC * (real32)RunIndex;

#ifdef DOLPHIN_AUDIO_NO_PITCH_LERP
                    __m128 SamplePos = _mm_setr_ps(
                        SamplePosition + 0.0f * dSample,
                        SamplePosition + 1.0f * dSample,
                        SamplePosition + 2.0f * dSample,
                        SamplePosition + 3.0f * dSample);
                    __m128i SampleIndex = _mm_cvttps_epi32(SamplePos);
                    __m128 FractionalPart = _mm_sub_ps(SamplePos, _mm_cvtepi32_ps(SampleIndex));

                    __m128 SampleValueBegin = _mm_setr_ps(LoadedSound->Samples[0][((int32 *)&SampleIndex)[0]],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[1]],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[2]],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[3]]);

                    __m128 SampleValueEnd = _mm_setr_ps(LoadedSound->Samples[0][((int32 *)&SampleIndex)[0] + 1],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[1] + 1],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[2] + 1],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[3] + 1]);

                    __m128 SampleValue = _mm_add_ps(
                        _mm_mul_ps(_mm_sub_ps(One, FractionalPart), SampleValueBegin),
                        _mm_mul_ps(_mm_mul_ps(FractionalPart(FractionalPart, SampleValueEnd))));
#else
                    __m128 SampleValue = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
#endif

                    __m128 D0 = _mm_load_ps((float*)&Dest0[0]);
                    __m128 D1 = _mm_load_ps((float*)&Dest1[0]);
                   
                    D0 = _mm_add_ps(D0, _mm_mul_ps(_mm_mul_ps(MasterVolume0, Volume0), SampleValue));
                    D1 = _mm_add_ps(D1, _mm_mul_ps(_mm_mul_ps(MasterVolume1, Volume1), SampleValue));

                    _mm_store_ps((float *)&Dest0[0], D0);
                    _mm_store_ps((float *)&Dest1[0], D1);

                    ++Dest0;
                    ++Dest1;
                    Volume0 = _mm_add_ps(Volume0, dVolumeChunk0);
                    Volume1 = _mm_add_ps(Volume1, dVolumeChunk1);
                }

                PlayingSound->CurrentVolume.Data[0] = ((real32*)&Volume0)[0];
                PlayingSound->CurrentVolume.Data[1] = ((real32*)&Volume1)[1];

                for(uint32 ChannelIndex = 0;
                    ChannelIndex < ArrayCount(VolumeEndsAt);
                    ChannelIndex++)
                {
                    if(ChunksToMix == VolumeEndsAt[ChannelIndex]){
                        PlayingSound->CurrentVolume.Data[ChannelIndex] = 
                            PlayingSound->TargetVolume.Data[ChannelIndex];
                        PlayingSound->dCurrentVolume.Data[ChannelIndex] = 0.0f;
                    }
                }

                PlayingSound->SamplesPlayed = EndSamplePosition;
                Assert(TotalChunksToMix >= ChunksToMix);
                TotalChunksToMix -= ChunksToMix;

                if(ChunksToMix == ChunksRemainingInSound){
                    if(IsValid(Info->NextIDToPlay)){
                        PlayingSound->ID = Info->NextIDToPlay;
                        Assert(PlayingSound->SamplesPlayed >= LoadedSound->SampleCount);
                        PlayingSound->SamplesPlayed -= (real32)LoadedSound->SampleCount;
                        if(PlayingSound->SamplesPlayed < 0){
                            PlayingSound->SamplesPlayed = 0.0f;
                        }
                    }
                    else{
                        SoundFinished = true;
                    }
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
        __m128* Source0 = RealChannel0;
        __m128* Source1 = RealChannel1;

        __m128i* SampleOut = (__m128i*)SoundOutput->Samples;
        for(uint32 SampleIndex = 0;
            SampleIndex < ChunkCount;
            ++SampleIndex)
        {
            __m128 LoadedLeftSamples = _mm_load_ps((float*)Source0++);
            __m128 LoadedRightSamples = _mm_load_ps((float*)Source1++);

            __m128i Left = _mm_cvtps_epi32(LoadedLeftSamples);
            __m128i Right = _mm_cvtps_epi32(LoadedRightSamples);

            __m128i Lows = _mm_unpacklo_epi32(Left, Right);
            __m128i Highs = _mm_unpackhi_epi32(Left, Right);

            __m128i ShuffledResult = _mm_packs_epi32(Lows, Highs);

            *SampleOut++ = ShuffledResult;
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