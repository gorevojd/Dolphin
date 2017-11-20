#include "ivan_world_mode.h"

INTERNAL_FUNCTION void UpdateCameraVectors(
    camera_transform* Camera, 
    float DeltaYaw = 0.0f, 
    float DeltaPitch = 0.0f, 
    float DeltaRoll = 0.0f,
    vec3 WorldUp = Vec3(0.0f, 1.0f, 0.0f))
{
    float DeadEdge = 89.0f * IVAN_DEG_TO_RAD;

    Camera->Yaw += DeltaYaw;
    Camera->Pitch = IVAN_MATH_CLAMP(Camera->Pitch + DeltaPitch, -DeadEdge, DeadEdge);
    Camera->Roll += DeltaRoll;

    vec3 Front;
    Front.x = Cos(Camera->Yaw) * Cos(Camera->Pitch);
    Front.y = Sin(Camera->Pitch);
    Front.z = Sin(Camera->Yaw) * Cos(Camera->Pitch);

    vec3 Left = Normalize(Cross(WorldUp, Front));
    vec3 Up = Normalize(Cross(Front, Left));

    Camera->Front = Front;
    Camera->Left = Left;
    Camera->Up = Up;
}

bool32 UpdateAndRenderWorld(
	game_state *GameState, 
	engine_mode_world *WorldMode, 
	transient_state *TranState,
 	game_input *Input, 
 	render_group *RenderGroup, 
 	loaded_bitmap *DrawBuffer)
{
	real32 WidthOfMonitor = 0.635f;
    real32 MetersToPixels = (real32)DrawBuffer->Width / WidthOfMonitor / 8;
    SetOrthographic(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels);

    SetCameraTransform(
        RenderGroup,
        0, 
        GameState->Camera.P, 
        GameState->Camera.Left,
        GameState->Camera.Up,
        GameState->Camera.Front);

    int temp1 = ArrayCount(Input->Controllers[0].Buttons);
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));

    vec2 MoveVector = Vec2(0.0f);
    vec3 CameraMoveVector = Vec3(0.0f);

    float MouseSpeed = 0.1f;
    float DeltaYaw = 0.0f;
    float DeltaPitch = 0.0f;
#if 0
    float DeltaYaw = (-(Input->MouseP.x - GameState->LastMouseP.x)) * IVAN_DEG_TO_RAD * MouseSpeed;
    float DeltaPitch = (Input->MouseP.y - GameState->LastMouseP.y) * IVAN_DEG_TO_RAD * MouseSpeed;
#else
    if(Input->CapturingMouse && (Input->DeltaMouseP.x != 0 || Input->DeltaMouseP.y != 0)){
        DeltaYaw = (Input->DeltaMouseP.x) * IVAN_DEG_TO_RAD * MouseSpeed;
        DeltaPitch = -(Input->DeltaMouseP.y) * IVAN_DEG_TO_RAD * MouseSpeed;
    }
#endif
    UpdateCameraVectors(&GameState->Camera, DeltaYaw, DeltaPitch, 0.0f);
    
    for (int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ControllerIndex++)
    {
        game_controller_input* Controller = &Input->Controllers[ControllerIndex];

        float AxisDeltaValue = 10.0f;
        if (Controller->MoveUp.EndedDown){
            MoveVector.y += 10;
            CameraMoveVector.z += AxisDeltaValue;
            GameState->HeroFacingDirection = 0.25f * IVAN_MATH_TAU;
        }

        if (Controller->MoveDown.EndedDown){
            MoveVector.y -= 10;
            CameraMoveVector.z -= AxisDeltaValue;
            GameState->HeroFacingDirection = 0.75f * IVAN_MATH_TAU;
        }

        if (Controller->MoveRight.EndedDown){
            MoveVector.x += 10;
            CameraMoveVector.x -= AxisDeltaValue;
            GameState->HeroFacingDirection = 0.0f * IVAN_MATH_TAU;
        }

        if (Controller->MoveLeft.EndedDown){
            MoveVector.x -= 10;
            CameraMoveVector.x += AxisDeltaValue;
            GameState->HeroFacingDirection = 0.5f * IVAN_MATH_TAU;
        }

        if(Controller->Space.EndedDown && Controller->Space.HalfTransitionCount == 0){
            //PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Bloop));
        }

        real32 PlayerSpeed = 0.4f;
        real32 CameraSpeed = 20.0f;
        GameState->PlayerPos = GameState->PlayerPos + Normalize0(MoveVector) * Input->DeltaTime * PlayerSpeed;

        CameraMoveVector = Normalize0(CameraMoveVector) * Input->DeltaTime * CameraSpeed;
        GameState->Camera.P += GameState->Camera.Front * CameraMoveVector.z;
        GameState->Camera.P -= GameState->Camera.Left * CameraMoveVector.x;
        GameState->Camera.P += GameState->Camera.Up * CameraMoveVector.y;
    }


    PushClear(RenderGroup, Vec4(0.05f, 0.05f, 0.05f, 1.0f));
    //PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_LastOfUs), 4.0f, Vec3(0.0f));

    UpdateVoxelChunks(TranState->VoxelChunkManager, RenderGroup, GameState->Camera.P);

    hero_bitmap_ids HeroBitmaps = {};
    asset_vector MatchVector = {};
    MatchVector.Data[Tag_FacingDirection] = GameState->HeroFacingDirection;
    asset_vector WeightVector = {};
    WeightVector.Data[Tag_FacingDirection] = 1.0f;

    HeroBitmaps.Head = GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector);
    HeroBitmaps.Cape = GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector);
    HeroBitmaps.Torso = GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector);

    real32 PlayerSizeConst = 0.8f;
    PushBitmap(RenderGroup, HeroBitmaps.Torso, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Cape, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Head, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));

    GameState->Time += Input->DeltaTime;
    GameState->LastMouseP = Input->MouseP;

/*
    render_group* RenderGroup,
    vec3 Offset,
    vec2 Dim,
    bool32 ScreenSpace = false,
    real32 Thickness = 4,
    vec4 Color = Vec4(0.0f, 0.0f, 0.0f, 1.0f))
*/

/*
    PushRectangleOutline(RenderGroup, Vec3(30,30, 0), Vec2(100, 100), true);
    PushRectangle(RenderGroup, Vec3(30, 30, 0), Vec2(100, 100), Vec4(1.0f, 0.6f, 0.0f, 1.0f), true);
*/
    PushRectangle(RenderGroup, Input->MouseP, Vec2(10, 10), Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);

    real32 Angle = GameState->Time;
    vec2 Origin = Vec2(400, 300);
    real32 AngleW = 0.5f;
    vec2 XAxis = 300.0f * Vec2(Cos(Angle  * AngleW), Sin(Angle * AngleW));
    vec2 YAxis = 300.0f * Vec2(-Sin(Angle * AngleW), Cos(Angle * AngleW));
    real32 OffsetX = Cos(Angle * 0.4f) / 4.0f;
    
/*
    SpawnFontain(&GameState->FontainCache, Vec3(0.0f, 0.0f, 0.0f));    
    UpdateAndRenderParticleSystems(&GameState->FontainCache, Input->DeltaTime, RenderGroup, Vec3(0.0f));
*/

    //TiledRenderGroupToOutput(Memory->HighPriorityQueue, RenderGroup->Commands, (loaded_bitmap*)Buffer);

    return(false);
}