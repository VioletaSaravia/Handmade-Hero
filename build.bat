@echo off
setlocal

set game=%1
set build=%2

@REM set TIMESTAMP=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set HOUR=%time:~0,2%
if "%HOUR:~0,1%"==" " set HOUR=0%HOUR:~1,1%
set TIMESTAMP=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%HOUR%%time:~3,2%%time:~6,2%

set CL_DEBUG=cl.exe /DDEBUG /DLOG_LEVEL=0 /MP /W1 /Zi /EHsc /nologo /Fobuild\\debug\\ /Fdbuild\\debug\\ /Febuild\\debug\\game.dll /Ivendor\\ src\\games\\game.c
@REM Not necessary: opengl32.lib
set WIN_LIBS=  SDL3.lib SDL3_image.lib SDL3_ttf.lib 
set VENDOR_UNITS=vendor\\glad.c

set INCLUDES=/Iinclude\\
set LIBS=/LIBPATH:lib\\x64\\

if /i "%build%"=="clean" (
    del build\\debug\\game*.pdb >nul 2>&1
    del build\\debug\\game*.dll >nul 2>&1
    %CL_DEBUG% %VENDOR_UNITS% %INCLUDES% /link %LIBS% /INCREMENTAL:NO %WIN_LIBS% /DLL
) else if /i "%build%"=="release" (
    cl.exe /DNDEBUG /DLOG_LEVEL=2 /MP /O2 /EHsc /nologo /Fobuild\\win32\\ /Ivendor\\ src\\main.c %VENDOR_UNITS% /link /NOEXP /NOIMPLIB %WIN_LIBS% /OUT:build\\win32\\main.exe
) else (
    %CL_DEBUG% %INCLUDES% /link %LIBS% /INCREMENTAL:NO build\\debug\\glad.obj %WIN_LIBS% /PDB:build\\debug\\game%TIMESTAMP%.pdb /DLL /NOEXP
)