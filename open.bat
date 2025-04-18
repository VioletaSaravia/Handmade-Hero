@echo off

REM Only necessary if not in VS prompt
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

powershell -Command "Start-Process -FilePath 'code' -ArgumentList '.' -WindowStyle Hidden"
exit