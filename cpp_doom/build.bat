@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /EHsc /O2 /DUNICODE /D_UNICODE main.cpp /Fe:aws-bomb.exe user32.lib gdi32.lib comctl32.lib shell32.lib
