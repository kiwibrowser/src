# The Vulkan runtime installer NSIS script

!include LogicLib.nsh

# Input file locations
!define RES "."

# Input parameters
!ifndef MAJOR
    !define MAJOR "1"
    !define MINOR "1"
    !define PATCH "73"
    !define BUILD "0.dev"
!endif
!define VERSION "${MAJOR}.${MINOR}.${PATCH}.${BUILD}"
!ifndef PUBLISHER
    !define PUBLISHER "YourCompany, Inc."
!endif
!ifndef COPYRIGHT
    !define COPYRIGHT ""
!endif

# Installer information
Icon ${RES}\V.ico
OutFile "VulkanRT-${VERSION}-Installer.exe"
InstallDir "$PROGRAMFILES\VulkanRT"

RequestExecutionLevel admin
AddBrandingImage left 150
Caption "Vulkan Runtime ${VERSION} Setup"
Name "Vulkan Runtime ${VERSION}"
LicenseData "${RES}\VulkanRT-License.txt"
Page custom brandimage "" ": Brand Image"
Page license
Page instfiles

VIProductVersion "${VERSION}"
VIAddVersionKey  "ProductName" "Vulkan Runtime"
VIAddVersionKey  "FileVersion" "${VERSION}"
VIAddVersionKey  "ProductVersion" "${VERSION}"
VIAddVersionKey  "LegalCopyright" "${COPYRIGHT}"
VIAddVersionKey  "FileDescription" "Vulkan Runtime Installer"

Function brandimage
  SetOutPath "$TEMP"
  SetFileAttributes V.bmp temporary
  File "${RES}\V.bmp"
  SetBrandingImage "$TEMP/V.bmp"
Functionend

# Utilties to check if a file is older than this installer or not
Function NeedsReplacing
    Pop $0

    # Extract the version of the existing file
    GetDllVersion "$0" $R0 $R1
    IntOp $R2 $R0 >> 16
    IntOp $R2 $R2 & 0xffff
    IntOp $R3 $R0 & 0xffff
    IntOp $R4 $R1 >> 16
    IntOp $R4 $R4 & 0xffff
    IntOp $R5 $R1 & 0xffff

    # Check major versions
    ${IF} ${MAJOR} > $R2
        Push True
    ${ELSEIF} ${MAJOR} < $R2
        Push False

    # Check minor versions
    ${ELSEIF} ${MINOR} > $R3
        Push True
    ${ELSEIF} ${MINOR} < $R3
        Push False

    # Check patch versions
    ${ELSEIF} ${PATCH} > $R4
        Push True
    ${ELSEIF} ${PATCH} < $R4
        Push False

    # Check build versions
    ${ELSEIF} ${BUILD} > $R5
        Push True
    ${ELSEIF} ${BUILD} < $R5
        Push False

    # If they match exactly, then we update
    ${ELSE}
        Push True
    ${ENDIF}
FunctionEnd

!macro InstallIfNewer SrcPath OutName
    Push "$OUTDIR\${OutName}"
    Call NeedsReplacing
    Pop $0

    ${IF} $0 == True
        DetailPrint "File $OUTDIR\${OutName} (version $R2.$R3.$R4.$R5) will be upgraded to ${VERSION}"
        File /oname=${OutName} "${SrcPath}"
    ${ELSE}
        DetailPrint "File $OUTDIR\${OutName} (version $R2.$R3.$R4.$R5) will not be replaced with ${VERSION}"
    ${ENDIF}
!macroend

# Utilities to check if this is a 64-bit OS or not
!define IsWow64 `"" IsWow64 ""`
!macro _IsWow64 _a _b _t _f
  !insertmacro _LOGICLIB_TEMP
  System::Call kernel32::GetCurrentProcess()p.s
  System::Call kernel32::IsWow64Process(ps,*i0s)
  Pop $_LOGICLIB_TEMP
  !insertmacro _!= $_LOGICLIB_TEMP 0 `${_t}` `${_f}`
!macroend

!define RunningX64 `"" RunningX64 ""`
!macro _RunningX64 _a _b _t _f
  !if ${NSIS_PTR_SIZE} > 4
    !insertmacro LogicLib_JumpToBranch `${_t}` `${_f}`
  !else
    !insertmacro _IsWow64 `${_a}` `${_b}` `${_t}` `${_f}`
  !endif
!macroend

# Installer
Section
    Delete "$INSTDIR\install.log"
    Delete "$INSTDIR\VULKANRT_LICENSE.rtf"
    LogSet on

    # Disable filesystem redirection
    System::Call kernel32::Wow64EnableWow64FsRedirection(i0)

    ${IF} ${RunningX64}
        SetOutPath $WINDIR\System32
        !insertmacro InstallIfNewer "${LOADER64}" "vulkan-1.dll"
        !insertmacro InstallIfNewer "${LOADER64}" "vulkan-1-999-0-0-0.dll"
        !insertmacro InstallIfNewer "${VULKANINFO64}" "vulkaninfo.exe"
        !insertmacro InstallIfNewer "${VULKANINFO64}" "vulkaninfo-1-999-0-0-0.exe"
        SetOutPath $WINDIR\SysWOW64
    ${ELSE}
        SetOutPath $WINDIR\System32
    ${ENDIF}

    # Install 32-bit contents
    !insertmacro InstallIfNewer "${LOADER32}" "vulkan-1.dll"
    !insertmacro InstallIfNewer "${LOADER32}" "vulkan-1-999-0-0-0.dll"
    !insertmacro InstallIfNewer "${VULKANINFO32}" "vulkaninfo.exe"
    !insertmacro InstallIfNewer "${VULKANINFO32}" "vulkaninfo-1-999-0-0-0.exe"

    # Dump licenses into a the installation directory
    SetOutPath "$INSTDIR"
    AccessControl::DisableFileInheritance $INSTDIR
    AccessControl::SetFileOwner $INSTDIR "Administrators"
    AccessControl::ClearOnFile  $INSTDIR "Administrators" "FullAccess"
    AccessControl::SetOnFile    $INSTDIR "SYSTEM" "FullAccess"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "ListDirectory"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "GenericExecute"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "GenericRead"
    AccessControl::GrantOnFile  $INSTDIR "Everyone" "ReadAttributes"
    File "${RES}\VulkanRT-License.txt"

    LogSet off
SectionEnd
