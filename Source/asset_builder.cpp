#include "asset_builder.h"

/*
    NOTE(Dima): 
		1) Images are stored in gamma-corrected 
		premultiplied-alpha format

		2) Animations will be baked into models. 
		Later maybe if I need I should make that 
		I can do it both ways.

		3) IMPORTANT!!! 
		If you bake multiple animations in a
		single file, you should make sure that
		theese animations are common by sence.
*/

/*
    TODO(Dima):
        Font Atlas
		Model asset type with animations baked into it
*/

#pragma pack(push, 1)
struct bitmap_header{
    uint16 FileType; /*File type, always 4d42 ("BM")*/
    uint32 FileSize; /*Size of the file in bytes*/
    uint16 Reserved1; /*Always 0*/
    uint16 Reserved2; /*Always 0*/
    uint32 BitmapOffset; /*Starting position of image data in bytes*/

    uint32 Size; /*Size of header in bytes*/
    int32 Width; /*Image width in pixels*/
    int32 Height; /*Image height in pixels*/
    uint16 Planes; /*Number of color planes*/
    uint16 BitsPerPixel; /*Number of bits per pixel*/

    uint32 Compression; /*Compression methods used*/
    uint32 SizeOfBitmap; /*Size of bitmap in bytes*/
    int32 HorzResolution;/*Horizontal resolution in pixels per meter*/
    int32 VertResolution;/*Vertical resolution in pixels per meter*/
    uint32 ColorsUsed;/*Number of colors in the image*/
    uint32 ColorsImportant;/*Minimum number of important colors*/

    uint32 RedMask; /*Mask identifying bits of red component*/
    uint32 GreenMask; /*Mask identifying bits of green component*/
    uint32 BlueMask; /*Mask identifying bits of blue component*/
    uint32 AlphaMask; /*Mask identifying bits of Alpha component*/
    uint32 CSType; /*Color space type*/
    int32 RedX; /*X coordinate of red endpoint*/
    int32 RedY; /*Y coordinate of red endpoint*/
    int32 RedZ; /*Z coordinate of red endpoint*/
    int32 GreenX; /*X coordinate of green endpoint*/
    int32 GreenY; /*Y coordinate of green endpoint*/
    int32 GreenZ; /*Z coordinate of green endpoint*/
    int32 BlueX; /*X coordinate of blue endpoint*/
    int32 BlueY; /*Y coordinate of blue endpoint*/
    int32 BlueZ; /*Z coordinate of blue endpoint*/
    uint32 GammaRed; /*Gamma red coordinate scale value*/
    uint32 GammaGreen; /*Gamma green coordinate scale value*/
    uint32 GammaBlue; /*Gamma blue coordinate scale value*/
};

struct WAVE_header{
    uint32 RIFFID;
    uint32 Size;
    uint32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum{
    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
};

struct WAVE_chunk{
    uint32 Id;
    uint32 Size;
};

struct WAVE_fmt{
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
    uint16 wValidBitsPerSample;
    uint32 dwChannelMask;
    uint8 SubFormat[16];
};
#pragma pack(pop)

struct loaded_file{
	unsigned int DataSize;
	void* Data;
};

loaded_file ReadEntireFile(char* FileName){
	loaded_file Result = {};

	FILE* In = fopen(FileName, "rb");
	if(In){
		fseek(In, 0, SEEK_END);
		Result.DataSize = ftell(In);
		fseek(In, 0, SEEK_SET);

		Result.Data = malloc(Result.DataSize);
		fread(Result.Data, Result.DataSize, 1, In);
		fclose(In);
	}
	else{
		printf("ERROR: Can not open the file %s\n", FileName);
	}

	return(Result);
}

struct loaded_bitmap{
	int Width;
	int Height;
	void* Memory;

	void* Free;
};

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

INTERNAL_FUNCTION loaded_bitmap
LoadBMP(char* FileName, bool32 FlipOnLoad = false){
    loaded_bitmap Result = {};

    stbi_set_flip_vertically_on_load(FlipOnLoad);

    loaded_file ReadResult = ReadEntireFile(FileName);

    Result.Free = ReadResult.Data;
    Result.Memory = stbi_load_from_memory(
        (stbi_uc*)ReadResult.Data,
        ReadResult.DataSize,
        &Result.Width,
        &Result.Height,
        0,
        STBI_rgb_alpha);

    uint32* Pixel = (uint32*)Result.Memory;
    for (int j = 0; j < Result.Height; j++){
        for (int i = 0; i < Result.Width; i++){
            uint32 SrcPixel = *Pixel;

            vec4 Color = {
                (float)(SrcPixel & 0xFF),
                (float)((SrcPixel >> 8) & 0xFF),
                (float)((SrcPixel >> 16) & 0xFF),
                (float)((SrcPixel >> 24) & 0xFF) };


#if 1
            /*Gamma-corrected premultiplied alpha*/
            Color = SRGB255ToLinear1(Color);
            real32 Alpha = Color.a;
            Color.r = Color.r * Alpha;
            Color.g = Color.g * Alpha;
            Color.b = Color.b * Alpha;
            Color = Linear1ToSRGB255(Color);
#else
            /*Premultiplied alpha*/
            Color.rgb *= Color.a;
#endif

            *Pixel++ = (((uint32)(Color.a + 0.5f) << 24) |
                ((uint32)(Color.r + 0.5f) << 16) |
                ((uint32)(Color.g + 0.5f) << 8) |
                ((uint32)(Color.b + 0.5f) << 0));

        }
    }

    return(Result);
}

enum load_mesh_flags{
    LoadMesh_RecalculateNormals = 1,
    LoadMesh_CalculateTangents = 2,
    LoadMesh_LoadSkinData = 4,
    LoadMesh_UseExistingSkeleton = 8,
};

#if BUILD_WITH_ASSIMP

INTERNAL_FUNCTION void LoadBoneInfoFromAssimp(
    aiMesh* Mesh,
    uint32 VertexBase, 
    bone_vertex_info* Infos,
    loaded_skeleton* Skeleton)
{
    for(uint32 BoneIndex = 0; 
        BoneIndex < Mesh->mNumBones;
        BoneIndex++)
    {
        Assert(Mesh->mNumBones <= MAX_BONE_COUNT);

        char* BoneName = Mesh->mBones[BoneIndex]->mName.data;
        std::string BoneNameString = std::string(BoneName);

        uint32 IndexToBoneArray = 0;

        if(Skeleton != 0){
            if(Skeleton->BoneMapping.find(BoneName) == Skeleton->BoneMapping.end()){
                IndexToBoneArray = Skeleton->BoneMapping.size();
                bone_transform_info BoneTransform;
                BoneTransform.Name = BoneName;
                BoneTransform.Offset = AiMatToOurs(&Mesh->mBones[BoneIndex]->mOffsetMatrix);
                Skeleton->Bones.push_back(BoneTransform);
                Skeleton->BoneMapping[BoneNameString] = IndexToBoneArray;
            }
            else{
                IndexToBoneArray = Skeleton->BoneMapping[BoneNameString];
            }
        }

        for(uint32 WeightIndex = 0;
            WeightIndex < Mesh->mBones[BoneIndex]->mNumWeights;
            WeightIndex++)
        {
            uint32 VertexIndex = VertexBase + Mesh->mBones[BoneIndex]->mWeights[WeightIndex].mVertexId;
            AddBoneVertexInfo(
                &Infos[VertexIndex], 
                Mesh->mBones[BoneIndex]->mWeights[WeightIndex].mWeight, 
                IndexToBoneArray);
        }
    }
}


INTERNAL_FUNCTION void
LoadSkinnedMeshDataFromAssimp(
	loaded_mesh* Result,
	char* FileName, 
	loaded_skeleton* Skeleton)
{
	Assimp::Importer Importer;
	const aiScene* AssimpScene = Importer.ReadFile(FileName, 0);

	Result->Type = DDAMeshType_Skinned;

    uint32 IndexBase = 0;
    uint32 VertexBase = 0;

    uint32 TotalVertexCount = 0;
    for(uint32 MeshIndex = 0;
        MeshIndex < AssimpScene->mNumMeshes;
        MeshIndex++)
    {
        TotalVertexCount += AssimpScene->mMeshes[MeshIndex]->mNumVertices;
    }
    Result->VerticesCount = TotalVertexCount;

    bone_vertex_info* VertexInfos = (bone_vertex_info*)calloc(TotalVertexCount, sizeof(bone_vertex_info));

    for(uint32 MeshIndex = 0;
        MeshIndex < AssimpScene->mNumMeshes;
        MeshIndex++)
    {       
        aiMesh* Mesh = AssimpScene->mMeshes[MeshIndex];

        if(Mesh->mNumBones > 0){
            LoadBoneInfoFromAssimp(Mesh, VertexBase, VertexInfos, Skeleton);
        }

        VertexBase += Mesh->mNumVertices;
    }

    VertexBase = 0;
    for(uint32 MeshIndex = 0;
        MeshIndex < AssimpScene->mNumMeshes;
        MeshIndex++)
    {       
        aiMesh* Mesh = AssimpScene->mMeshes[MeshIndex];

        for(uint32 VertexIndex = 0; 
            VertexIndex < Mesh->mNumVertices;
            VertexIndex++)
        {
            aiVector3D* RefP = &Mesh->mVertices[VertexIndex];        
            aiVector3D* RefN = &Mesh->mNormals[VertexIndex];

            Assert(VertexBase + VertexIndex <= MAX_VERTICES_COUNT);
            skinned_vertex* NewVertex = &Result->SkinnedVertices[VertexBase + VertexIndex];

            NewVertex->P.x = RefP->x;
            NewVertex->P.y = RefP->y;
            NewVertex->P.z = RefP->z;

            NewVertex->N.x = RefN->x;
            NewVertex->N.y = RefN->y;
            NewVertex->N.z = RefN->z;

            if(Mesh->mTextureCoords[0]){
                NewVertex->UV.x = Mesh->mTextureCoords[0][VertexIndex].x;
                NewVertex->UV.y = Mesh->mTextureCoords[0][VertexIndex].y;
            }

            for(int32 InfluenceBoneIndex = 0;
                InfluenceBoneIndex < MAX_INFLUENCE_BONE_COUNT;
                InfluenceBoneIndex++)
            {
                NewVertex->Weights[InfluenceBoneIndex] = VertexInfos[VertexBase + VertexIndex].Weights[InfluenceBoneIndex];
                NewVertex->BoneIDs[InfluenceBoneIndex] = VertexInfos[VertexBase + VertexIndex].BoneIDs[InfluenceBoneIndex];
            }
        }

        uint32 IndexIterator = 0;
        for(uint32 FaceIndex = 0;
            FaceIndex < Mesh->mNumFaces; 
            FaceIndex++)
        {
            aiFace* AssimpFace = &Mesh->mFaces[FaceIndex];

            for(uint32 FaceVertexIndex = AssimpFace->mNumIndices;
                FaceVertexIndex > 0; 
                FaceVertexIndex--)
            {
                Result->Indices[IndexBase + IndexIterator++] = AssimpFace->mIndices[FaceVertexIndex - 1];
            }
        }
#if 1
		//NOTE(Dima): Tangents calculation
        for(uint32 FaceIndex = 0;
            FaceIndex < Mesh->mNumFaces; 
            FaceIndex++)
        {
            aiFace* AssimpFace = &Mesh->mFaces[FaceIndex];
            if(AssimpFace->mNumIndices == 3){
                vec3 NewT;

                vec3 Vert1 = AiVec3ToOurs(Mesh->mVertices[AssimpFace->mIndices[0]]);
                vec3 Vert2 = AiVec3ToOurs(Mesh->mVertices[AssimpFace->mIndices[1]]);
                vec3 Vert3 = AiVec3ToOurs(Mesh->mVertices[AssimpFace->mIndices[2]]);

                vec2 UV1 = AiVec3ToOurs(Mesh->mTextureCoords[0][AssimpFace->mIndices[0]]).xy;
                vec2 UV2 = AiVec3ToOurs(Mesh->mTextureCoords[0][AssimpFace->mIndices[1]]).xy;
                vec2 UV3 = AiVec3ToOurs(Mesh->mTextureCoords[0][AssimpFace->mIndices[2]]).xy;

                vec3 Edge1 = Vert2 - Vert1;
                vec3 Edge2 = Vert3 - Vert1;

                vec2 DeltaUV1 = UV2 - UV1;
                vec2 DeltaUV2 = UV3 - UV1;

                float f = 1.0f / (DeltaUV1.x * DeltaUV2.y - DeltaUV2.x * DeltaUV1.y);

                NewT.x = f *(DeltaUV2.y * Edge1.x - DeltaUV1.y * Edge2.x);
                NewT.y = f *(DeltaUV2.y * Edge1.y - DeltaUV1.y * Edge2.y);
                NewT.z = f *(DeltaUV2.y * Edge1.z - DeltaUV1.y * Edge2.z);
                NewT = Normalize(NewT);
                    
                Result->SkinnedVertices[IndexBase + AssimpFace->mIndices[0]].T = NewT;
                Result->SkinnedVertices[IndexBase + AssimpFace->mIndices[1]].T = NewT;
                Result->SkinnedVertices[IndexBase + AssimpFace->mIndices[2]].T = NewT;
            }
        }
#endif

        IndexBase += IndexIterator;
    }

    Result->IndicesCount = IndexBase;

    free(VertexInfos);
}

inline aiNodeAnim* FindNodeAnimInAssimpAnimation(aiAnimation* Animation, char* NodeName){
    aiNodeAnim* Result = 0;

    for(uint32 ChannelIndex = 0;
        ChannelIndex < Animation->mNumChannels;
        ChannelIndex++)
    {
        aiNodeAnim* NodeAnim = Animation->mChannels[ChannelIndex];

        if(strcmp(NodeAnim->mNodeName.data, NodeName) == 0){
            Result = NodeAnim;
            break;
        }
    }

    return(Result);
}

INTERNAL_FUNCTION void LoadJointAnimationFromAssimpRecursively(
    loaded_animation* SkAnimation, 
    aiNode* AssimpNode,
    aiAnimation* AssimpAnimation,
    loaded_skeleton* Skeleton)
{
    std::string CurrentJointName = std::string(AssimpNode->mName.data);
    uint32 CurrentJointIndex = Skeleton->BoneMapping[CurrentJointName];

    joint_animation* Animation = &SkAnimation->JointAnims[CurrentJointIndex];

    aiNodeAnim* NodeAnim = FindNodeAnimInAssimpAnimation(AssimpAnimation, AssimpNode->mName.data);

    Animation->TranslationFramesCount = NodeAnim->mNumPositionKeys;
    Animation->RotationFramesCount = NodeAnim->mNumRotationKeys;
    Animation->ScalingFramesCount = NodeAnim->mNumScalingKeys;
    uint32 JointMemoryBlockByteSize = 
        (Animation->TranslationFramesCount * sizeof(translation_key_frame)) + 
        (Animation->RotationFramesCount * sizeof(rotation_key_frame)) + 
        (Animation->ScalingFramesCount * sizeof(scaling_key_frame));

    uint8* JointMemoryBlock = (uint8*)malloc(JointMemoryBlockByteSize);
    memset(JointMemoryBlock, 0, JointMemoryBlockByteSize);

    Animation->TranslationFrames = (translation_key_frame*)JointMemoryBlock;
    Animation->RotationFrames = (rotation_key_frame*)(JointMemoryBlock + Animation->TranslationFramesCount * sizeof(translation_key_frame));
    Animation->ScalingFrames = (scaling_key_frame*)((uint8*)Animation->RotationFrames + Animation->RotationFramesCount * sizeof(rotation_key_frame));

    Animation->Free = JointMemoryBlock;

    //NOTE(Dima): Copying the translation animation data;
    for(uint32 TranslationFrameIndex = 0;
        TranslationFrameIndex < Animation->TranslationFramesCount;
        TranslationFrameIndex++)
    {
        aiVectorKey* SrcFrame = &NodeAnim->mPositionKeys[TranslationFrameIndex];
        translation_key_frame* DstFrame = &Animation->TranslationFrames[TranslationFrameIndex];
        
        DstFrame->TimeStamp = (float)SrcFrame->mTime;
        DstFrame->Translation = AiVec3ToOurs(SrcFrame->mValue);
    }

    //NOTE(Dima): Copying the rotation animation data;
    for(uint32 RotationFrameIndex = 0;
        RotationFrameIndex < Animation->RotationFramesCount;
        RotationFrameIndex++)
    {
        aiQuatKey* SrcFrame = &NodeAnim->mRotationKeys[RotationFrameIndex];
        rotation_key_frame* DstFrame = &Animation->RotationFrames[RotationFrameIndex];

        DstFrame->TimeStamp = (float)SrcFrame->mTime;
        DstFrame->Rotation = AiQuatToOurs(SrcFrame->mValue);
    }

    //NOTE(Dima): Copying the scaling animation data;
    for(uint32 ScalingFrameIndex = 0;
        ScalingFrameIndex < Animation->ScalingFramesCount;
        ScalingFrameIndex++)
    {
        aiVectorKey* SrcFrame = &NodeAnim->mScalingKeys[ScalingFrameIndex];
        scaling_key_frame* DstFrame = &Animation->ScalingFrames[ScalingFrameIndex];

        DstFrame->TimeStamp = (float)SrcFrame->mTime;
        DstFrame->Scaling = AiVec3ToOurs(SrcFrame->mValue);
    }

    char* CurrentBoneName = AssimpNode->mName.data;
    std::string CurrentBoneNameStr = std::string(CurrentBoneName);
    bone_transform_info* CurrentBoneTransform = &(Skeleton->Bones.at(Skeleton->BoneMapping[CurrentBoneNameStr]));    

    //NOTE(Dima): Children processing
    if(AssimpNode->mNumChildren > 0){
        //NOTE(Dima): Allocating children
        CurrentBoneTransform->ChildrenCount = AssimpNode->mNumChildren;
		/*
        CurrentBoneTransform->Children = (bone_transform_info*)
            malloc(sizeof(bone_transform_info) * CurrentBoneTransform->ChildrenCount);
		*/

		CurrentBoneTransform->Children = (uint32*)malloc(sizeof(uint32) * CurrentBoneTransform->ChildrenCount);

        //NOTE(Dima): Assigning children indices to current bone
        for(uint32 ChildIndex = 0;
            ChildIndex < AssimpNode->mNumChildren;
            ChildIndex++)
        {
            std::string ChildNodeName = std::string(AssimpNode->mChildren[ChildIndex]->mName.data);

            if(Skeleton->BoneMapping.find(ChildNodeName) != Skeleton->BoneMapping.end()){
                CurrentBoneTransform->Children[ChildIndex] = Skeleton->BoneMapping[ChildNodeName];
            }
            else{
                //NOTE(Dima): This should not happen because child bone should be found
                INVALID_CODE_PATH;
            }
        }
    }
    else{
        CurrentBoneTransform->Children = 0;
        CurrentBoneTransform->ChildrenCount = 0;
    }

    //NOTE(Dima): Hopefully this should work
    if(AssimpNode->mNumChildren){
        for(uint32 ChildIndex = 0;
            ChildIndex < AssimpNode->mNumChildren;
            ChildIndex++)
        {
            LoadJointAnimationFromAssimpRecursively(
				SkAnimation,
                AssimpNode->mChildren[ChildIndex], 
                AssimpAnimation, 
                Skeleton);
        }
    }
}

INTERNAL_FUNCTION loaded_animations_result
LoadSkeletalAnimation(
	char* FileName, 
	loaded_skeleton* Skeleton, 
	asset_animation_type AssetAnimationType)
{
    Assimp::Importer Importer;
    const aiScene* AssimpScene = Importer.ReadFile(FileName, 0);

    loaded_animations_result Result_ = {};
    loaded_animations_result* Result = &Result_;

    uint32 SrcAnimationsCount = AssimpScene->mNumAnimations;
    Result->AnimationsCount = SrcAnimationsCount;
    Result->Animations = (loaded_animation*)malloc(sizeof(loaded_animation) * SrcAnimationsCount);
    Result->Free = (void*)Result->Animations;

    for(uint32 CurrentAnimationIndex = 0;
        CurrentAnimationIndex < SrcAnimationsCount;
        CurrentAnimationIndex++)
    {
        aiAnimation* SrcAnim = AssimpScene->mAnimations[CurrentAnimationIndex];
        loaded_animation* DstAnim = &Result->Animations[CurrentAnimationIndex];

        DstAnim->LengthTime = (float)SrcAnim->mDuration;
        DstAnim->PlayCursorTime = 0.0f;
        DstAnim->PlaybackSpeed = 1.0f;
		DstAnim->Type = AssetAnimationType;
        
        if(SrcAnim->mTicksPerSecond != 0){
            DstAnim->TicksPerSecond = SrcAnim->mTicksPerSecond;
        }
        else{
            DstAnim->TicksPerSecond = 25.0f;
        }

        uint32 BonesCount = Skeleton->Bones.size();
        memset(DstAnim->JointAnims, 0, DDA_ANIMATION_MAX_BONE_COUNT * sizeof(joint_animation));
        DstAnim->JointAnimsCount = BonesCount;

        LoadJointAnimationFromAssimpRecursively(
            DstAnim, 
            AssimpScene->mRootNode,
            SrcAnim,
            Skeleton);
    }

	return(Result_);
}

#endif

INTERNAL_FUNCTION loaded_font*
LoadFont(char* FileName, char* FontName, int PixelHeight){
    loaded_font* Font = (loaded_font*)malloc(sizeof(loaded_font));

#if USE_FONTS_FROM_WINDOWS
    AddFontResourceExA(FileName, FR_PRIVATE, 0);
    Font->Win32Handle = CreateFontA(
        PixelHeight,
        0, 0, 0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        FontName);

    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);
    GetTextMetrics(GlobalFontDeviceContext, &Font->TextMetric);
#else
    loaded_file TTFFile = ReadEntireFile(FileName);
    Font->FileContents = TTFFile.Data;

    if(TTFFile.DataSize != 0){
        stbtt_InitFont(
            &Font->FontInfo,
            (uint8*)TTFFile.Data,
            stbtt_GetFontOffsetForIndex((uint8*)TTFFile.Data, 0));

        Font->Scale = stbtt_ScaleForPixelHeight(&Font->FontInfo, PixelHeight);

        int Ascent, Descent, LineGap;
        stbtt_GetFontVMetrics(&Font->FontInfo, &Ascent, &Descent, &LineGap);

        Font->AscenderHeight = Font->Scale * Ascent;
        Font->DescenderHeight = Font->Scale * -Descent;
        Font->ExternalLeading = Font->Scale * LineGap;
    }
#endif

    Font->MinCodePoint = INT_MAX;
    Font->MaxCodePoint = 0;

    Font->MaxGlyphCount = 5000;
    Font->GlyphCount = 0;

    uint32 GlyphFromCodepointArraySize = ONE_PAST_MAX_FONT_CODEPOINT * sizeof(uint32);
    Font->GlyphIndexFromCodePoint = (uint32*)malloc(GlyphFromCodepointArraySize);
    memset(Font->GlyphIndexFromCodePoint, 0, GlyphFromCodepointArraySize);

    Font->Glyphs = (dda_font_glyph*)malloc(sizeof(dda_font_glyph) * Font->MaxGlyphCount);
    size_t HorizontalAdvanceArraySize = sizeof(float) * Font->MaxGlyphCount * Font->MaxGlyphCount;
    Font->HorizontalAdvance = (float*)malloc(HorizontalAdvanceArraySize);
    memset(Font->HorizontalAdvance, 0, HorizontalAdvanceArraySize);

    Font->OnePastHighestCodepoint = 0;

    Font->GlyphCount = 1;
    Font->Glyphs[0].UnicodeCodePoint = 0;
    Font->Glyphs[0].BitmapID.Value = 0;

    return(Font);
}

INTERNAL_FUNCTION loaded_voxel_atlas*
LoadVoxelAtlas(char* FileName, uint32 AtlasWidth, uint32 OneTextureWidth)
{
    /*AtlasWidth must be multiple of OneTextureWidth*/
    Assert((AtlasWidth & (OneTextureWidth - 1)) == 0);

    loaded_voxel_atlas* Atlas = (loaded_voxel_atlas*)malloc(sizeof(loaded_voxel_atlas));

    uint32 TexturesByWidth = AtlasWidth / OneTextureWidth;
    Atlas->MaxTextureCount = TexturesByWidth * TexturesByWidth;
    Atlas->TextureCount = 0;

    Atlas->AtlasWidth = AtlasWidth;
    Atlas->OneTextureWidth = OneTextureWidth;

    Atlas->TextureFileName = FileName;

    float UVOneTextureDelta = (float)OneTextureWidth / (float)AtlasWidth;

    for(int MaterialIndex = 0;
        MaterialIndex < VoxelMaterial_Count;
        MaterialIndex++)
    {
        for(int i = 0;
            i < VoxelFaceTypeIndex_Count;
            i++)
        {
            Atlas->Materials[MaterialIndex].Sets[i] = 0;
        }
    }

    return(Atlas);
}

INTERNAL_FUNCTION void
FinalizeFontKerning(loaded_font* Font){
#if USE_FONTS_FROM_WINDOWS
    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);

    DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
    KERNINGPAIR* KerningPairs = (KERNINGPAIR*)malloc(KerningPairCount * sizeof(KERNINGPAIR));
    GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);

    for(DWORD KernPairIndex = 0;
        KernPairIndex < KerningPairCount;
        KernPairIndex++)
    {
        KERNINGPAIR* Pair = KerningPairs + KernPairIndex;
        if((Pair->wFirst < ONE_PAST_MAX_FONT_CODEPOINT) &&
            (Pair->wSecond < ONE_PAST_MAX_FONT_CODEPOINT))
        {
            uint32 First = Font->GlyphIndexFromCodePoint[Pair->wFirst];
            uint32 Second = Font->GlyphIndexFromCodePoint[Pair->wSecond];
            if((First != 0) && (Second != 0)){
                Font->HorizontalAdvance[First * Font->MaxGlyphCount + Second] += (float)Pair->iKernAmount;
            }
        }
    }

    free(KerningPairs);

#else
    for(uint32 FirstGlyphIndex = 1;
        FirstGlyphIndex < Font->GlyphCount;
        FirstGlyphIndex++)
    {
        dda_font_glyph First = Font->Glyphs[FirstGlyphIndex];

        for(uint32 SecondGlyphIndex = 1;
            SecondGlyphIndex < Font->GlyphCount;
            SecondGlyphIndex++)
        {
            dda_font_glyph Second = Font->Glyphs[SecondGlyphIndex];

            float KernAdvance = Font->Scale * stbtt_GetCodepointKernAdvance(
                &Font->FontInfo,
                First.UnicodeCodePoint,
                Second.UnicodeCodePoint);

            Font->HorizontalAdvance[SecondGlyphIndex * Font->MaxGlyphCount + FirstGlyphIndex] += KernAdvance;
        }
    }
#endif
}

INTERNAL_FUNCTION void 
FreeFont(loaded_font* Font){
    if(Font){
#if USE_FONTS_FROM_WINDOWS
        DeleteObject(Font->Win32Handle);
#else
        free(Font->FileContents);
#endif
        free(Font->Glyphs);
        free(Font->HorizontalAdvance);
        free(Font->GlyphIndexFromCodePoint);
        free(Font);
    }
}

INTERNAL_FUNCTION void InitializeFontDC(){

#if USE_FONTS_FROM_WINDOWS
    GlobalFontDeviceContext = CreateCompatibleDC(GetDC(0));

    BITMAPINFO Info = {};
    Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
    Info.bmiHeader.biWidth = MAX_FONT_WIDTH;
    Info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;
    Info.bmiHeader.biSizeImage = 0;
    Info.bmiHeader.biXPelsPerMeter = 0;
    Info.bmiHeader.biYPelsPerMeter = 0;
    Info.bmiHeader.biClrUsed = 0;
    Info.bmiHeader.biClrImportant = 0;
    HBITMAP Bitmap = CreateDIBSection(GlobalFontDeviceContext, &Info, DIB_RGB_COLORS, &GlobalFontBits, 0, 0);
    SelectObject(GlobalFontDeviceContext, Bitmap);
    SetBkColor(GlobalFontDeviceContext, RGB(0, 0, 0));  
#endif
}

INTERNAL_FUNCTION loaded_bitmap
LoadGlyphBitmap(loaded_font* Font, uint32 Codepoint, dda_asset* Asset){
    loaded_bitmap Result = {};

    uint32 GlyphIndex = Font->GlyphIndexFromCodePoint[Codepoint];

#if USE_FONTS_FROM_WINDOWS
    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);

    memset(GlobalFontBits, 0x00, MAX_FONT_WIDTH * MAX_FONT_HEIGHT * sizeof(uint32));

    wchar_t FakePoint = (wchar_t)Codepoint;

    SIZE Size;
    GetTextExtentPoint32W(GlobalFontDeviceContext, &FakePoint, 1, &Size);

    int PreStepX = 128;

    int BoundWidth = Size.cx + 2 * PreStepX;
    if(BoundWidth > MAX_FONT_WIDTH){
        BoundWidth = MAX_FONT_WIDTH;
    }
    int BoundHeight = Size.cy;
    if(BoundHeight > MAX_FONT_HEIGHT){
        BoundHeight = MAX_FONT_HEIGHT;
    }

    SetTextColor(GlobalFontDeviceContext, RGB(255, 255, 255));
    TextOutW(GlobalFontDeviceContext, PreStepX, 0, &FakePoint, 1);

    int MinX = 10000;
    int MinY = 10000;
    int MaxX = -10000;
    int MaxY = -10000;

    for(int j  = 0; j < BoundHeight; j++){

        uint32 *Pixel = (uint32*)GlobalFontBits + (MAX_FONT_HEIGHT - 1 - j) * MAX_FONT_WIDTH;
        
        for(int i = 0; i < BoundWidth; i++){
            if(*Pixel != 0)
            {
                if(MinX > i)
                {
                    MinX = i;                    
                }

                if(MinY > j)
                {
                    MinY = j;                    
                }
                
                if(MaxX < i)
                {
                    MaxX = i;                    
                }

                if(MaxY < j)
                {
                    MaxY = j;                    
                }
            }

            ++Pixel;
        }
    }

    float KerningChange = 0;
    if(MinX <= MaxX){
        int Width = (MaxX - MinX) + 1;
        int Height = (MaxY - MinY) + 1;

        Result.Width = Width + 2;
        Result.Height = Height + 2;
        Result.Memory = malloc(Result.Height * Result.Width * 4);
        Result.Free = Result.Memory;

        memset(Result.Memory, 0, Result.Height * Result.Width * 4);

        uint8* DestRow = (uint8*)Result.Memory + (Result.Height - 1 - 1) * Result.Width * 4;
        uint32* SourceRow = (uint32*)GlobalFontBits + (MAX_FONT_HEIGHT - 1 - MinY) * MAX_FONT_WIDTH;
        
        for(int j = MinY; j < MaxY; j++){
            
            uint32* Source = (uint32*)SourceRow + MinX;
            uint32* Dest = (uint32*)DestRow + 1;    

            for(int i = MinX; i < MaxX; i++){
                uint32 Pixel = *Source;

                float Gray = (float)(Pixel & 0xFF);
                vec4 Texel = {255.0f, 255.0f, 255.0f, Gray};
                Texel = SRGB255ToLinear1(Texel);
                Texel.rgb *= Texel.a;
                Texel = Linear1ToSRGB255(Texel);

               *Dest++ = (((uint32)(Texel.a + 0.5f) << 24) |
                   ((uint32)(Texel.r + 0.5f) << 16) |
                   ((uint32)(Texel.g + 0.5f) << 8) |
                   ((uint32)(Texel.b + 0.5f) << 0));

               Source++;
            }

            DestRow -= Result.Width * 4;
            SourceRow -= MAX_FONT_WIDTH;
        }

        Asset->Bitmap.AlignPercentage[0] = (1.0f) / (float)Result.Width;
        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (float)Result.Height;
    
        KerningChange = (float)(MinX - PreStepX);
    }

    INT ThisWidth;
    GetCharWidth32W(GlobalFontDeviceContext, Codepoint, Codepoint, &ThisWidth);
    float CharAdvance = (float)ThisWidth;

#else
    int Width, Height, XOffset, YOffset;
    uint8* MonoBitmap = stbtt_GetCodepointBitmap(
        &Font->FontInfo,
        0,
        Font->Scale,
        Codepoint,
        &Width,
        &Height,
        &XOffset,
        &YOffset);

    Result.Width = Width + 2;
    Result.Height = Height + 2;
    Result.Memory = malloc(Result.Width * Result.Height * 4);
    Result.Free = Result.Memory;

    memset(Result.Memory, 0, Result.Width * Result.Height * 4);

    uint8* Source = MonoBitmap;
    uint8* DestRow = (uint8*)Result.Memory + (Result.Height - 1 - 1) * Result.Width * 4;
    for(int j = 0; j < Height; j++){
        uint32* Dest = ((uint32*)DestRow) + 1;
        for(int i = 0; i < Width; i++){
            uint32 Pixel = *Source;

            float Gray = (float)(Pixel & 0xFF);
            vec4 Texel = {255.0f, 255.0f, 255.0f, Gray};
            Texel = SRGB255ToLinear1(Texel);
            Texel.rgb *= Texel.a;
            Texel = Linear1ToSRGB255(Texel);

           *Dest++ = (((uint32)(Texel.a + 0.5f) << 24) |
               ((uint32)(Texel.r + 0.5f) << 16) |
               ((uint32)(Texel.g + 0.5f) << 8) |
               ((uint32)(Texel.b + 0.5f) << 0));

           Source++;
        }

        DestRow -= Result.Width * 4;
    }

    stbtt_FreeBitmap(MonoBitmap, 0);

    Asset->Bitmap.AlignPercentage[0] = (1.0f) / (float)Result.Width;
    Asset->Bitmap.AlignPercentage[1] = (1.0f + (Height + YOffset)) / (float)Result.Height;

    float KerningChange = (float)XOffset;

    int Advance;
    stbtt_GetCodepointHMetrics(&Font->FontInfo, Codepoint, &Advance, 0);
    float CharAdvance = Font->Scale * (float)Advance;
#endif

    for(uint32 OtherGlyphIndex = 0;
        OtherGlyphIndex < Font->MaxGlyphCount;
        OtherGlyphIndex++)
    {
        Font->HorizontalAdvance[GlyphIndex * Font->MaxGlyphCount + OtherGlyphIndex] += CharAdvance - KerningChange;
        if(OtherGlyphIndex != 0){
            Font->HorizontalAdvance[OtherGlyphIndex * Font->MaxGlyphCount + GlyphIndex] += KerningChange;
        }
    }

    return(Result);
}

struct riff_iterator{
    uint8* At;
    uint8* Stop;
};

inline riff_iterator ParseChunkAt(void* At, void* Stop){
    riff_iterator Iter;

    Iter.At = (uint8*)At;
    Iter.Stop = (uint8*)Stop;

    return(Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter){
    WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
    uint32 Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(WAVE_chunk) + Size;

    return(Iter);
}

inline bool32
IsValid(riff_iterator Iter){
    bool32 Result = (Iter.At < Iter.Stop);

    return(Result);
}

inline void*
GetChunkData(riff_iterator Iter){
    void* Result = (Iter.At + sizeof(WAVE_chunk));

    return(Result);
}

inline uint32
GetType(riff_iterator Iter){
    WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
    uint32 Result = Chunk->Id;

    return(Result);
}

inline uint32
GetChunkDataSize(riff_iterator Iter){
    WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
    uint32 Result = Chunk->Size;

    return(Result);
}

struct loaded_sound{
	uint32 SampleCount;
	uint32 ChannelCount;
	int16* Samples[2];

	void* Free;
};

INTERNAL_FUNCTION loaded_sound
LoadWAV(char* FileName, uint32 SectionFirstSampleIndex, uint32 SectionSampleCount){
    loaded_sound Result = {};

    loaded_file ReadResult = ReadEntireFile(FileName);
    if(ReadResult.DataSize != 0){

        Result.Free = ReadResult.Data;

        WAVE_header* Header = (WAVE_header*)ReadResult.Data;
        Assert(Header->RIFFID  == WAVE_ChunkID_RIFF);
        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

        int16* SampleData = 0;
        uint32 ChannelCount = 0;
        uint32 SampleDataSize = 0;

        for(riff_iterator Iter = ParseChunkAt(Header + 1, (uint8*)(Header + 1) + Header->Size - 4);
            IsValid(Iter);
            Iter = NextChunk(Iter))
        {
            switch(GetType(Iter)){

                case WAVE_ChunkID_fmt:{
                    WAVE_fmt* fmt = (WAVE_fmt*)GetChunkData(Iter);
                    ChannelCount = fmt->nChannels;
                }break;

                case WAVE_ChunkID_data:{
                    SampleData = (int16 *)GetChunkData(Iter);
                    SampleDataSize = GetChunkDataSize(Iter);
                }break;
            }
        }

        Assert(ChannelCount && SampleData);

        Result.ChannelCount = ChannelCount;
        uint32 SampleCount = SampleDataSize / (ChannelCount * sizeof(int16));

        int16* NewChannel0 = 0;
        int16* NewChannel1 = 0;
        
        if(ChannelCount == 1){

            NewChannel0 = (int16*)malloc(SampleCount * sizeof(int16) + 8 * sizeof(int16));

            Result.Samples[0] = NewChannel0;
            Result.Samples[1] = NewChannel1;

            for(uint32 SampleIndex = SectionFirstSampleIndex;
                SampleIndex < SectionFirstSampleIndex + SectionSampleCount;
                SampleIndex++)
            {
                NewChannel0[SampleIndex - SectionFirstSampleIndex] = SampleData[SampleIndex];
            }
        }
        else if(ChannelCount == 2){


            NewChannel0 = (int16*)malloc(SampleCount * sizeof(int16) + 8 * sizeof(int16));
            NewChannel1 = (int16*)malloc(SampleCount * sizeof(int16) + 8 * sizeof(int16));

            Result.Samples[0] = NewChannel0;
            Result.Samples[1] = NewChannel1;       
            
            for(uint32 SampleIndex = SectionFirstSampleIndex;
                SampleIndex < SectionFirstSampleIndex + SectionSampleCount;
                SampleIndex++)
            {
                NewChannel0[SampleIndex - SectionFirstSampleIndex] = SampleData[SampleIndex * 2];
                NewChannel1[SampleIndex - SectionFirstSampleIndex] = SampleData[SampleIndex * 2 + 1];
            }
        }
        else{
            Assert(!"Invalid channel count in WAV file");
        }

        bool32 AtEnd = true;

        if(SectionSampleCount){
            Assert((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
            AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
            SampleCount = SectionSampleCount;
        }

        if(AtEnd){
            for(uint32 ChannelIndex = 0;
                ChannelIndex < Result.ChannelCount;
                ChannelIndex++)
            {
                for(uint32 SampleIndex = SampleCount;
                    SampleIndex < (SampleCount + 8);
                    SampleIndex++)
                {
                    Result.Samples[ChannelIndex][SampleIndex] = 0;
                }
            }
        }

        Result.SampleCount = SampleCount;
    }

    return(Result);
}

INTERNAL_FUNCTION void 
BeginAssetType(game_assets* Assets, asset_type_id TypeID){
	Assert(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
	Assets->DEBUGAssetType->TypeID = TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

struct added_asset{
    uint32 ID;
    dda_asset* DDA;
    asset_source* Source;
};

INTERNAL_FUNCTION added_asset AddAsset(game_assets* Assets){
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

    uint32 Index = Assets->DEBUGAssetType->OnePastLastAssetIndex++;
    asset_source* Source = Assets->AssetSources + Index;
    dda_asset* DDA = Assets->Assets + Index;
    DDA->FirstTagIndex = Assets->TagCount;
    DDA->OnePastLastTagIndex = DDA->FirstTagIndex;

    Assets->AssetIndex = Index;

    added_asset Result;
    Result.ID = Index;
    Result.DDA = DDA;
    Result.Source = Source;
    return(Result);
}

INTERNAL_FUNCTION bitmap_id 
AddCharacterAsset(game_assets* Assets, loaded_font* Font, uint32 Codepoint){
    added_asset Asset = AddAsset(Assets);
    Asset.DDA->Bitmap.AlignPercentage[0] = 0.0f;
    Asset.DDA->Bitmap.AlignPercentage[1] = 0.0f;
    Asset.DDA->Bitmap.BitmapType = DDABitmap_FontGlyph;
    Asset.Source->Type = AssetType_FontGlyph;
    Asset.Source->Glyph.Font = Font;
    Asset.Source->Glyph.Codepoint = Codepoint;

    bitmap_id Result = {Asset.ID};

    Assert(Font->GlyphCount < Font->MaxGlyphCount);
    uint32 GlyphIndex = Font->GlyphCount++;
    dda_font_glyph* Glyph = Font->Glyphs + GlyphIndex;
    Glyph->UnicodeCodePoint = Codepoint;
    Glyph->BitmapID = Result;
    Font->GlyphIndexFromCodePoint[Codepoint] = GlyphIndex;

    if(Font->OnePastHighestCodepoint <= Codepoint){
        Font->OnePastHighestCodepoint = Codepoint + 1;
    }

    return(Result);
}

INTERNAL_FUNCTION bitmap_id
AddVoxelAtlasTextureAsset(game_assets* Assets, loaded_voxel_atlas* Atlas){
    added_asset Asset = AddAsset(Assets);

    Asset.DDA->Bitmap.AlignPercentage[0] = 0.0f;
    Asset.DDA->Bitmap.AlignPercentage[1] = 0.0f;
    Asset.DDA->Bitmap.BitmapType = DDABitmap_VoxelAtlas;

    Asset.Source->Type = AssetType_VoxelAtlasTexture;
    Asset.Source->VoxelAtlasTexture.Atlas = Atlas;
    Asset.Source->VoxelAtlasTexture.FileName = Atlas->TextureFileName;

    bitmap_id Result = {Asset.ID};

    Atlas->AtlasTexture.BitmapID = Result;

    return(Result);
}


#if BUILD_WITH_ASSIMP
INTERNAL_FUNCTION animated_model_id
AddAnimatedModelAsset(
	game_assets* Assets, 
	loaded_animated_model* AnimatedModel) 
{
	added_asset Asset = AddAsset(Assets);

	Asset.Source->Type = AssetType_AnimatedModel;
	Asset.Source->AnimatedModel.AnimatedModel = AnimatedModel;

	Asset.DDA->AnimatedModel.AnimationsCount = 0;

	animated_model_id Result = { Asset.ID };
	return(Result);
}

INTERNAL_FUNCTION void
AddAnimationToModelAsset(
	loaded_animated_model* Model, 
	loaded_animation* Animation)
{
	Assert((Model->AnimationsCount) < MAX_ANIMATIONS_PER_MODEL);
	Model->Animations[Model->AnimationsCount++] = Animation;
}

INTERNAL_FUNCTION void
AddAnimationsToModelAsset(
	loaded_animated_model* AnimatedModel,
	char* FileName,
	asset_animation_type AnimationType)
{
	loaded_animations_result Result = LoadSkeletalAnimation(
		FileName,
		&AnimatedModel->Skeleton,
		AnimationType);

	for (uint32 AnimationIndex = 0;
		AnimationIndex < Result.AnimationsCount;
		AnimationIndex++)
	{
		AddAnimationToModelAsset(AnimatedModel, &Result.Animations[AnimationIndex]);
	}
}

INTERNAL_FUNCTION void 
AddSkinnedMeshToModelAsset(
	loaded_animated_model* AnimatedModel,
	char* FileName)
{
	LoadSkinnedMeshDataFromAssimp(
		&AnimatedModel->Mesh,
		FileName,
		&AnimatedModel->Skeleton);
}
#endif

INTERNAL_FUNCTION voxel_atlas_id
AddVoxelAtlasAsset(
    game_assets* Assets,
    loaded_voxel_atlas* Atlas)
{
    added_asset Asset = AddAsset(Assets);

    //TODO
    Asset.Source->Type = AssetType_VoxelAtlas;
    Asset.Source->VoxelAtlas.Atlas = Atlas;

    Asset.DDA->VoxelAtlas.AtlasWidth = Atlas->AtlasWidth;
    Asset.DDA->VoxelAtlas.OneTextureWidth = Atlas->OneTextureWidth;
    Asset.DDA->VoxelAtlas.BitmapID = Atlas->AtlasTexture.BitmapID;

    voxel_atlas_id Result = {Asset.ID};
    return(Result);
}

inline void 
DescribeVoxelAtlasTexture(
    loaded_voxel_atlas* Atlas, 
    voxel_mat_type MaterialType,
    voxel_face_type_index FaceTypeIndex,
    int CurrTextureIndex)
{
    uint32 TexturesByWidth = Atlas->AtlasWidth / Atlas->OneTextureWidth;

    Assert(CurrTextureIndex < Atlas->MaxTextureCount);

    voxel_tex_coords_set* MatTexSet = &Atlas->Materials[MaterialType];

    switch(FaceTypeIndex){
        case(VoxelFaceTypeIndex_Top):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Top] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_Bottom):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Bottom] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_Left):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Left] = CurrTextureIndex;            
        }break;

        case(VoxelFaceTypeIndex_Right):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Right] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_Front):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Front] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_Back):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Back] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_All):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Bottom] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Top] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Left] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Right] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Front] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Back] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_Side):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Left] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Right] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Front] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Back] = CurrTextureIndex;
        }break;

        case(VoxelFaceTypeIndex_TopBottom):{
            MatTexSet->Sets[VoxelFaceTypeIndex_Bottom] = CurrTextureIndex;
            MatTexSet->Sets[VoxelFaceTypeIndex_Top] = CurrTextureIndex;
        }break;

        default:{
            INVALID_CODE_PATH;
        }break;
    }
}

INTERNAL_FUNCTION void
DescribeByIndex(
    loaded_voxel_atlas* Atlas, 
    int HorzIndex, int VertIndex,
    voxel_mat_type MaterialType,
    voxel_face_type_index FaceTypeIndex)
{
    DescribeVoxelAtlasTexture(Atlas, MaterialType, FaceTypeIndex, VertIndex * 16 + HorzIndex);
}

INTERNAL_FUNCTION bitmap_id
AddBitmapAsset(
    game_assets* Assets,
    char* FileName,
    vec2 AlignPercentage = Vec2(0.5f))
{
    added_asset Asset = AddAsset(Assets);

    Asset.DDA->Bitmap.AlignPercentage[0] = AlignPercentage.x;
    Asset.DDA->Bitmap.AlignPercentage[1] = AlignPercentage.y;
    Asset.DDA->Bitmap.BitmapType = DDABitmap_Bitmap;

    Asset.Source->Type = AssetType_Bitmap;
    Asset.Source->Bitmap.FileName = FileName;

    bitmap_id Result = {Asset.ID};
    return(Result);
}

INTERNAL_FUNCTION sound_id
AddSoundAsset(
    game_assets* Assets,
    char* FileName,
    unsigned int FirstSampleIndex = 0,
    unsigned int SampleCount = 0)
{
    added_asset Asset = AddAsset(Assets);

    Asset.DDA->Sound.SampleCount = SampleCount;
    Asset.DDA->Sound.Chain = DDASoundChain_None;

    Asset.Source->Type = AssetType_Sound;
    Asset.Source->Sound.FileName = FileName;
    Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;

    sound_id Result = {Asset.ID};
    return(Result);
}

INTERNAL_FUNCTION font_id
AddFontAsset(game_assets* Assets, loaded_font* Font){
    added_asset Asset = AddAsset(Assets);
    Asset.DDA->Font.OnePastHighestCodepoint = Font->OnePastHighestCodepoint;
    Asset.DDA->Font.GlyphCount = Font->GlyphCount;
#if USE_FONTS_FROM_WINDOWS
    Asset.DDA->Font.AscenderHeight = (float)Font->TextMetric.tmAscent;
    Asset.DDA->Font.DescenderHeight = (float)Font->TextMetric.tmDescent;
    Asset.DDA->Font.ExternalLeading = (float)Font->TextMetric.tmExternalLeading;
#else
    Asset.DDA->Font.AscenderHeight = Font->AscenderHeight;
    Asset.DDA->Font.DescenderHeight = Font->DescenderHeight;
    Asset.DDA->Font.ExternalLeading = Font->ExternalLeading;
#endif
    Asset.Source->Type = AssetType_Font;
    Asset.Source->Font.Font = Font;

    font_id Result = {Asset.ID};
    return(Result);
}


INTERNAL_FUNCTION void
AddTag(game_assets* Assets, asset_tag_id ID, float Value){
	Assert(Assets->AssetIndex);

	dda_asset* DDA = Assets->Assets + Assets->AssetIndex;
	++DDA->OnePastLastTagIndex;
	dda_tag* Tag = Assets->Tags + Assets->TagCount++;

	Tag->ID = ID;
	Tag->Value = Value;
}

INTERNAL_FUNCTION void
EndAssetType(game_assets* Assets){
	Assert(Assets->DEBUGAssetType);
	Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
	Assets->DEBUGAssetType = 0;
	Assets->AssetIndex = 0;
}

INTERNAL_FUNCTION void
WriteDDA(game_assets* Assets, char* FileName){
    FILE* fp = fopen(FileName, "wb");
    if(fp){
        dda_header Header = {};
        Header.MagicValue = DDA_MAGIC_VALUE;
        Header.Version = DDA_VERSION;
        Header.TagCount = Assets->TagCount;
        Header.AssetTypeCount = Asset_Count;
        Header.AssetCount = Assets->AssetCount;

        uint32 TagArraySize = Header.TagCount * sizeof(dda_tag);
        uint32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(dda_asset_type);
        uint32 AssetArraySize = Header.AssetCount * sizeof(dda_asset);

        Header.TagOffset = sizeof(Header);
        Header.AssetTypeOffset = Header.TagOffset + TagArraySize;
        Header.AssetOffset = Header.AssetTypeOffset + AssetTypeArraySize;

        fwrite(&Header, sizeof(Header), 1, fp);
        fwrite(Assets->Tags, TagArraySize, 1, fp);
        fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, fp);
        fseek(fp, AssetArraySize, SEEK_CUR);

        for(uint32 AssetIndex = 1;
            AssetIndex < Header.AssetCount;
            AssetIndex++)
        {
            asset_source* Source = Assets->AssetSources + AssetIndex;
            dda_asset* DDA = Assets->Assets + AssetIndex;

            DDA->DataOffset = ftell(fp);

            switch(Source->Type){
                case(AssetType_Sound):{
                    loaded_sound WAV = LoadWAV(
                        Source->Sound.FileName,
                        Source->Sound.FirstSampleIndex,
                        DDA->Sound.SampleCount);

                    DDA->Sound.SampleCount = WAV.SampleCount;
                    DDA->Sound.ChannelCount = WAV.ChannelCount;

                    for(uint32 ChannelIndex = 0;
                        ChannelIndex < WAV.ChannelCount;
                        ChannelIndex++)
                    {
                        fwrite(WAV.Samples[ChannelIndex], DDA->Sound.SampleCount * sizeof(int16), 1, fp);
                    }

                    free(WAV.Free);
                }break;

                case(AssetType_VoxelAtlasTexture):
                case(AssetType_FontGlyph):
                case(AssetType_Bitmap):{
                    //loaded_bitmap Bitmap = LoadBMP(Source->FileName);

                    loaded_bitmap Bitmap;
                    if(Source->Type == AssetType_FontGlyph){
                        Bitmap = LoadGlyphBitmap(Source->Glyph.Font, Source->Glyph.Codepoint, DDA);
                    }
                    else if(Source->Type = AssetType_VoxelAtlasTexture){
                        Bitmap = LoadBMP(Source->VoxelAtlasTexture.FileName);
                    }
                    else{
                        Assert(Source->Type == AssetType_Bitmap);
                        Bitmap = LoadBMP(Source->Bitmap.FileName);
                    }

                    DDA->Bitmap.Dimension[0] = Bitmap.Width;
                    DDA->Bitmap.Dimension[1] = Bitmap.Height;

                    fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, fp);

                    free(Bitmap.Free);
                }break;

				case(AssetType_Mesh): {
#if BUILD_WITH_ASSIMP
					loaded_mesh* Mesh = Source->Mesh.Mesh;

					dda_mesh_header MeshHeader;
					MeshHeader.IndicesCount = Mesh->IndicesCount;
					MeshHeader.VerticesCount = Mesh->VerticesCount;
					MeshHeader.MeshType = Mesh->Type;

					u8* VerticesToWrite = 0;
					u32 VerticesToWriteSize = 0;
					switch (MeshHeader.MeshType) {
						case(DDAMeshType_Simple): {
							MeshHeader.VertexStructureSize = sizeof(simple_vertex);
							VerticesToWrite = (u8*)Mesh->SimpleVertices;
						}break;

						case(DDAMeshType_Skinned): {
							MeshHeader.VertexStructureSize = sizeof(skinned_vertex);
							VerticesToWrite = (u8*)Mesh->SkinnedVertices;
						}break;

						INVALID_DEFAULT_CASE;
					}
					
					VerticesToWriteSize = Mesh->VerticesCount * MeshHeader.VertexStructureSize;

					//NOTE(Dima): First I write the mesh header.
					fwrite(&MeshHeader, 1, sizeof(dda_mesh_header), fp);

					//NOTE(DIMA): Then I write all the vertices
					fwrite(VerticesToWrite, VerticesToWriteSize, 1, fp);

					//NOTE(Dima): Last I write indices. Index is uint32 value(4 bytes)
					fwrite(Mesh->Indices, Mesh->IndicesCount, sizeof(u32), fp);
#endif
				}break;

                case(AssetType_Animation):{
#if BUILD_WITH_ASSIMP
                    loaded_animation* Anim = Source->Animation.Animation;

                    Assert(Anim->JointAnimsCount <= DDA_ANIMATION_MAX_BONE_COUNT);

                    uint32 TotalFileSize = 0;

                    for(uint32 JointAnimIndex = 0;
                        JointAnimIndex < Anim->JointAnimsCount;
                        JointAnimIndex++)
                    {
                        joint_animation* JointAnim = &Anim->JointAnims[JointAnimIndex];

                        dda_joint_frames_header TempHeader;

                        TempHeader.BytesLength = JointAnim->TranslationFramesByteSize;
                        TempHeader.Type = JointFrameHeader_Translation;
                        fwrite(&TempHeader, sizeof(dda_joint_frames_header), 1, fp);
                        fwrite(JointAnim->TranslationFrames, JointAnim->TranslationFramesByteSize, 1, fp);
                        TotalFileSize += (sizeof(dda_joint_frames_header) + JointAnim->TranslationFramesByteSize);

                        TempHeader.BytesLength = JointAnim->RotationFramesByteSize;
                        TempHeader.Type = JointFrameHeader_Rotation;
                        fwrite(&TempHeader, sizeof(dda_joint_frames_header), 1, fp);
                        fwrite(JointAnim->RotationFrames, JointAnim->RotationFramesByteSize, 1, fp);
                        TotalFileSize += (sizeof(dda_joint_frames_header) + JointAnim->RotationFramesByteSize);

                        TempHeader.BytesLength = JointAnim->ScalingFramesByteSize;
                        TempHeader.Type = JointFrameHeader_Scaling;
                        fwrite(&TempHeader, sizeof(dda_joint_frames_header), 1, fp);
                        fwrite(JointAnim->ScalingFrames, JointAnim->ScalingFramesByteSize, 1, fp);
                        TotalFileSize = (sizeof(dda_joint_frames_header) + JointAnim->ScalingFramesByteSize);

                        free(JointAnim->Free);
                    }

                    DDA->Animation.TotalFileSize = TotalFileSize;
#endif
                }break;

				case(AssetType_AnimatedModel): {
#if BUILD_WITH_ASSIMP
                    loaded_animated_model* Model = Source->AnimatedModel.AnimatedModel;


#endif
				}break;

                case(AssetType_VoxelAtlas):{
                    loaded_voxel_atlas* Atlas = Source->VoxelAtlas.Atlas;

                    uint32 MaterialsSize = VoxelMaterial_Count * sizeof(voxel_tex_coords_set);
                    fwrite(Atlas->Materials, MaterialsSize, 1, fp);
                }break;

                case(AssetType_Font):{
                    loaded_font* Font = Source->Font.Font;
                    
                    FinalizeFontKerning(Font);

                    uint32 GlyphsSize = Font->GlyphCount * sizeof(dda_font_glyph);
                    fwrite(Font->Glyphs, GlyphsSize, 1, fp);

                    uint8* HorizontalAdvance = (uint8*)Font->HorizontalAdvance;
                    for(uint32 GlyphIndex = 0;
                        GlyphIndex < Font->GlyphCount;
                        GlyphIndex++)
                    {
                        /*Here we write one row of horizontal advances for GlyphIndex*/
                        uint32 HorizontalAdvanceSliceSize = sizeof(float) * Font->GlyphCount;
                        fwrite(HorizontalAdvance, HorizontalAdvanceSliceSize, 1, fp);
                        HorizontalAdvance += sizeof(float) * Font->MaxGlyphCount;
                    }
                }break;

                default:{
                    INVALID_CODE_PATH;
                }break;
            }


        }
        
        fseek(fp, (uint32)Header.AssetOffset, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, fp);

        fclose(fp);
    }
    else{
        printf("Error while opening the file T_T\n");
    }

}

INTERNAL_FUNCTION void Initialize(game_assets* Assets){
    Assets->TagCount = 1;
    Assets->AssetCount = 1;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;

    Assets->AssetTypeCount = Asset_Count;
    memset(Assets->AssetTypes, 0, sizeof(Assets->AssetTypes));
}

INTERNAL_FUNCTION void WriteModels(){
#if BUILD_WITH_ASSIMP
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

	loaded_animated_model OldSchoolDima = {};
	AddSkinnedMeshToModelAsset(&OldSchoolDima, "../../IvanEngine/Data/Animations/Dima.fbx");
	AddAnimationsToModelAsset(&OldSchoolDima, "../../IvanEngine/Data/Animations/Dima_RunF01.fbx", AssetAnimationType_RunForward);
	AddAnimationsToModelAsset(&OldSchoolDima, "../../IvanEngine/Data/Animations/Dima_Idle01Idle01.fbx", AssetAnimationType_Idle00);

	BeginAssetType(Assets, Asset_OldSchoolDima);
	AddAnimatedModelAsset(Assets, &OldSchoolDima);
	EndAssetType(Assets);

    WriteDDA(Assets, "../../IvanEngine/Data/asset_pack_animations.dda");
    printf("Animation assets written successfully :D");
#endif
}


INTERNAL_FUNCTION void WriteFonts(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    loaded_font *Fonts[] = {
        LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", 20),
        LoadFont("c:/Windows/Fonts/LiberationMono-Regular.ttf", "Liberation Mono", 20),
        LoadFont("c:/Windows/Fonts/Antique-Olive-Std-Nord-Italic.ttf", "Antique Olive Std Nord", 20),
    };

    BeginAssetType(Assets, Asset_FontGlyph);
    for(uint32 FontIndex = 0;
        FontIndex < ArrayCount(Fonts);
        FontIndex++)
    {
        loaded_font* Font = Fonts[FontIndex];

        AddCharacterAsset(Assets, Font, ' ');
        for(uint32 Character = '!';
            Character <= '~';
            Character++)
        {
            AddCharacterAsset(Assets, Font, Character);
        }
    }
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Font);
    AddFontAsset(Assets, Fonts[0]);
    AddTag(Assets, Tag_FontType, FontType_Default);
    AddFontAsset(Assets, Fonts[1]);
    AddTag(Assets, Tag_FontType, FontType_Debug);
    AddFontAsset(Assets, Fonts[2]);
    AddTag(Assets, Tag_FontType, FontType_Forsazh);
    EndAssetType(Assets);

    WriteDDA(Assets, "../../IvanEngine/Data/asset_pack_fonts.dda");
	printf("Font assets written successfully :D\n");
}

INTERNAL_FUNCTION void WriteVoxelAtlases(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    loaded_voxel_atlas* Atlases[] = {
        LoadVoxelAtlas("../../IvanEngine/Data/Images/MyVoxelAtlas/VoxelAtlas.png", 256, 16),
        LoadVoxelAtlas("../../IvanEngine/Data/Images/terrain.png", 256, 16),
    };

    loaded_voxel_atlas* Atlas = Atlases[0];
    DescribeByIndex(Atlas, 0, 0, VoxelMaterial_GrassyGround, VoxelFaceTypeIndex_Top);
    DescribeByIndex(Atlas, 1, 0, VoxelMaterial_GrassyGround, VoxelFaceTypeIndex_Side);
    DescribeByIndex(Atlas, 2, 0, VoxelMaterial_GrassyGround, VoxelFaceTypeIndex_Bottom);
    DescribeByIndex(Atlas, 2, 0, VoxelMaterial_Ground, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 3, 0, VoxelMaterial_Tree, VoxelFaceTypeIndex_Side);
    DescribeByIndex(Atlas, 4, 0, VoxelMaterial_Tree, VoxelFaceTypeIndex_TopBottom);
    DescribeByIndex(Atlas, 5, 0, VoxelMaterial_Stone, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 6, 0, VoxelMaterial_Sand, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 7, 0, VoxelMaterial_Leaves, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 0, 1, VoxelMaterial_SnowGround, VoxelFaceTypeIndex_Top);
    DescribeByIndex(Atlas, 1, 1, VoxelMaterial_SnowGround, VoxelFaceTypeIndex_Side);
    DescribeByIndex(Atlas, 2, 1, VoxelMaterial_SnowGround, VoxelFaceTypeIndex_Bottom);
    DescribeByIndex(Atlas, 2, 1, VoxelMaterial_WinterGround, VoxelFaceTypeIndex_All);
 
    Atlas = Atlases[1];
    DescribeByIndex(Atlas, 0, 0, VoxelMaterial_GrassyGround, VoxelFaceTypeIndex_Top);
    DescribeByIndex(Atlas, 1, 0, VoxelMaterial_Stone, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 2, 0, VoxelMaterial_GrassyGround, VoxelFaceTypeIndex_Bottom);
    DescribeByIndex(Atlas, 2, 0, VoxelMaterial_Ground, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 3, 0, VoxelMaterial_GrassyGround, VoxelFaceTypeIndex_Side);
    DescribeByIndex(Atlas, 4, 1, VoxelMaterial_Tree, VoxelFaceTypeIndex_Side);
    DescribeByIndex(Atlas, 5, 1, VoxelMaterial_Tree, VoxelFaceTypeIndex_TopBottom);
    DescribeByIndex(Atlas, 5, 8, VoxelMaterial_Leaves, VoxelFaceTypeIndex_All);
    DescribeByIndex(Atlas, 0, 3, VoxelMaterial_Sand, VoxelFaceTypeIndex_All);


    BeginAssetType(Assets, Asset_VoxelAtlasTexture);
    for(uint32 VoxelAtlasIndex = 0;
        VoxelAtlasIndex < ArrayCount(Atlases);
        VoxelAtlasIndex++)
    {
        AddVoxelAtlasTextureAsset(Assets, Atlases[VoxelAtlasIndex]);
    }
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_VoxelAtlas);
    AddVoxelAtlasAsset(Assets, Atlases[0]);
    AddTag(Assets, Tag_VoxelAtlasType, VoxelAtlasType_Default);
    AddVoxelAtlasAsset(Assets, Atlases[1]);
    AddTag(Assets, Tag_VoxelAtlasType, VoxelAtlasType_Minecraft);
    EndAssetType(Assets);

	WriteDDA(Assets, "../../IvanEngine/Data/asset_pack_voxatl.dda");
    printf("Voxel Texture Atlas written successfully :D\n");
}

INTERNAL_FUNCTION void WriteHero(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    real32 AngleRight = 0.0f * IVAN_MATH_TAU;
    real32 AngleBack = 0.25f * IVAN_MATH_TAU;
    real32 AngleLeft = 0.5f * IVAN_MATH_TAU;
    real32 AngleFront = 0.75f * IVAN_MATH_TAU;

    vec2 HeroAlign = {0.5f, 1.0f - 0.156682029f};

    BeginAssetType(Assets, Asset_Head);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_right_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_back_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_left_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_front_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Cape);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_right_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_back_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_left_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_front_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Torso);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_right_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_back_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_left_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_hero_front_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    WriteDDA(Assets, "../../IvanEngine/Data/asset_pack_hero.dda");
	printf("Hero assets written successfully :D\n");
}

INTERNAL_FUNCTION void WriteNonHero(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    BeginAssetType(Assets, Asset_Backdrop);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test/test_background.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_LastOfUs);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/last.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tree);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/test2/tree00.bmp", Vec2(0.5f, 0.3f));
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_StarWars);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/star_wars.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Witcher);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/witcher.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Assassin);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/assassin.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Grass);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/grass00.bmp");
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/grass01.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Stone);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/ground00.bmp");
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/ground01.bmp");
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/ground02.bmp");
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/ground03.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tuft);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/tuft00.bmp");
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/tuft01.bmp");
    AddBitmapAsset(Assets, "../../IvanEngine/Data/HH/Test2/tuft02.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Heart);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/heart.png");
    EndAssetType(Assets);

    float ColorBlue = GetFloatRepresentOfColor(Vec3(0.0f, 0.0f, 1.0f));
    float ColorGreen = GetFloatRepresentOfColor(Vec3(0.0f, 1.0f, 0.0f));
    float ColorRed = GetFloatRepresentOfColor(Vec3(1.0f, 0.0f, 0.0f));

    BeginAssetType(Assets, Asset_Diamond);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/gemBlue.png");
    AddTag(Assets, Tag_Color, ColorBlue);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/gemGreen.png");
    AddTag(Assets, Tag_Color, ColorGreen);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/gemRed.png");
    AddTag(Assets, Tag_Color, ColorRed);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Bottle);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/potionBlue.png");
    AddTag(Assets, Tag_Color, ColorBlue);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/potionGreen.png");
    AddTag(Assets, Tag_Color, ColorGreen);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/potionRed.png");
    AddTag(Assets, Tag_Color, ColorRed);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Book);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/128/tome.png");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Particle);
    AddBitmapAsset(Assets, "../../IvanEngine/Data/Images/ShockSpell.png");
    EndAssetType(Assets);
    
    WriteDDA(Assets, "../../IvanEngine/Data/asset_pack_non_hero.dda");
	printf("Non-hero assets written successfully :D\n");
}

INTERNAL_FUNCTION void WriteSounds(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    BeginAssetType(Assets, Asset_Bloop);
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/bloop_00.wav");
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/bloop_01.wav");
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/bloop_02.wav");
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/bloop_03.wav");
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/bloop_04.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Crack);
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/crack_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Drop);
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/drop_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Glide);
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/glide_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/puhp_00.wav");
    AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/puhp_01.wav");
    EndAssetType(Assets);

    uint32 OneMusicChunk = 10 * 44100;
    uint32 TotalMusicSampleCount = 7468095;
    BeginAssetType(Assets, Asset_Music);
    for(uint32 FirstSampleIndex = 0;
        FirstSampleIndex < TotalMusicSampleCount;
        FirstSampleIndex += OneMusicChunk)
    {
        uint32 SampleCount;
        if(FirstSampleIndex + OneMusicChunk < TotalMusicSampleCount){
            SampleCount = OneMusicChunk;
        }
        else{
            SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        }

        sound_id ThisMusic = AddSoundAsset(Assets, "../../IvanEngine/Data/HH/test3/music_test.wav", FirstSampleIndex, SampleCount);
        if((FirstSampleIndex + OneMusicChunk) < TotalMusicSampleCount){
            Assets->Assets[ThisMusic.Value].Sound.Chain = DDASoundChain_Advance;
        }
    }
    EndAssetType(Assets);

    WriteDDA(Assets, "../../IvanEngine/Data/asset_pack_sounds.dda");
	printf("Sound assets written successfully :D\n");
}

int main(int ArgCount, char** Args){

    InitializeFontDC();

    WriteHero();
    WriteNonHero();
    WriteSounds();
    WriteFonts();
    WriteVoxelAtlases();

    WriteModels();

    system("pause");
	return(0);
}
