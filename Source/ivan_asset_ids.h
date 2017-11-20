#ifndef IVAN_ASSET_IDS
#define IVAN_ASSET_IDS

enum asset_font_type{
	FontType_Default = 0,
	FontType_Debug = 10,
	FontType_Forsazh = 20,
};

enum asset_voxel_atlas_type{
	VoxelAtlasType_Default = 0,
	VoxelAtlasType_Minecraft = 1,
};

//NOTE(Dima): Probably need to rebuild assets when change this enum
enum asset_animation_type {
	AssetAnimationType_Unknown,

	AssetAnimationType_RunForward,
	AssetAnimationType_RunLeft,
	AssetAnimationType_RunRight,
	AssetAnimationType_RunBack,

	AssetAnimationType_WalkForward,
	AssetAnimationType_WalkLeft,
	AssetAnimationType_WalkRight,
	AssetAnimationType_WalkBack,

	/*NOTE(Dima): Add as much as you need xD*/
	AssetAnimationType_Idle00,
	AssetAnimationType_Idle01,
	AssetAnimationType_Idle02,
	AssetAnimationType_Idle04,
	AssetAnimationType_Idle05,
	AssetAnimationType_Idle06,
	AssetAnimationType_Idle07,
	AssetAnimationType_Idle08,

	AssetAnimationType_Atack01,
	AssetAnimationType_Atack02,
	AssetAnimationType_Atack03,

	/*NOTE(Dima): Add animations type above this line*/
	AssetAnimationType_Count,
};

enum asset_tag_id{	
	Tag_FacingDirection,

	Tag_Color,

	Tag_UnicodeCodepoint,
	Tag_FontType,

	Tag_VoxelAtlasType,

	Tag_Count,
};

enum asset_type_id{
	Asset_None,

	Asset_LastOfUs,
	Asset_StarWars,
	Asset_Witcher,
	Asset_Assassin,
	Asset_Tree,
	Asset_Backdrop,	

	Asset_Particle,

	Asset_Heart,
	Asset_Diamond,
	Asset_Book,
	Asset_Bottle,

	Asset_Grass,
	Asset_Tuft,
	Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

	Asset_Bloop,
	Asset_Crack,
	Asset_Drop,
	Asset_Glide,
	Asset_Music,
	Asset_Puhp,

	Asset_Font,
	Asset_FontGlyph,

	Asset_VoxelAtlas,
	Asset_VoxelAtlasTexture,

	Asset_Animation,
	Asset_Mesh,

	Asset_OldSchoolDima,

	Asset_Count,
};


#endif /*IVAN_ASSET_IDS*/