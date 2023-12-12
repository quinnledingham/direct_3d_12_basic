@echo off

pushd %CD%
IF NOT EXIST build mkdir build
cd build

set CFs= -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -FC -Z7 /I..\sdl-vc\include /I..\glad /I..\stb /DSDL /D_CRT_SECURE_NO_WARNINGS -W3
set CFs2= -nologo -O2 -Oi /Isdl-vc\include /Iglad /Istb /DWINDOWS
set LFs= -incremental:no -opt:ref shell32.lib opengl32.lib ..\sdl-vc\lib\x64\SDL2main.lib ..\sdl-vc\lib\x64\SDL2.lib /subsystem:console

cl %CFs% ../game.cpp ../application.cpp /link %LFs% /out:river.exe

IF NOT EXIST SDL2.dll copy ..\sdl-vc\lib\x64\SDL2.dll build