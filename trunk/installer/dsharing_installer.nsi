;NSIS Modern User Interface
;Start Menu Folder Selection Example Script
;Written by Joost Verburg
;Modified by Davide Pasca

;--------------------------------
SetCompressor /SOLID lzma 

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;Configuration
;!define MUI_TEXT_FINISH_SHOWREADME "Run ${MY_APP_NAME}"

;--------------------------------
;General
	;Name and file
	Name "DSharingu"

	!define MY_APP_NAME	"DSharingu"

	;${DSHARINGU_VNAME}
	OutFile "${MY_APP_NAME}Setup.exe"
	
	;Default installation folder
	InstallDir "$PROGRAMFILES\${MY_APP_NAME}"

	;Get installation folder from registry if available
	InstallDirRegKey HKCU "Software\${MY_APP_NAME}" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
; Kill all instances of DSharingu
Function .onInit
    FindWindow $0 "DSHARINGU_CLASS"
    IntCmp $0 0 not_found
    
	MessageBox MB_OKCANCEL "All instances of ${MY_APP_NAME} will now be closed." IDOK kill_loop IDCANCEL do_quit

kill_loop:
		FindWindow $0 "DSHARINGU_CLASS"
		IntCmp $0 0 done_killing
		IsWindow $0 0 done_killing
		System::Call 'user32::PostMessageA(i,i,i,i) i($0,${WM_CLOSE},0,0)'
		Sleep 100
		Goto kill_loop

do_quit:
		Quit

done_killing:
not_found:
;	MessageBox MB_OK "yoooooooooo !!!!"
	
;done:
FunctionEnd
  
;--------------------------------
;Pages		
	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "..\manual\license.txt"
	
		!define MUI_COMPONENTSPAGE_NODESC
	!insertmacro MUI_PAGE_COMPONENTS
	
	!insertmacro MUI_PAGE_DIRECTORY
	
		!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
		!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${MY_APP_NAME}" 
		!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
	!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER	

	!insertmacro MUI_PAGE_INSTFILES

	
		# These indented statements modify settings for MUI_PAGE_FINISH
		#!define MUI_FINISHPAGE_NOAUTOCLOSE
		!define MUI_FINISHPAGE_RUN		"$INSTDIR\${MY_APP_NAME}.exe"
		!define MUI_FINISHPAGE_RUN_CHECKED
		;!define MUI_FINISHPAGE_RUN_TEXT "Start a shortcut"
		;!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
		!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
		!define MUI_FINISHPAGE_SHOWREADME $INSTDIR\manual\index.html
	!insertmacro MUI_PAGE_FINISH
	
;	!define MUI_FINISHPAGE_RUN 			"$INSTDIR"
;	!insertmacro MUI_PAGE_FINISH
	
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Russian"

;--------------------------------
;Installer Sections
Section "Core Files (required)" SecCoreFiles
SectionIn RO
	SetOutPath "$INSTDIR"
	File "..\release\${MY_APP_NAME}.exe"
	File /r /x *.svn "..\manual"
	
	;Store installation folder
	WriteRegStr HKCU "Software\${MY_APP_NAME}" "" $INSTDIR
	
	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	
	;Create shortcuts
	CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${MY_APP_NAME}.lnk" "$INSTDIR\${MY_APP_NAME}.exe"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\User Manual.lnk" "$INSTDIR\manual\index.html"
	
	!insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

; Optional section (can be disabled by the user)
Section "Desktop Shortcut" SecDesktop
	IfSilent +2
		CreateShortCut "$DESKTOP\${MY_APP_NAME}.lnk" "$INSTDIR\${MY_APP_NAME}.exe" ""
SectionEnd

; Optional section (can be disabled by the user)
Section "Quick Launch Shortcut" SecQuick
	IfSilent +2
		CreateShortCut "$QUICKLAUNCH\${MY_APP_NAME}.lnk" "$INSTDIR\${MY_APP_NAME}.exe" ""
SectionEnd

; Optional section (can be disabled by the user)
Section "Run After Login" SecRunLogin
	IfSilent +2
		WriteRegStr HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "DSharingu" "$\"$INSTDIR\${MY_APP_NAME}.exe$\" /minimized"
SectionEnd

Section "-run after silent install"
	IfSilent 0 +2
		Exec "$INSTDIR\${MY_APP_NAME}.exe"
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecCoreFiles ${LANG_ENGLISH} "Required files to use ${MY_APP_NAME}."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCoreFiles} $(DESC_SecCoreFiles)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section "Uninstall"
	
	Delete "$DESKTOP\${MY_APP_NAME}.lnk"
	Delete "$QUICKLAUNCH\${MY_APP_NAME}.lnk"

  	RMDir /r	"$INSTDIR"
	
	!insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
	
	Delete "$SMPROGRAMS\$MUI_TEMP\${MY_APP_NAME}.lnk"
	Delete "$SMPROGRAMS\$MUI_TEMP\${MY_APP_NAME}.cfg"
	Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
	Delete "$SMPROGRAMS\$MUI_TEMP\User Manual.lnk"

	Delete "$SMPROGRAMS\$MUI_TEMP\${MY_APP_NAME}.lnk"
	Delete "$SMPROGRAMS\$MUI_TEMP\${MY_APP_NAME}.lnk"

	;Delete empty start menu parent diretories
	StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
	
	startMenuDeleteLoop:
	ClearErrors
	RMDir $MUI_TEMP
	GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
	
	IfErrors startMenuDeleteLoopDone
	
	StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
	startMenuDeleteLoopDone:
	
	DeleteRegKey /ifempty HKCU "Software\${MY_APP_NAME}"
    DeleteRegValue HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "DSharingu"

SectionEnd