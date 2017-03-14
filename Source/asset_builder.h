#ifndef ASSET_BUILDER_H

#include <stdio.h>
#include <stdlib.h>

#include "dolphin_platform.h"
#include "dolphin_math.h"
#include "dolphin_render_math.h"
#include "dolphin_intrinsics.h"
#include "dolphin_file_formats.h"


enum asset_type{
	AssetType_Sound,
	AssetType_Bitmap,
	AssetType_Model,
};

struct asset_source{
	asset_type Type;
	char* FileName;
	unsigned int FirstSampleIndex;
};

#define DOLPHIN_NUMBER_OF_ASSETS_TO_ALLOC 4096

struct game_assets{
	unsigned int TagCount;
	dda_tag Tags[DOLPHIN_NUMBER_OF_ASSETS_TO_ALLOC];

	unsigned int AssetTypeCount;
	dda_asset_type AssetTypes[Asset_Count];

	unsigned int AssetCount;
	asset_source AssetSources[DOLPHIN_NUMBER_OF_ASSETS_TO_ALLOC];
	dda_asset Assets[DOLPHIN_NUMBER_OF_ASSETS_TO_ALLOC];

	dda_asset_type* DEBUGAssetType;
	unsigned int AssetIndex;
};

#define ASSET_BUILDER_H
#endif