!include WriteEnvStr.nsh
!include WritePathStr.nsh

!define TT_ENV_INI_FILE "setenvpage.ini"

!define UI_FILE_INTERNAL_DESC "Trolltech.DesignerForm"
!define UI_FILE_OPEN_DESC "Open with Qt Designer"
!define DESIGNER_DESC "Qt Designer"
!define DESIGNER_FILE_DESC "Qt Designer File"
!define DESIGNER_CMD "bin\designer.exe $\"%1$\""
!define DESIGNER_CMD_SHORT "designer.exe"

var SET_ENV_VARS
var OLD_QTDIR
var REGISTER_UI_EXT_STATE
var RUNNING_AS_ADMIN

LangString EnvTitle ${LANG_ENGLISH} "Configure Environment"
LangString EnvTitleDescription ${LANG_ENGLISH} "Configure the environment variables for ${PRODUCT_NAME} ${PRODUCT_VERSION}"

!macro TT_QTENV_PAGE_INIT
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "${TT_ENV_INI_FILE}"
  push $0
  ExpandEnvStrings $0 "%QTDIR%"

  StrCmp $0 "%QTDIR%" +4
  strcpy $SET_ENV_VARS "0" ;QTDIR exists
  strcpy $OLD_QTDIR $0
  Goto +3
  strcpy $SET_ENV_VARS "1" ;no QTDIR
  strcpy $OLD_QTDIR "0"

  pop $0
  strcpy $REGISTER_UI_EXT_STATE "0"
!macroend

!macro TT_QTENV_PAGE_SHOW
  Page custom SetEnvPage SetEnvVariables
!macroend

!macro TT_QTENV_UNREGISTER
  push "$INSTDIR"
  call un.UnRegisterQtEnvVariables
  call un.UnregisterUIExtension
!macroend

!macro TT_QTENV_REGISTER
  push "$INSTDIR"
  call RegisterQtEnvVariables
  call RegisterUIExtension
!macroend

Function SetEnvPage
  !insertmacro MUI_HEADER_TEXT "$(EnvTitle)" "$(EnvTitleDescription)"
  
  strcmp $SET_ENV_VARS "1" 0 envCheckNo
    !insertmacro MUI_INSTALLOPTIONS_WRITE "${TT_ENV_INI_FILE}" "Field 3" "State" "1"
    goto showEnvPage
  envCheckNo:
    !insertmacro MUI_INSTALLOPTIONS_WRITE "${TT_ENV_INI_FILE}" "Field 3" "State" "0"

  showEnvPage:
  
  strcmp $RUNNING_AS_ADMIN "true" handleUiCheck
  !insertmacro MUI_INSTALLOPTIONS_WRITE "${TT_ENV_INI_FILE}" "Field 1" "Flags" "DISABLED"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "${TT_ENV_INI_FILE}" "Field 6" "Flags" "DISABLED"
  goto uiCheckNo
  
  handleUiCheck:
  strcmp $REGISTER_UI_EXT_STATE "1" 0 uiCheckNo
    !insertmacro MUI_INSTALLOPTIONS_WRITE "${TT_ENV_INI_FILE}" "Field 6" "State" "1"
    goto showUiPage
  uiCheckNo:
    !insertmacro MUI_INSTALLOPTIONS_WRITE "${TT_ENV_INI_FILE}" "Field 6" "State" "0"

  showUiPage:
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "${TT_ENV_INI_FILE}"
FunctionEnd

Function SetEnvVariables
  !insertmacro MUI_INSTALLOPTIONS_READ $SET_ENV_VARS "${TT_ENV_INI_FILE}" "Field 3" "State"
  !insertmacro MUI_INSTALLOPTIONS_READ $REGISTER_UI_EXT_STATE "${TT_ENV_INI_FILE}" "Field 6" "State"

  StrCmp $RUNNING_AS_ADMIN "true" +3
  WriteRegDWORD "HKCU" "${PRODUCT_UNINST_KEY}" "QtEnvSet" $SET_ENV_VARS
  Goto +2
  WriteRegDWORD "HKLM" "${PRODUCT_UNINST_KEY}" "QtEnvSet" $SET_ENV_VARS
  
FunctionEnd

Function RegisterUIExtension
  strcmp $REGISTER_UI_EXT_STATE "1" 0 end
    WriteRegStr HKCR "${UI_FILE_INTERNAL_DESC}" "" "${DESIGNER_FILE_DESC}"
    WriteRegStr HKCR "${UI_FILE_INTERNAL_DESC}\shell" "" "open"
    WriteRegStr HKCR "${UI_FILE_INTERNAL_DESC}\shell\open" "" "${UI_FILE_OPEN_DESC}"
    WriteRegStr HKCR "${UI_FILE_INTERNAL_DESC}\shell\open\command" "" "$INSTDIR\${DESIGNER_CMD}"
    WriteRegStr HKCR "${UI_FILE_INTERNAL_DESC}\DefaultIcon" "" "$INSTDIR\bin\${DESIGNER_CMD_SHORT},0"

    ; Overwrite it silently...
    WriteRegStr HKCR ".ui" "" "${UI_FILE_INTERNAL_DESC}"
  end:
FunctionEnd

Function un.UnregisterUIExtension
  push $1
  ReadRegStr $1 HKCR ".ui" ""
  strcmp $1 "${UI_FILE_INTERNAL_DESC}" 0 continue
    ; do not delete this key since a subkey openwithlist
    ; or open withprogid may exist
    WriteRegStr HKCR ".ui" "" ""
  continue:
  ; just delete it since nobody else is supposed to use it
  DeleteRegKey HKCR "${UI_FILE_INTERNAL_DESC}"

  pop $1
FunctionEnd

#
# to set the qt env. varibles
# push "c:\qt"  #QTDIR
# call setQtEnvVariables
#
Function RegisterQtEnvVariables
  exch $2 ; the installation path = QTDIR
  push $0 ; I think WriteEnvStr mixes up $0 and $1
  push $1
  
  intcmp $SET_ENV_VARS 0 noenv
  
  StrCmp $OLD_QTDIR "0" +4
  DetailPrint "Removing $OLD_QTDIR\bin from PATH"
  push "$OLD_QTDIR\bin"
  Call RemoveFromPath ; remove old qtdir

  DetailPrint "Setting QTDIR to $2"
  push "QTDIR"
  push $2
  Call WriteEnvStr ; set the QTDIR

  DetailPrint "Adding $2\bin to PATH"
  push "$2\bin"
  Call AddToPath ; set the PATH

  push "QMAKESPEC"
  Call GetMkSpec
  pop $0
  DetailPrint "Setting QMAKESPEC to $0"
  push $0
  Call WriteEnvStr ; set the QMAKESPEC

  noenv:
    pop $1
    pop $0
    pop $2
FunctionEnd

#
# removes the qt env. varibles
# push "c:\qt"  #QTDIR
# call un.RemoveQtEnvVariables
#
Function un.UnRegisterQtEnvVariables
  exch $0 ; QTDIR
  push $1

  ClearErrors
  ReadRegDWORD $SET_ENV_VARS "HKCU" "${PRODUCT_UNINST_KEY}" "QtEnvSet"
  IfErrors 0 +2
  ReadRegDWORD $SET_ENV_VARS "HKLM" "${PRODUCT_UNINST_KEY}" "QtEnvSet"

  intcmp $SET_ENV_VARS 0 noenv

  DetailPrint "Removing $0\bin from the PATH"
  push "$0\bin"
  Call un.RemoveFromPath ; removes qt from the path

  ;Check if QTDIR is equal to installdir
  ExpandEnvStrings $1 "%QTDIR%"
  
  StrCmp "$0" "$1" removeenv
  StrCmp "$0\" "$1" removeenv
  StrCmp "$0" "$1\" removeenv
  Goto noenv
  
  removeenv:
  DetailPrint "Removing QTDIR"
  push "QTDIR"
  Call un.DeleteEnvStr ; removes QTDIR

  DetailPrint "Removing QMAKESPEC"
  push "QMAKESPEC"
  Call un.DeleteEnvStr ; removes QMAKESPEC

  noenv:
    pop $1
    pop $0
FunctionEnd

#
# returns the makespec
# the result is placed on top of the stack
#
Function GetMkSpec
  push $0
  
  StrCmp ${FORCE_MAKESPEC} "vs2003" win32-msvc.net
  StrCmp ${FORCE_MAKESPEC} "vs2002" win32-msvc.net
  StrCmp ${FORCE_MAKESPEC} "vc60" win32-msvc

  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\8.0" "InstallDir"
  StrCmp $0 "" +1 win32-msvc.net ; found msvc.net 2005

  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\7.1" "InstallDir"
  StrCmp $0 "" +1 win32-msvc.net ; found msvc.net 2003

  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\7.0" "InstallDir"
  StrCmp $0 "" +1 win32-msvc.net ; found msvc.net 2002

  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\6.0\Setup" "VsCommonDir"
  StrCmp $0 "" +1 win32-msvc ; found msvc 6.0

  ReadRegStr $0 HKLM "Software\Intel\Compilers\C++\80" "Major Version"
  StrCmp $0 "" +1 win32-icc ; found icc 8.x

  ; we don't support this compiler, but it doesn't hurt :)
  ReadRegStr $0 HKLM "Software\Intel\Compilers\C++\70" "Major Version"
  StrCmp $0 "" +1 win32-icc ; found icc 7.x

  Goto win32-unknown

  win32-msvc.net:
    pop $0
    push "win32-msvc.net"
    Goto getmkspec_done

  win32-msvc:
    pop $0
    push "win32-msvc"
    Goto getmkspec_done

  win32-icc:
    pop $0
    push "win32-icc"
    Goto getmkspec_done

  ; unknown compiler
  win32-unknown:
    pop $0
    push "win32-msvc.net" ; fall back on .net

  getmkspec_done:
FunctionEnd

#
# creates a qtvars.bat file in $QTDIR\bin
# push "c:\qt"  #QTDIR
# call MakeQtVarsFile
#
Function MakeQtVarsFile
  exch $1 ; QTDIR
  push $0 ; file handle
  push $2 ; mkspec

  push $1
  call GetMkSpec
  pop $2

  ClearErrors
  FileOpen $0 "$1\bin\qtvars.bat" w
  IfErrors done
  FileWrite $0 "@rem$\r$\n"
  FileWrite $0 "@rem This file is generated$\r$\n"
  FileWrite $0 "@rem$\r$\n"
  FileWrite $0 "$\r$\n"
  FileWrite $0 "@echo Setting up a Qt environment...$\r$\n"
  FileWrite $0 "@echo -- QTDIR set to $1$\r$\n"
  FileWrite $0 "@echo -- Added $1\bin to PATH$\r$\n"
  FileWrite $0 "@echo -- QMAKESPEC set to $2$\r$\n"
  FileWrite $0 "$\r$\n"
  FileWrite $0 "@set QTDIR=$1$\r$\n"
  FileWrite $0 "@set PATH=$1\bin;%PATH%$\r$\n"
  FileWrite $0 "@set QMAKESPEC=$2$\r$\n"
  
  FileWrite $0 "$\r$\n"
  FileWrite $0 '@if not "%1"=="setup" goto SETUP_DONE$\r$\n'
  FileWrite $0 "@cd %QTDIR%\examples$\r$\n"
  FileWrite $0 "@qmake -r -tp vc$\r$\n"
  FileWrite $0 "@:SETUP_DONE$\r$\n"

  call GetVSVarsFile
  pop $2
  strcmp $2 "" novsvars
    FileWrite $0 "$\r$\n"
    FileWrite $0 '@if not "%1"=="vsvars" goto END$\r$\n'
    FileWrite $0 '@call "$2"$\r$\n'
    FileWrite $0 "@:END$\r$\n"
  novsvars:
  FileWrite $0 "$\r$\n"
  FileWrite $0 "@cd %QTDIR%$\r$\n"
  FileClose $0
  done:
  pop $2
  pop $0
  pop $1
FunctionEnd

#
# try to get the right vxvarsfile
#
Function GetVSVarsFile
  push $0

  StrCmp ${FORCE_MAKESPEC} "vs2003" VS2003
  StrCmp ${FORCE_MAKESPEC} "vs2002" VS2002
  StrCmp ${FORCE_MAKESPEC} "vc60" VS60

  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\8.0\Setup\VS" "ProductDir"
  StrCmp $0 "" +1 foundVSDir ; found msvc.net 2005

  VS2003:
  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\7.1\Setup\VS" "ProductDir"
  StrCmp $0 "" +1 foundVSDir ; found msvc.net 2003

  VS2002:
  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\7.0\Setup\VS" "ProductDir"
  StrCmp $0 "" +1 foundVSDir ; found msvc.net 2002

  VS60:
  ReadRegStr $0 HKLM "Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir"
  StrCmp $0 "" +1 foundVCDir ; found msvc 6.0

  push "" ;empty string if not found
  goto done

  foundVSDir:
    push "$0\Common7\Tools\vsvars32.bat"
    goto done

  foundVCDir:
    push "$0\bin\vcvars32.bat"

  done:
    exch
    pop $0
FunctionEnd