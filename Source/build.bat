@echo off

set CommonCompilerFlags=-O2 -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -wd4201
set CommonCompilerFlags=%CommonCompilerFlags% -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7

set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\BatBuild mkdir ..\BatBuild
pushd ..\BatBuild

cl %CommonCompilerFlags% ..\Source\win32_dolphin.cpp /link %CommonLinkerFlags%

popd
