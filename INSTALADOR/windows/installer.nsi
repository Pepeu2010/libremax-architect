Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!ifndef APP_VERSION
  !error "Provide APP_VERSION, PAYLOAD and OUTPUT."
!endif
Name "LibreMax Architect ${APP_VERSION} — Prévia"
OutFile "${OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\LibreMax Architect"
RequestExecutionLevel user
SetCompressor /SOLID lzma
Icon "..\..\resources\libremax.ico"
UninstallIcon "..\..\resources\libremax.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Este assistente instala o LibreMax Architect com modelos, exemplos e o mecanismo de imagem Blender/Cycles.$\r$\n$\r$\nTudo fica pronto para criar imagens, sem instalar o Blender separadamente.$\r$\n$\r$\nClique em Avançar para continuar."
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\libremax-architect.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Abrir LibreMax Architect"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "PortugueseBR"

Section "LibreMax Architect"
  SetShellVarContext current
  SetOutPath "$INSTDIR"
  File /r "${PAYLOAD}\*.*"
  WriteUninstaller "$INSTDIR\Desinstalar.exe"
  CreateDirectory "$SMPROGRAMS\LibreMax Architect"
  CreateShortcut "$SMPROGRAMS\LibreMax Architect\LibreMax Architect.lnk" "$INSTDIR\bin\libremax-architect.exe"
  CreateShortcut "$SMPROGRAMS\LibreMax Architect\Desinstalar.lnk" "$INSTDIR\Desinstalar.exe"
  CreateDirectory "$DESKTOP"
  CreateShortcut "$DESKTOP\LibreMax Architect.lnk" "$INSTDIR\bin\libremax-architect.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "DisplayName" "LibreMax Architect"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "DisplayVersion" "${APP_VERSION} — Prévia"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "Publisher" "LibreMax contributors"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "UninstallString" '$\"$INSTDIR\Desinstalar.exe$\"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "DisplayIcon" "$INSTDIR\bin\libremax-architect.exe"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext current
  Delete "$DESKTOP\LibreMax Architect.lnk"
  Delete "$SMPROGRAMS\LibreMax Architect\LibreMax Architect.lnk"
  Delete "$SMPROGRAMS\LibreMax Architect\Desinstalar.lnk"
  RMDir "$SMPROGRAMS\LibreMax Architect"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\LibreMaxArchitect"
  ; Only remove installed application folders. Projects and settings are stored elsewhere.
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\share"
  Delete "$INSTDIR\Desinstalar.exe"
  RMDir "$INSTDIR"
SectionEnd
