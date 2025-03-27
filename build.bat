@echo off
setlocal enabledelayedexpansion

if "%1"=="release" (
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

    echo First available filename: !FILE!

    odin build .\src\game -build-mode=dll -debug -pdb-name:build/debug/game_!PDB_NUMBER!.pdb -out:build/debug/game.dll

    if !PDB_NUMBER! == 0 (
        odin build .\src\main_debug -debug -out:build/debug/main.exe
    )
)

