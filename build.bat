@echo off
setlocal

set game=%1
set build=%2
set platform=%3

set TIMESTAMP=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%

if /i "%build%"=="clean" (
    del build\\debug\\game*.pdb >nul 2>&1
    del build\\debug\\game*.dll >nul 2>&1
    cl.exe /DDEBUG /MP /W1 /Zi /EHsc /nologo /Fobuild\\debug\\ /Fdbuild\\debug\\ /Febuild\\debug\\game.dll /Ivendor\\ src\\games\\game.c vendor\\glad.c vendor\\miniaudio.c vendor\\stb_image.c /link /INCREMENTAL:NO User32.lib Gdi32.lib Winmm.lib opengl32.lib xinput.lib /DLL
) else if /i "%build%"=="release" (
    cl.exe /DNDEBUG /MP /O2 /EHsc /nologo /Fobuild\\win32\\ /Ivendor\\ src\\main.c vendor\\glad.c vendor\\miniaudio.c vendor\\stb_image.c /link /NOEXP /NOIMPLIB User32.lib Gdi32.lib Winmm.lib opengl32.lib xinput.lib /OUT:build\\win32\\main.exe"
) else (
    cl.exe /DDEBUG /MP /W1 /Zi /EHsc /nologo /Fobuild\\debug\\ /Fdbuild\\debug\\ /Febuild\\debug\\game.dll /Ivendor\\ src\\games\\game.c /link /INCREMENTAL:NO build\\debug\\miniaudio.obj build\\debug\\glad.obj build\\debug\\stb_image.obj User32.lib Gdi32.lib Winmm.lib opengl32.lib xinput.lib /PDB:build\\debug\\game%TIMESTAMP%.pdb /DLL
)