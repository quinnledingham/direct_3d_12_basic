@echo off

pushd %CD%
IF NOT EXIST build mkdir build
cd build

set CFs= -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -FC -Z7 /D_CRT_SECURE_NO_WARNINGS -W3 /DWINDOWS /DDEBUG /EHsc
set LFs= -incremental:no -opt:ref shell32.lib user32.lib gdi32.lib D3d12.lib D3DCompiler.lib dxgi.lib /subsystem:windows

cl %CFs% ../win32_application.cpp /link %LFs% /out:d.exe
