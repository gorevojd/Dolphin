#include <iostream>
#include <Windows.h>

typedef unsigned int u32;
typedef unsigned char u8;

enum format_collation_type {
	FormatCollation_Header,
	FormatCollation_Source,
	FormatCollation_Batch,
};

struct format_collation_info {
	u32 LineCount;
	u32 FileCount;

	u32 FormatCollationType;
};

struct file_collation_info {
	u32 LineCount;
};

struct open_file_result {
	u32 FileSize;
	u8* Data;
};

static open_file_result OpenFileForCollation(char* FileName) {
	open_file_result Result = {};
	
	FILE* fp = fopen(FileName, "rb");

	if (fp) {

		u32 FileSize = 0;

		fseek(fp, 0, SEEK_END);
		FileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		Result.FileSize = FileSize;
		Result.Data = (u8*)malloc(FileSize);

		fread(Result.Data, 1, FileSize, fp);

		fclose(fp);
	}

	return(Result);
}

static file_collation_info CollateFileInfo(char* FileName) {
	file_collation_info Result = {};

	open_file_result OpenRes = OpenFileForCollation(FileName);
	
	for (u32 ByteIndex = 0; ByteIndex < OpenRes.FileSize; ByteIndex++) {
		u8 CheckByte = OpenRes.Data[ByteIndex];

		if ((CheckByte == '\n') ||
			(CheckByte == '\r') ||
			(CheckByte == 0))
		{
			Result.LineCount++;
		}
	}

	return(Result);
}

static void MergeFileCollationToFormatCollation(format_collation_info* FormatCollation, file_collation_info* FileCollation) {
	FormatCollation->FileCount++;
	FormatCollation->LineCount += FileCollation->LineCount;
}

static format_collation_info CollateInfoForFileFormatInDirectory(char* WildCard, u32 Type) {
	format_collation_info Result = {};

	Result.FormatCollationType = Type;

	WIN32_FIND_DATAA FindData;
	HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE) {
		file_collation_info FileCollation = CollateFileInfo(FindData.cFileName);
		MergeFileCollationToFormatCollation(&Result, &FileCollation);

		while (FindNextFileA(FindHandle, &FindData)) {
			file_collation_info NextFileCollation = CollateFileInfo(FindData.cFileName);
			MergeFileCollationToFormatCollation(&Result, &NextFileCollation);
		}
	}

	FindClose(FindHandle);

	return(Result);
}

static void CollateProjectSourceDirectoryInfo(char* DirPath) {
	SetCurrentDirectoryA(DirPath);

	char TempBufferSize[256];
	GetCurrentDirectoryA(256, TempBufferSize);

	format_collation_info Collations[] = {
		CollateInfoForFileFormatInDirectory("*.cpp", FormatCollation_Source),
		CollateInfoForFileFormatInDirectory("*.h", FormatCollation_Header),
		CollateInfoForFileFormatInDirectory("*.bat", FormatCollation_Batch),
	};

	u32 CollationsCount = sizeof(Collations) / sizeof(Collations[0]);

	u32 TotalLineCount = 0;
	u32 HeaderFilesCount = 0;
	u32 SourceFilesCount = 0;
	u32 BatchFilesCount = 0;

	for (int CollationIndex = 0;
		CollationIndex < CollationsCount;
		CollationIndex++)
	{
		format_collation_info* Collation = &Collations[CollationIndex];

		TotalLineCount += Collation->LineCount;

		switch (Collation->FormatCollationType) {
			case(FormatCollation_Batch): {
				BatchFilesCount = Collation->FileCount;
			}break;

			case(FormatCollation_Header): {
				HeaderFilesCount = Collation->FileCount;
			}break;

			case(FormatCollation_Source): {
				SourceFilesCount = Collation->FileCount;
			}break;

			default: {

			}break;
		}
	}

	printf("Total lines count: %u\n", TotalLineCount);
	printf("Header files count: %u\n", HeaderFilesCount);
	printf("Source files count: %u\n", SourceFilesCount);
	printf("Batch files count: %u\n", BatchFilesCount);
}

int main(int ArgCount, char** Args) {

	CollateProjectSourceDirectoryInfo("E:/Programming/MyProjects/IvanEngine/Source");
	
	system("pause");
	return(0);
}