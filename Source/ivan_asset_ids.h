#ifndef IVAN_ASSET_IDS
#define IVAN_ASSET_IDS

enum asset_font_type{
	FontType_Default = 0,
	FontType_Debug = 10,
	FontType_Forsazh = 20,
};

enum asset_voxel_atlas_type{
	VoxelAtlasType_Default = 0,
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

	Asset_Count,
};


#endif /*IVAN_ASSET_IDS*/