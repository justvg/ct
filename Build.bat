@echo off

REM To compile debug version use -Od and -DENGINE_DEBUG
REM To compile profile version use -O2 and -DENGINE_PROFILE
REM To compile release version use -O2 and -DENGINE_RELEASE
REM F4194304 is set to increase stack size to 4MB, needed for builds with editor. For release builds can be disabled.
REM std:c++17 is only here for static_assert

call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

set CommonCompilerFlags=-F4194304 -MT -nologo -Gm- -GR- -EHa- -fp:fast -O2 -Oi -WX -W4 -wd4324 -wd4505 -wd4456 -wd4457 -wd4063 -wd4702 -wd4201 -wd4100 -wd4189 -wd4459 -wd4127 -wd4311 -wd4302 -wd4366 -FC -Z7 
set CommonCompilerFlags=-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN %CommonCompilerFlags%
set CommonCompilerFlags=-DENGINE_PROFILE %CommonCompilerFlags%
set CommonCompilerFlags=-std:c++17 %CommonCompilerFlags% 
set CommonCompilerFlags=-I%VULKAN_SDK%\Include\vulkan %CommonCompilerFlags%
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib Winmm.lib %VULKAN_SDK%\Lib\vulkan-1.lib -ignore:4099

IF NOT EXIST Binaries mkdir Binaries
pushd Binaries

cl %CommonCompilerFlags% "..\Code\WinMain.cpp" -FmWinMain.map /link -PDB:WinMain.pdb %CommonLinkerFlags% 

popd