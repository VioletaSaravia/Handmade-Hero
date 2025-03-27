@echo off
setlocal enabledelayedexpansion

if "%1" == "release" (
    odin build .\src\main_release -out:build/win32/main.exe
    REM mv .\data\ .\build\win32\data
    REM mv .\shaders\ .\build\win32\shaders
) else (
    set PDB_NUMBER=0

    :loop
    set FILE=build/debug/game_!PDB_NUMBER!.pdb
    if exist "!FILE!" (
        set /a PDB_NUMBER+=1
        goto :loop
    )

    odin build .\src\game -build-mode=dll -debug -pdb-name:build/debug/game_!PDB_NUMBER!.pdb -out:build/debug/game.dll

    handle build/debug/main.exe >nul 2>&1
    if %errorlevel% == 0 (
        odin build .\src\main_debug -debug -out:build/debug/main.exe
    )
)
