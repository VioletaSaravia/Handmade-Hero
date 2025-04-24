@echo off
setlocal

set game=%1
set build=%2

set CL_DEBUG=cl.exe /DDEBUG /DLOG_LEVEL=0 /MP /W1 /Zi /EHsc /nologo /Fobuild\\debug\\ /Fdbuild\\debug\\ /Febuild\\debug\\game.dll /Ivendor\\ src\\games\\game.c
set WIN_LIBS=User32.lib Gdi32.lib Winmm.lib opengl32.lib xinput.lib
set VENDOR_UNITS=vendor\\glad.c vendor\\miniaudio.c vendor\\stb_image.c

if /i "%build%"=="clean" (
    del build\\debug\\game*.pdb >nul 2>&1
    del build\\debug\\game*.dll >nul 2>&1
    %CL_DEBUG% %VENDOR_UNITS% /link /INCREMENTAL:NO %WIN_LIBS% /DLL
) else if /i "%build%"=="release" (
    cl.exe /DNDEBUG /DLOG_LEVEL=2 /MP /O2 /EHsc /nologo /Fobuild\\win32\\ /Ivendor\\ src\\main.c %VENDOR_UNITS% /link /NOEXP /NOIMPLIB %WIN_LIBS% /OUT:build\\win32\\main.exe"
) else (
    set TIMESTAMP=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
    %CL_DEBUG% /link /INCREMENTAL:NO build\\debug\\miniaudio.obj build\\debug\\glad.obj build\\debug\\stb_image.obj %WIN_LIBS% /PDB:build\\debug\\game%TIMESTAMP%.pdb /DLL
)