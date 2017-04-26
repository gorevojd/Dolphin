@echo off

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -wd4201 -Wv:18
set CommonCompilerFlags=%CommonCompilerFlags% -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7

set CommonLinkerFlags=-incremental:no -opt:ref opengl32.lib user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\BatBuild mkdir ..\BatBuild
pushd ..\BatBuild

del *.pdb > NUL 2> NUL

REM Asset builder
cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\Source\asset_builder.cpp /link %CommonLinkerFlags%


cl %CommonCompilerFlags% /LD ..\Source\ivan.cpp -Fmivan.map -DDebugRecordArray=DebugRecords_MainTranslationUnit /link -incremental:no -opt:ref -PDB:ivan_%random%.pdb

cl %CommonCompilerFlags% ..\Source\win32_ivan.cpp -Fmwin32_ivan.map /link %CommonLinkerFlags%

popd
