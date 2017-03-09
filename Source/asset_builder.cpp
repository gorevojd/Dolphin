#include "asset_builder.h"

FILE* fp = 0;

INTERNAL_FUNCTION void 
BeginAssetType(game_assets* Assets, asset_type_id TypeID){
	Assert(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
	Assets->DEBUGAssetType->TypeID = TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}


#if 0

struct added_asset{
	unsigned int ID;
	dfa_asset* DFA;
	asset_source* Source;
};

INTERNAL_FUCNTION added_asset
AddAsset(game_assets* Assets){
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

	unsigned int Value = Assets->DEBUGAssetType->OnePastLastTagIndex++;
	asset_source* Source = Assets->AssetSources + Value;
	dfa_asset* DFA = Assets->Assets + Value;
	DFA->FirstTagIndex = Assets->TagCount;
	DFA->OnePastLastTagIndex = DFA->FirstTagIndex;

	Assets->AssetIndex = Value;
	
	added_asset Result;
	Result.DFA = DFA;
	Result.Source = Source;
	Result.ID = Value;

	return(Result);
}

INTERNAL_FUNCTION bitmap_id
AddBitmapAsset(
	game_assets* Assets,
	char* FileName,
	float AlignPercentageX = 0.0f,
	float AlignPercentageY = 0.0f)
{
	added_asseet Asset = AddAsset(Assets);
	Asset.DFA->Bitmap.AlignPercentage[0] = AlignPercentageX;
	Asset.DFA->Bitmap.AlignPercentage[1] = AlignPercentageY;
	Asset.Source->Type = AssetType_Bitmap;
	Asset.Source->Bitmap.FileName = FileName;

	bitmap_id Result = {Asset.ID};
	return(Result);
}

INTERNAL_FUNCTION sound_id
AddSoundAsset(
	game_assets* Assets, 
	char* FileName,
	unsigned int FistSampleIndex = 0,
	unsigned int SampleCount = 0)
{
	added_asset Asset = AddAsset(Assets);
	Asset.DFA->Sound.SampleCount = SampleCount;
	Asset.DFA->Sound.Chain = DFASoundChain_None;
	Asset.Source->Type = AssetType_Sound;
	Asset.Source->Sound.FileName = FileName;
	Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;

	sound_id Result = {Asset.ID};
	return(Result);
}

#endif


INTERNAL_FUNCTION bitmap_id
AddBitmapAsset(
	game_assets* Assets,
	char* FileName,
	float AlignPercentageX = 0.0f,
	float AlignPercentageY = 0.0f)
{
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

	bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
	asset* Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->TagCount;
	Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
	Asset->Bitmap.FileName = FileName;
	Asset->Bitmap.AlignPercentage[0] = AlignPercentageX;
	Asset->Bitmap.AlignPercentage[1] = AlignPercentageY;

	Assets->DEBUGAsset = Asset;

	return(Result);
}

INTERNAL_FUNCTION sound_id
AddSoundAsset(
	game_assets* Assets,
	char* FileName,
	unsigned int FirstSampleIndex = 0,
	unsigned int SampleCount = 0)
{
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

	sound_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
	asset* Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->TagCount;
	Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
	Asset->Sound.FileName = FileName;
	Asset->Sound.FirstSampleIndex = FirstSampleIndex;
	Asset->Sound.SampleCount = SampleCount;
	Asset->Sound.NextIDToPlay.Value = 0;

	Assets->DEBUGAsset = Asset;

	return(Result);
}

INTERNAL_FUNCTION void
AddTag(game_assets* Assets, asset_tag_id ID, float Value){
	Assert(Assetes->DEBUGAsset);

	Assets->DEBUGAsset->OnePastLastTagIndex++;
	dfa_tag* tag = Assets->Tags + Assets->TagCount++;

	Tag->ID = ID;
	Tag->Value = Value;
}

INTERNAL_FUNCTION void
EndAssetType(game_assets* Assets){
	Assert(Assets->DEBUGAssetType);
	Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
	Assets->DEBUGAssetType = 0;
	Assets->DEBUGAsset = 0;
}

int main(int ArgCount, char** Args){

	fp = fopen("test.dfa", "wb");
	if(fp){

		dfa_header Header = {};
		Header.MagickValue = DFA_MAGICK_VALUE;
		Header.Version = DFA_VERSION;
		Header.TagCount = Assets->TagCount;
		Header.AssetTypeCount = ??;
		Header.AssetCount = Assets->AssetCount;

		uint32 TagArraySize = Header.TagCount * sizeof(dfa_tag);
		uint32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(dfa_asset_type);
		uint32 AssetArraySize = Header.AssetCount * sizeof(dfa_asset);

		Header.TagOffset = sizeof(Header);
		Header.AssetTypeOffset = Header.TagOffset + TagArraySize;
		Header.AssetOffset = Header.AssetTypeOffset + AssetTypeArraySize;

		fwrite(Header, sizeof(Header), 1, fp);


		fclose(fp);
	}
	else{
		printf("Error while opening the file T_T\n");
	}


	return(0);
}
