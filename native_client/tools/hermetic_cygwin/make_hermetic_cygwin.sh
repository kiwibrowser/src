#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

declare NSIS=0 verboase=0

while getopts "nv" flag ; do
  case "$flag" in
    n) NSIS=1 ;;
    v) verbose=1 ;;
    ?) cat <<END
  Usage: $0 [-n] [-v]
  Flags:
    -n: run NSIS to create actual installer
    -v: be more verbose when processing data
    Place CygWin decription file (setup.ini) in current directory, run script.
    You'll get:
      1. Downloaded binary files in subdirectory "packages"
      2. Downloaded source files in subdirectory "packages.src"
      3. Unpacked files in subdirectory "packages.unpacked"
      4. Setup log files in subdirectory "setup"
      5. Ready to use file make_installer.nsi for NSIS installer
   setup.ini is here: http://mirrors.kernel.org/sourceware/cygwin/setup.ini
   It's not downloaded by script to keep it hermetic.

    NSIS file format is described here: http://nsis.sourceforge.net/Docs
END
      exit 1;;
  esac
done

declare CygWin=0
if [[ "`uname -s`" != "Darwin" ]] ; then
  if [[ "`uname -o`" = "Cygwin" ]] ; then
    CygWin=1
    declare need_restart=0
    if ((BASH_VERSINFO[0]<4)) ; then
      need_restart=1
    fi
    if ((NSIS)) && ! [ -x NSIS/makensis.exe ] && ! [ -x /bin/7z ] ; then
      need_restart=1
    fi
  fi
fi

# Can only happen on CygWin - we don't autoinstall tools on other platforms
if ((need_restart)) ; then
  if ! [ -x "$PWD/hermetic_cygwin/bin/7z" ] && ! [ -x "$PWD/hermetic_cygwin/bin/7z.exe" ] ; then
    wget https://storage.googleapis.com/nativeclient-mirror/nacl/cygwin_mirror/hermetic_cygwin_1_7_9-0_1.exe -O cygwin_mini_setup.exe
    chmod a+x cygwin_mini_setup.exe
    "`cygpath $COMSPEC`" /C start /WAIT ".\\cygwin_mini_setup" /CYGPORT /S "/D=`cygpath -w $PWD/hermetic_cygwin`"
  fi
  exec "`cygpath $COMSPEC`" /C "`cygpath -w $PWD/hermetic_cygwin/bin/bash`" "`cygpath -w $0`" "$@"
fi

if ((BASH_VERSINFO[0]<4)) ; then
  echo "You need Bash4 to use this script" >&2
  exit 1
fi

if ((NSIS)) && ((CygWin)) && ! [ -d NSIS ] ; then
  7z -oNSIS x ../../../third_party/NSIS/nsis-2.46-Unicode-setup.exe
  ln -sfn NSIS AccessControl
  7z x ../../../third_party/NSIS/AccessControl.zip
  rm AccessControl
  mkdir -p NSIS/Contrib/Graphics/{Checks,Header,Icons,Wizard}
  for dirname in Checks Header Icons Wizard ; do
    mv NSIS/\$_OUTDIR/$dirname/* NSIS/Contrib/Graphics/$dirname
  done
  rmdir NSIS/\$_OUTDIR/{Checks,Header,Icons,Wizard,}
  mkdir "NSIS/Docs/Modern UI/images"
  ln "NSIS/Docs/Modern UI 2/images"/* "NSIS/Docs/Modern UI/images"
  mv NSIS/\$PLUGINSDIR/modern-header.bmp NSIS/Contrib/Graphics/Header/nsis.bmp
  mv NSIS/\$PLUGINSDIR/modern-wizard.bmp NSIS/Contrib/Graphics/Wizard/nsis.bmp
  mv NSIS/\$PLUGINSDIR/*.dll NSIS/Plugins
  rmdir NSIS/\$PLUGINSDIR
  chmod a+x NSIS/{,Bin,Contrib/UIs}/*.exe
  mkdir -p "MkLink/nsis"
  cp -aiv "NSIS/Examples/Plugin/nsis/"* "Mklink/nsis"
  cp -aiv "MkLink/Release Unicode/MkLink.dll" "NSIS/Plugins"
fi

declare -A description packages
. "${0/.sh/.conf}"
. "`dirname \"$0\"`"/make_installer.inc

CYGWIN_VERSION=1.7.24-1.0

mkdir -p packages{,.src,.unpacked} setup

parse_setup_ini
fix_setup_inf_info
download_package_dependences bash 0
reqpackages=()
sectionin=()
allinstpackages=()
allinstalledpackages=()
rm setup/*.lst.gz
download_package "Base" "`seq -s ' ' \"$((${#packages[@]}+3))\"`"
download_addon_packages 2
if ((include_all_packages)) ; then
  download_all_packages 1
else
  for pkgname in "${!sectionin[@]}" ; do
    sectionin["$pkgname"]=" 1${sectionin[$pkgname]}"
  done
  for pkgname in "${!seed[@]}" ; do
    seed["$pkgname"]=" 1${seed[$pkgname]}"
  done
fi
fill_required_packages
fill_filetype_info

(
  cat <<END
RequestExecutionLevel user
SetCompressor $compressor
SetCompressorDictSize 128
Name "Hermetic CygWin"
OutFile hermetic_cygwin_${CYGWIN_VERSION//./_}.exe
END
  declare_nsis_variables
  cat <<END

InstallDir "c:\\cygwin"

!include "MUI2.nsh"
!include "Sections.nsh"

!define MUI_HEADERIMAGE
!define MUI_WELCOMEFINISHPAGE_BITMAP "\${NSISDIR}\\Contrib\\Graphics\\Wizard\\win.bmp"

!define MUI_WELCOMEPAGE_TITLE "Welcome to Hermetic CygWin"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of Hermetic CygWin $CYGWIN_VERSION.\$\\r\$\\n\$\\r\$\\nHermetic CygWin is simple and reliable way to install pre-defined version of CygWin.\$\\r\$\\n\$\\r\$\\n\$_CLICK"

!define MUI_COMPONENTSPAGE_SMALLDESC

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_LINK "Visit the Native Client site for news, FAQs and support"
!define MUI_FINISHPAGE_LINK_LOCATION "http://code.google.com/chrome/nativeclient"

!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "" sec_Preinstall
  SectionIn `seq -s ' ' $((${#packages[@]}+3))`
  Push \$R0
  CreateDirectory "\$INSTDIR"
  ; Owner can do anything
  AccessControlW::GrantOnFile "\$INSTDIR" "(S-1-3-0)" "FullAccess"
  ; Group can read
  AccessControlW::GrantOnFile "\$INSTDIR" "(S-1-3-1)" "Traverse + GenericRead"
  ; "Everyone" can read too
  AccessControlW::GrantOnFile "\$INSTDIR" "(S-1-1-0)" "Traverse + GenericRead"
  CreateDirectory "\$INSTDIR\\etc"
  CreateDirectory "\$INSTDIR\\etc\\setup"
  FileOpen \$R0 \$INSTDIR\\postinstall.sh w
  FileWrite \$R0 'export PATH=/usr/local/bin:/usr/bin:/bin\$\nexport CYGWIN="\$\$CYGWIN nodosfilewarning"\$\n'
  FileClose \$R0
  FileOpen \$R0 \$INSTDIR\\etc\\setup\\installed.log w
  FileWrite \$R0 "INSTALLED.DB 2\$\n"
  FileClose \$R0
  Pop \$R0
SectionEnd
END
  generate_section_list
  cat <<END
Section "" sec_PostInstall
  SectionIn `seq -s ' ' $((${#packages[@]}+3))`
  Push \$R0
  Push \$R1
  FileOpen \$R0 \$INSTDIR\\postinstall.sh a
  FileSeek \$R0 0 END
  FileWrite \$R0 "/bin/sort /etc/setup/installed.log -o /etc/setup/installed.db\$\nrm /etc/setup/installed.log\$\n"
  FileClose \$R0
  SetOutPath \$INSTDIR
  nsExec::ExecToLog '"bin\\bash" -c ./postinstall.sh'
  nsExec::ExecToLog '"bin\dash" -c "bin/rebaseall -v"'
  Delete \$INSTDIR\\postinstall.sh
  FileOpen \$R0 \$INSTDIR\\Cygwin.bat w
  StrCpy \$R1 \$INSTDIR 1
  FileWrite \$R0 "@echo off\$\r\$\n\$\r\$\n\$R1:$\r\$\nchdir \$INSTDIR\\bin\$\r\$\nbash --login -i$\r\$\n"
  FileClose \$R0
  Pop \$R1
  Pop \$R0
SectionEnd
END
  generate_init_function 2
  generate_onselchange_function
) > make_hermetic_cygwin.nsi
# Replace symlinks with hardlinks for python and gawk
if ! patch --no-backup-if-mismatch <<END
--- make_hermetic_cygwin.nsi
+++ make_hermetic_cygwin.nsi
@@ -2295,2 +2295,2 @@
   MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\gawk.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\gawk-4.1.0.exe"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\awk.exe" "gawk.exe"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\awk.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\gawk-4.1.0.exe"
@@ -5370,6 +5370,7 @@
   CreateDirectory "\$INSTDIR\\usr"
   CreateDirectory "\$INSTDIR\\bin"
   CreateDirectory "\$INSTDIR\\bin-unrebased"
+  CreateDirectory "\$INSTDIR\\dev"
   CreateDirectory "\$INSTDIR\\usr\\share"
   CreateDirectory "\$INSTDIR\\usr\\share\\doc"
   CreateDirectory "\$INSTDIR\\usr\\share\\doc\\bash"
@@ -21338,3 +21338,4 @@
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python.exe" "python2.7.exe"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python-config" "python2.7-config"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}lib\\libpython2.7.dll.a" "python2.7\\config\\libpython2.7.dll.a"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python2.7.exe"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python2.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python2.7.exe"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python-config" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python2.7-config"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}lib\\libpython2.7.dll.a" "\$INSTDIR\\${CYGWIN_PREFIX}lib\\python2.7\\config\\libpython2.7.dll.a"
@@ -34953,6 +34953,10 @@
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\pydoc3" "pydoc3.2"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.exe" "python3.2m.exe"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3-config" "python3.2m-config"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.2.exe" "python3.2m.exe"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}lib\\libpython3.2m.dll.a" "python3.2\\config-3.2m\\libpython3.2m.dll.a"
-  MkLink::SoftF "\$INSTDIR\\${CYGWIN_PREFIX}lib\\pkgconfig\\python3.pc" "python-3.2.pc"
+  IntCmp \$PKV_python 1 L1_NoInstallPythonHardlink +1 L1_NoInstallPythonHardlink
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.2m.exe"
+L1_NoInstallPythonHardlink:
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\pydoc3" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\pydoc3.2"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.2m.exe"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3-config" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.2m-config"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.2.exe" "\$INSTDIR\\${CYGWIN_PREFIX}bin\\python3.2m.exe"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}lib\\libpython3.2m.dll.a" "\$INSTDIR\\${CYGWIN_PREFIX}lib\\python3.2\\config-3.2m\\libpython3.2m.dll.a"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}lib\\pkgconfig\\python.pc" "\$INSTDIR\\${CYGWIN_PREFIX}lib\\pkgconfig\\python-3.2.pc"
+  MkLink::Hard "\$INSTDIR\\${CYGWIN_PREFIX}lib\\pkgconfig\\python3.pc" "\$INSTDIR\\${CYGWIN_PREFIX}lib\\pkgconfig\\python-3.2.pc"
END
  then
    exit 1
fi
if ((NSIS)) ; then
  if [ -e NSIS/makensis.exe ] ; then
    NSIS/makensis.exe /V2 make_hermetic_cygwin.nsi
  else
    makensis /V2 make_hermetic_cygwin.nsi
  fi
fi
