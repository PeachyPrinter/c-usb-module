@ECHO OFF

ECHO ------------------------------------
ECHO Cleaning workspace
ECHO ------------------------------------

DEL /Q *.dll
RMDIR /S /Q _build

ECHO ------------------------------------
ECHO Extracting Git Revision Number
ECHO ------------------------------------

SET SEMANTIC=0.0.1
SET /p SEMANTIC=<symantic.version
IF NOT DEFINED GIT_HOME (
  git --version
  IF "%ERRORLEVEL%" == "0" (
    SET GIT_HOME=git
  ) ELSE (
    ECHO "Could not find git."
    PAUSE
    EXIT /B 1
  )
)

FOR /f "delims=" %%A in ('%GIT_HOME% rev-list HEAD --count') do SET "GIT_REV_COUNT=%%A"
FOR /f "delims=" %%A in ('%GIT_HOME% rev-parse HEAD') do SET "GIT_REV=%%A"

SET VERSION=%SEMANTIC%.%GIT_REV_COUNT%
ECHO Version: %VERSION%
ECHO # THIS IS A GENERATED FILE  > version.properties
ECHO version='%VERSION%' >> version.properties
ECHO revision='%GIT_REV%' >> version.properties
ECHO Git Revision Number is %GIT_REV_COUNT%


ECHO ------------------------------------
ECHO Creating DLL
ECHO ------------------------------------

mkdir win32
IF NOT "%ERRORLEVEL%" == "0" (
  ECHO FAILED executing command: mkdir win32
  EXIT /B 100
)
mkdir _build
IF NOT "%ERRORLEVEL%" == "0" (
  ECHO FAILED executing command: mkdir _build
  EXIT /B 101
)
cd _build

cmake .. -G "Visual Studio 12 2013 Win64"
IF NOT "%ERRORLEVEL%" == "0" (
  ECHO FAILED executing command: cmake .. -G "Visual Studio 12 2013"
  EXIT /B 102
)

cmake --build . --config Release
IF NOT "%ERRORLEVEL%" == "0" (
  ECHO FAILED executing command: cmake --build . --config Release
  EXIT /B 103
)


ECHO ------------------------------------
ECHO Moving file
ECHO ------------------------------------
copy src\Release\libPeachyUSB.dll ..\win32
IF NOT "%ERRORLEVEL%" == "0" (
    ECHO "FAILED moving files"
    EXIT /B 798
)
cd ..