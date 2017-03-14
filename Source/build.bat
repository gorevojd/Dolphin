@echo off

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -wd4201
set CommonCompilerFlags=%CommonCompilerFlags% -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7

set CommonLinkerFlags=-incremental:no -opt:ref opengl32.lib user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\BatBuild mkdir ..\BatBuild
pushd ..\BatBuild

del *.pdb > NUL 2> NUL

cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\Source\asset_builder.cpp /link %CommonLinkerFlags%

cl %CommonCompilerFlags% /LD ..\Source\dolphin.cpp -Fmdolphin.map /link -incremental:no -opt:ref -PDB:dolphin_%random%.pdb
cl %CommonCompilerFlags% ..\Source\win32_dolphin.cpp -Fmwin32_dolphin.map /link %CommonLinkerFlags%

popd
