call :%1 %2
goto END

:begin
  call :reset
  echo !define PRODUCT_NAME "%~1" >> %IWMAKE_NSISCONF%
  echo !define INSTALL_ROOT "%IWMAKE_ROOT%" >> %IWMAKE_NSISCONF%
goto :eof

:output
  set IWMAKE_OUTPUT_FILE=%~1
  echo !define OUTPUT_FILE "%~1" >> %IWMAKE_NSISCONF%
goto :eof

:module
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %~1
  echo !define MODULE_%IWMAKE_RESULT% >> %IWMAKE_NSISCONF%
goto :eof

:enable
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %~1
  echo !define USE_%IWMAKE_RESULT:"=% >> %IWMAKE_NSISCONF%
goto :eof

:disable
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %~1
  echo !undef USE_%IWMAKE_RESULT:"=% >> %IWMAKE_NSISCONF%
goto :eof

:startmenu
  echo !define DEFAULT_STARTMENU_STRING "%~1" >> %IWMAKE_NSISCONF%
goto :eof

:reset
  if exist "%IWMAKE_NSISCONF%" del /Q /F "%IWMAKE_NSISCONF%"
goto :eof

:instdir
  for /F "tokens=1,2*" %%m in ("%~1") do set IWMAKE_TMP=%%~m& set IWMAKE_TMP2=%%~n& set IWMAKE_TMP3="%%~o"
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP%
  echo !define INSTDIR_%IWMAKE_TMP2% %IWMAKE_RESULT:"=% >> "%IWMAKE_NSISCONF%"
  echo !define INSTDIR_%IWMAKE_TMP2%_TEXT %IWMAKE_TMP3% >> "%IWMAKE_NSISCONF%"
goto :eof

:version
  echo !define PRODUCT_VERSION "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:readme
  echo !define README_FILE "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:licenseFile
  echo !define LICENSE_FILE "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:runfunction
  echo !define RUN_FUNCTION "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:readmefunction
  echo !define README_FUNCTION "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:welcomenote
  echo !define WELCOME_NOTE "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:updateplugins
  call "%IWMAKE_SCRIPTDIR%\batch\copy.bat" extsync INetLoad
  xcopy /Q /Y /R "%IWMAKE_ROOT%\INetLoad\INetLoad.dll" "%IWMAKE_NSISPATH%\Plugins\" >> %IWMAKE_LOGFILE%
  xcopy /Q /Y /R "%IWMAKE_SCRIPTDIR%\nsis\qtnsisext\qtnsisext.dll" "%IWMAKE_NSISPATH%\Plugins\" >> %IWMAKE_LOGFILE%
goto :eof

:defineDir
  for /F "tokens=1,2*" %%m in ("%~1") do set IWMAKE_TMP=%%~m& set IWMAKE_TMP2=%%~n& set IWMAKE_TMP3=%%~o
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP%
  set IWMAKE_TMP=%IWMAKE_RESULT%
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP2%
  set IWMAKE_TMP2=%IWMAKE_RESULT%
  echo !define MODULE_%IWMAKE_TMP%_%IWMAKE_TMP2% "%IWMAKE_ROOT%\%IWMAKE_TMP3%" >> "%IWMAKE_NSISCONF%"
goto :eof

:define
  for /F "tokens=1,2*" %%m in ("%~1") do set IWMAKE_TMP=%%~m& set IWMAKE_TMP2=%%~n& set IWMAKE_TMP3="%%~o"
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP%
  set IWMAKE_TMP=%IWMAKE_RESULT%
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP2%
  set IWMAKE_TMP2=%IWMAKE_RESULT%
  if %IWMAKE_TMP3%=="" set IWMAKE_TMP3=
  echo !define MODULE_%IWMAKE_TMP%_%IWMAKE_TMP2% %IWMAKE_TMP3% >> "%IWMAKE_NSISCONF%"
goto :eof

:src
  for /F "tokens=1*" %%m in ("%~1") do set IWMAKE_TMP=%%~m& set IWMAKE_TMP2=%%~n
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP%
  set IWMAKE_TMP=%IWMAKE_RESULT%
  set IWMAKE_TMP3="%IWMAKE_ROOT%\%IWMAKE_TMP2%"
  echo !define MODULE_%IWMAKE_TMP%_ROOT %IWMAKE_TMP3% >> "%IWMAKE_NSISCONF%"
goto :eof

:buildDir
  for /F "tokens=1*" %%m in ("%~1") do set IWMAKE_TMP=%%~m& set IWMAKE_TMP2=%%~n
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP%
  set IWMAKE_TMP=%IWMAKE_RESULT%
  set IWMAKE_TMP3="%IWMAKE_ROOT%\%IWMAKE_TMP2%"
  echo !define MODULE_%IWMAKE_TMP%_BUILDDIR %IWMAKE_TMP3% >> "%IWMAKE_NSISCONF%"
  for /F "eol=- tokens=1,2,3" %%m in ('find "QT_PACKAGEDATE_STR" %IWMAKE_ROOT%\%IWMAKE_TMP2%\src\corelib\global\qglobal.h') do echo !define MODULE_LICENSECHECK_PACKAGEDATE %%o >> "%IWMAKE_NSISCONF%"
goto :eof

:compile
  call :required
  pushd %IWMAKE_SCRIPTDIR%\nsis
  "%IWMAKE_NSISPATH%\makensis.exe" installer.nsi >> %IWMAKE_LOGFILE%
  popd
goto :eof

:required
  call :setrequired PRODUCT_NAME
  call :setrequired INSTALL_ROOT
  call :setrequired PRODUCT_VERSION
  call :setrequired OUTPUT_FILE
  call :setrequired INSTDIR_0
  call :setrequired DEFAULT_STARTMENU_STRING
goto :eof

:setrequired
  echo !ifndef %1 >> "%IWMAKE_NSISCONF%"
  echo   !error "%1 must be in the .conf file..." >> "%IWMAKE_NSISCONF%"
  echo !endif >> "%IWMAKE_NSISCONF%"
goto :eof

:makeFileList
  for /F "tokens=1*" %%m in ("%~1") do set IWMAKE_TMP=%%~m& set IWMAKE_TMP2=%%~n
  call "%IWMAKE_SCRIPTDIR%\batch\toupper.bat" %IWMAKE_TMP%
  set IWMAKE_TMP=%IWMAKE_RESULT%
  set IWMAKE_TMP3="%IWMAKE_ROOT%\%IWMAKE_TMP2%"


  echo !macro MODULE_%IWMAKE_TMP%_INSTALLFILES >> "%IWMAKE_NSISCONF%"

  pushd %IWMAKE_TMP3%

  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$0=$$0%%> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$1=$$1%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$2=$$2%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$3=$$3%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$4=$$4%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$5=$$5%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$6=$$6%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$7=$$7%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$8=$$8%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:$9=$$9%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo set IWMAKE_TMP2=%%IWMAKE_TMP2:%CD%\=%%>> "%IWMAKE_ROOT%\iwmake_tmp.bat"

  dir /AD /B /S | sort /R > "%IWMAKE_ROOT%\iwmake_tmp.txt"
  for /F "tokens=1" %%m in (%IWMAKE_ROOT%\iwmake_tmp.txt) do call :addInstallDirectory "%%m"


  dir /A-D /B /S > "%IWMAKE_ROOT%\iwmake_tmp.txt"
  for /F "tokens=1" %%m in (%IWMAKE_ROOT%\iwmake_tmp.txt) do call :addInstallFile "%%m"

  popd
  echo !macroend >> "%IWMAKE_NSISCONF%"


  echo !macro MODULE_%IWMAKE_TMP%_REMOVE removepath >> "%IWMAKE_NSISCONF%"
  echo     strcmp ${removepath} "" MODULE_%IWMAKE_TMP%_REMOVE_SAFETYLABEL >> "%IWMAKE_NSISCONF%"
  pushd %IWMAKE_TMP3%

  dir /A-D /B /S > "%IWMAKE_ROOT%\iwmake_tmp.txt"
  for /F "tokens=1" %%m in (%IWMAKE_ROOT%\iwmake_tmp.txt) do call :addRemoveFiles "%%m"

  dir /AD /B /S | sort /R > "%IWMAKE_ROOT%\iwmake_tmp.txt"
  for /F "tokens=1" %%m in (%IWMAKE_ROOT%\iwmake_tmp.txt) do call :addRemoveDirectory "%%m"

  popd
  echo     MODULE_%IWMAKE_TMP%_REMOVE_SAFETYLABEL: >> "%IWMAKE_NSISCONF%"
  echo !macroend >> "%IWMAKE_NSISCONF%"
goto :eof

:addInstallFile
  set IWMAKE_TMP2=%~1
  call "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo     File "/oname=%IWMAKE_TMP2%" "%~1" >> "%IWMAKE_NSISCONF%"
goto :eof

:addInstallDirectory
  set IWMAKE_TMP2=%~1
  call "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo     CreateDirectory "$OUTDIR\%IWMAKE_TMP2%" >> "%IWMAKE_NSISCONF%"
goto :eof

:addRemoveDirectory
  set IWMAKE_TMP2=%~1
  call "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo     RMDir ${removepath}\%IWMAKE_TMP2% >> "%IWMAKE_NSISCONF%" 
goto :eof

:addRemoveFiles
  set IWMAKE_TMP2=%~1
  call "%IWMAKE_ROOT%\iwmake_tmp.bat"
  echo     Delete ${removepath}\%IWMAKE_TMP2% >> "%IWMAKE_NSISCONF%"
goto :eof

:sign
  echo Signing Installer ...
  %IWMAKE_SIGNPATH%\signtool.exe sign /v /t http://timestamp.verisign.com/scripts/timestamp.dll /f "%IWMAKE_SIGNPATH%\keys.pfx" "%IWMAKE_OUTPUT_FILE%" >> %IWMAKE_LOGFILE%
goto :eof

:END
