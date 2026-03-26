@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
rc.exe /nologo /fo resources.res src\resources.rc
cl.exe /nologo /utf-8 /EHsc /O2 /DUNICODE /D_UNICODE src\main.cpp resources.res /Fe:AWSCleaner.exe user32.lib gdi32.lib comctl32.lib shell32.lib gdiplus.lib ole32.lib
