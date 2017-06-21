#include "ivan_render_group.h"

inline push_buffer_result PushBuffer(render_group* RenderGroup, uint32 DataSize){
    game_render_commands* Commands = RenderGroup->Commands;

    push_buffer_result Result = {};

    uint8* PushBufferEnd = Commands->PushBufferBase + Commands->MaxPushBufferSize;
    if((Commands->PushBufferDataAt + DataSize) <= PushBufferEnd){
        Result.Header = (render_group_entry_header*)Commands->PushBufferDataAt;
        Commands->PushBufferDataAt += DataSize;
    }
    else{
        INVALID_CODE_PATH;
    }

    return(Result);
}

inline void 
SetOrthographic(
    render_group* RenderGroup,
    int32 PixelWidth,
    int32 PixelHeight,
    real32 MetersToPixels)
{
    real32 PixelsToMeters = 1.0f / MetersToPixels;
    RenderGroup->MonitorHalfDimInMeters = 
        {0.5f * PixelWidth * PixelsToMeters,
        0.5f * PixelHeight * PixelsToMeters};

    RenderGroup->Transform.MetersToPixels = MetersToPixels;
    RenderGroup->Transform.FocalLength = 1.0f;
    RenderGroup->Transform.DistanceAboveTarget = 1.0f;
    RenderGroup->Transform.ScreenCenter = Vec2(0.5f * PixelWidth, 0.5f * PixelHeight);

    RenderGroup->Transform.Orthographic = true;
}

#if 0
INTERNAL_FUNCTION render_group*
AllocateRenderGroup(struct game_assets* Assets, memory_arena* Arena, uint32 MaxPushBufferSize)
{ 
    render_group* Result = PushStruct(Arena, render_group);
    Result->PushBufferBase = (uint8*)PushSize(Arena, MaxPushBufferSize);

    Result->Assets = Assets;
    Result->GlobalAlphaChannel = 1.0f;

    Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferSize = 0;

    Result->Transform.OffsetP = Vec3(0.0f, 0.0f, 0.0f);
    Result->Transform.Scale = 1.0f;

    Result->GenerationID = 0;
    Result->InsideRender = false;

    return(Result);
}
#endif

inline render_group
BeginRenderGroup(
    game_assets* Assets, 
    game_render_commands* Commands,
    uint32 GenerationID)
{
    render_group Result = {};

    Result.Assets = Assets;
    Result.GenerationID = GenerationID;
    Result.Commands = Commands;

    Result.Transform.OffsetP = Vec3(0.0f, 0.0f, 0.0f);
    Result.Transform.Scale = 1.0f;
    Result.GlobalAlphaChannel = 1.0f;

    return(Result);
}

inline void EndRenderGroup(render_group* RenderGroup){

}

#define PUSH_RENDER_ELEMENT(RenderGroup, type) (type*)PushRenderElement_(RenderGroup, sizeof(type), RenderGroupEntry_##type)
inline void* PushRenderElement_(
    render_group* RenderGroup,
    uint32 Size,
    render_group_entry_type Type)
{

    game_render_commands* Commands = RenderGroup->Commands;

    void* Result = 0;

    Size += sizeof(render_group_entry_header);
    push_buffer_result Push = PushBuffer(RenderGroup, Size);
    if(Push.Header){
        render_group_entry_header* Header = Push.Header;
        Header->Type = Type;
        Result = (uint8*)Header + sizeof(*Header);
    }
    else{
        INVALID_CODE_PATH;
    }

    return(Result);
}

struct entity_basis_p_result{
    vec2 P;
    real32 Scale;
    bool32 Valid;
};

inline entity_basis_p_result GetRenderEntityBasisP(render_group_transform* Transform, vec3 OriginalP){
    TIMED_BLOCK();

    entity_basis_p_result Result = {};

    vec3 P = Add(Vec3(OriginalP.xy, 0.0f), Transform->OffsetP);

    if(Transform->Orthographic){
        //Result.P = Transform->ScreenCenter + Transform->MetersToPixels * P.xy;
        Result.P = Transform->ScreenCenter + Transform->MetersToPixels * Vec2(P.x, -P.y);
        Result.Scale = Transform->MetersToPixels;
        Result.Valid = true;
    }
    else{
        real32 OffsetZ = 0.0f;
        real32 DistanceAboveTarget = Transform->DistanceAboveTarget;

        real32 OffsetToPZ = (DistanceAboveTarget - P.z);
        real32 NearClipPlane = 0.2f;

        vec3 RawXY = Vec3(P.xy, 1.0f);

        if(OffsetToPZ > NearClipPlane){
            vec3 ProjectedXY = (1.0f / OffsetToPZ) * Transform->FocalLength * RawXY;
            Result.Scale = Transform->MetersToPixels * ProjectedXY.z;
            Result.P = Transform->ScreenCenter + Transform->MetersToPixels * ProjectedXY.xy + Vec2(0.0f, Result.Scale * OffsetZ);
            Result.Valid = true;
        }
    }

    return(Result);
}

inline bitmap_dimension
GetBitmapDim(render_group* RenderGroup, loaded_bitmap* Bitmap, real32 Height, vec3 Offset, real32 CAlign)
{
    bitmap_dimension Dim;

    Dim.Size = Vec2(Height * Bitmap->WidthOverHeight, Height);
    Dim.Align = CAlign * Hadamard(Bitmap->AlignPercentage, Dim.Size);
#if 0
    Dim.P = Offset - Vec3(Dim.Align, 0);
#else
    Dim.P.x = Offset.x - Vec3(Dim.Align, 0).x;
    Dim.P.y = Offset.y + Vec3(Dim.Align, 0).y;
#endif
    return(Dim);
}

inline void PushRectangle(
    render_group* RenderGroup,
    vec3 Offset,
    vec2 Dim,
    vec4 Color = Vec4(1.0f, 1.0f, 1.0f, 1.0f),
    bool32 ScreenSpace = false)
{
    vec3 P = {};
    P.x = Offset.x - Dim.x * 0.5f;
    P.y = Offset.y + Dim.y * 0.5f;

    entity_basis_p_result Basis = GetRenderEntityBasisP(&RenderGroup->Transform, P);
    if(Basis.Valid){
        render_entry_rectangle* PushedRect = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_rectangle);
        if (PushedRect){
            PushedRect->Color = Color;

            if(ScreenSpace == false){
                PushedRect->Dim = Dim * Basis.Scale;
                PushedRect->P = Basis.P;
            }
            else{
                PushedRect->Dim = Dim * Basis.Scale;
                PushedRect->P = Offset.xy;
            }
        }
    }
}

inline void PushBitmap(
    render_group* RenderGroup,
    loaded_bitmap* Bitmap,
    real32 Height,
    vec3 Offset,
    vec4 Color = Vec4(1.0f, 1.0f, 1.0f, 1.0f),
    bool32 ScreenSpace = false,
    real32 CAlign = 1.0f)
{
    TIMED_BLOCK();

    render_entry_bitmap* PushedBitmap = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_bitmap);

    bitmap_dimension Dim = GetBitmapDim(RenderGroup, Bitmap, IVAN_MATH_ABS(Height), Offset, CAlign);
    //bitmap_dimension Dim = GetBitmapDim(RenderGroup, Bitmap, Height, Offset, CAlign);

    entity_basis_p_result Basis = GetRenderEntityBasisP(&RenderGroup->Transform, Dim.P);
    if(Basis.Valid){
        if (PushedBitmap){
            PushedBitmap->Bitmap = Bitmap;
    
            if(ScreenSpace == false){
                PushedBitmap->P = Basis.P;
                PushedBitmap->Size = Basis.Scale * Dim.Size;
            }
            else{
                PushedBitmap->P = Dim.P.xy;
                PushedBitmap->Size = Vec2(Bitmap->WidthOverHeight * IVAN_MATH_ABS(Height), Height);
            }
 
            PushedBitmap->Color = Color * RenderGroup->GlobalAlphaChannel;
        }
   }
}

inline void PushBitmap(
    render_group* RenderGroup,
    bitmap_id Id,
    real32 Height,
    vec3 Offset,
    vec4 Color = Vec4(1.0f, 1.0f, 1.0f, 1.0f),
    bool32 ScreenSpace = false,
    real32 CAlign = 1.0f)
{
    TIMED_BLOCK();

    loaded_bitmap* BitmapToRender = GetBitmap(RenderGroup->Assets, Id, RenderGroup->GenerationID);
    if (BitmapToRender){
        PushBitmap(RenderGroup, BitmapToRender, Height, Offset, Color, ScreenSpace, CAlign);
    }
    else{
        LoadBitmapAsset(RenderGroup->Assets, Id, false);
    }
}

inline loaded_font* PushFont(render_group* Group, font_id ID){
    loaded_font* Font = GetFont(Group->Assets, ID, Group->GenerationID);
    if(Font){

    }
    else{
        LoadFontAsset(Group->Assets, ID, false);
    }

    return(Font);
}

extern bitmap_id GetBitmapForVoxelAtlas(game_assets* Assets, loaded_voxel_atlas* Atlas);
//extern void LoadVoxelAtlasAsset(game_assets* Assets, voxel_atlas_id ID, bool32 Immediate);
inline loaded_bitmap*
PushVoxelAtlas(
    render_group* RenderGroup, 
    voxel_atlas_id ID)
{
    TIMED_BLOCK();

    loaded_bitmap* Result = 0;

#if 1
    loaded_voxel_atlas* Atlas = GetVoxelAtlas(RenderGroup->Assets, ID, RenderGroup->GenerationID);
    if(Atlas){
        bitmap_id BmpID = GetBitmapForVoxelAtlas(RenderGroup->Assets, Atlas);
        Result = GetBitmap(RenderGroup->Assets, BmpID, RenderGroup->GenerationID);
		if (Result) {
            
        }
		else {
			LoadBitmapAsset(RenderGroup->Assets, BmpID, false);
		}
    }
    else{
        LoadVoxelAtlasAsset(RenderGroup->Assets, ID, false);
    }
#endif

    return(Result);
}

inline void PushVoxelChunkMesh(
    render_group* RenderGroup,
    voxel_chunk_mesh* Mesh,
    voxel_atlas_id VoxelAtlasID,
    vec3 Pos)
{
    render_entry_voxel_mesh* PushedMesh = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_voxel_mesh);

    if(PushedMesh){
        PushedMesh->Mesh = Mesh;
        PushedMesh->Bitmap = PushVoxelAtlas(RenderGroup, VoxelAtlasID);
        PushedMesh->P = Pos;
    }
}

inline void PushRectangleOutline(
    render_group* RenderGroup,
    vec3 Offset,
    vec2 Dim,
    vec4 Color = Vec4(0.0f, 0.0f, 0.0f, 1.0f),
    real32 Thickness = 4)
{
    PushRectangle(RenderGroup, Offset - Vec3(Thickness, 0.0f, 0.0f), Vec2(Thickness, Dim.y + Thickness), Color);
    PushRectangle(RenderGroup, Offset - Vec3(Thickness, Thickness, 0.0f), Vec2(Dim.x + 2.0f * Thickness, Thickness), Color);
    PushRectangle(RenderGroup, Offset + Vec3(0.0f, Dim.y, 0.0f), Vec2(Dim.x + Thickness, Thickness), Color);
    PushRectangle(RenderGroup, Offset + Vec3(Dim.x, 0.0f, 0.0f), Vec2(Thickness, Dim.y), Color);
}

inline void PushClear(render_group* RenderGroup, vec4 Color){
    render_entry_clear* PushedClear = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_clear);
    if (PushedClear){
        PushedClear->Color = Color;
    }
}

inline void PushCoordinateSystem(render_group* RenderGroup, vec2 Origin, vec2 XAxis, vec2 YAxis, vec4 Color, loaded_bitmap* Texture){
    render_entry_coordinate_system* PushedCS = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_coordinate_system);
    if (PushedCS){
        PushedCS->Origin = Origin;
        PushedCS->XAxis = XAxis;
        PushedCS->YAxis = YAxis;
        PushedCS->Color = Color;
        PushedCS->Texture = Texture;
    }
}
