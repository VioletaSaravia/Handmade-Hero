@echo off

REM Only necessary if not in VS prompt
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

powershell -Command "Start-Process -FilePath 'code' -ArgumentList '.' -WindowStyle Hidden"
exit