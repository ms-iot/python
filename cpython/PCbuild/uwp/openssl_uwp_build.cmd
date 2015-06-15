setlocal
set CONFIGURATION=%2
set PLATFORM=%3

if not exist %1 (
    echo %1 does not exist!
    endlocal
    exit /b 1
)

pushd %1

if not exist vsout\NT-Universal-10.0-Static-Unicode\nt-%CONFIGURATION%-%PLATFORM%.mak (
    call ms\do_vsprojects14.bat
)

perl Configure no-asm no-hw no-dso VC-WINUNIVERSAL 

call ms\do_winuniversal.bat

call ms\setVSVars.bat universal10.0%PLATFORM%

set

if not exist vsout\NT-Universal-10.0-Static-Unicode\%CONFIGURATION%\%PLATFORM%\bin\libeay32.lib (
    call nmake -f vsout\NT-Universal-10.0-Static-Unicode\nt-%CONFIGURATION%-%PLATFORM%.mak init > vsout\NT-Universal-10.0-Static-Unicode\nt-%CONFIGURATION%-%PLATFORM%.build.log
    call nmake -f vsout\NT-Universal-10.0-Static-Unicode\nt-%CONFIGURATION%-%PLATFORM%.mak >> vsout\NT-Universal-10.0-Static-Unicode\nt-%CONFIGURATION%-%PLATFORM%.build.log
)
popd
endlocal
