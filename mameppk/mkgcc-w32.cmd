@echo off

set BUILDTOOLS_ROOT=..\buildtools

SET PROCESSOR_ARCHITECTURE=x86
CALL "%BUILDTOOLS_ROOT%\vendor\env.bat"

gcc -v

make -j3 >compile.log
pause
