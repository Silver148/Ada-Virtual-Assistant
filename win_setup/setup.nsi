!include "MUI2.nsh"
!include "x64.nsh"

!define PRODUCT_NAME "Ada Virtual Assistant"
!define PRODUCT_PUBLISHER "Juan Yaguaro(aka silverhacker)"
!define PRODUCT_EXE "Ada.exe"
!define SOURCE_DIR "../Ada_packed"

Name "${PRODUCT_NAME}"
OutFile "../Ada-Virtual-Assistant-Setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_EXE}" ""
RequestExecutionLevel admin
Unicode True

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "../LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Spanish"

LangString MSG_REMOVE_MODELS ${LANG_ENGLISH} "Do you want to delete the local AI model (.gguf) and all user configurations from your Documents folder to free up space?"
LangString MSG_REMOVE_MODELS ${LANG_SPANISH} "¿Deseas eliminar el modelo local de IA (.gguf) y todas las configuraciones de usuario de tu carpeta Documentos para liberar espacio?"

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT_NAME} installer"
VIAddVersionKey /LANG=1033 "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright (C) 2026 ${PRODUCT_PUBLISHER}"

Function .onInit
    ${If} ${RunningX64}
    ${Else}
        MessageBox MB_OK|MB_ICONSTOP "This application requires a 64-bit Windows operating system."
        Abort
    ${EndIf}
FunctionEnd

Section "${PRODUCT_NAME}" SecMain
    ${DisableX64FSRedirection}

    SetOutPath "$INSTDIR"
    File /r "${SOURCE_DIR}\*.*"

    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${PRODUCT_EXE}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${PRODUCT_EXE}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Section "Uninstall"
    ${DisableX64FSRedirection}

    MessageBox MB_YESNO|MB_ICONQUESTION "$(MSG_REMOVE_MODELS)" IDNO skip_models
        Delete "$DOCUMENTS\Ada\AdaOffline.Q4_K_M.gguf"
        RMDir /r "$DOCUMENTS\Ada"
    skip_models:

    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
    RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"
    
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
    
    RMDir /r "$INSTDIR"
SectionEnd