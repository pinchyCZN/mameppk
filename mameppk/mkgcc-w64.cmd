@echo off

set BUILDTOOLS_ROOT=..\buildtools

REM SET PROCESSOR_ARCHITECTURE=AMD64
CALL "%BUILDTOOLS_ROOT%\vendor\env.bat"

gcc -v

make -j3 >compile.log
pause
