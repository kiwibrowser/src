# Copyright 2012, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# Information from setup.ini
declare -A {s,l}desc category require{d,s} prefix version install source message
# Contents of categories
declare -A category_contents
# Packagenames: in request order and in install order
declare -A allinstpackages instpackages reqpackages sectionin seed
declare -a allinstalledpackages
# Filetype info
declare -A filetype
# Sanitized names cache
declare -A sanitized_name
# Default installation type - filled in download_addon_packages if there are options
declare definsttype=1

parse_setup_ini() {
  if ((verbose)) ; then echo "Parsing setup.ini..." >&2 ; fi
  local package line acceptmode=0 sdescmode=0 ldescmode=0 messagemode=0
  while IFS= read -r line ; do
    if [[ "${line:0:1}" = "@" ]] ; then
      package="${line:2}"
      acceptmode=1
      sdescmode=0
      ldescmode=0
      messagemode=0
      if ((verbose)) ; then echo "Package: \"$package\"..." >&2 ; fi
    elif [[ "$line" = "[prev]" ]] || [[ "$line" = "[test]" ]] ; then
      acceptmode=0
    elif ((acceptmode)) ; then
      if ((sdescmode)) ; then
        sdesc["$package"]="${sdesc[$package]} $line"
        if [[ "$line" = *\"* ]] ; then
          sdescmode=0
        fi
      elif ((ldescmode)) ; then
        ldesc["$package"]="${ldesc[$package]} $line"
        if [[ "$line" = *\"* ]] ; then
          ldescmode=0
        fi
      elif ((messagemode)) ; then
        message["$package"]="${ldesc[$package]} $line"
        if [[ "$line" = *\"* ]] ; then
          messagemode=0
        fi
      elif [[ "${line:0:6}" = "sdesc:" ]] ; then
        sdesc["$package"]="${line:7}"
        if [[ "${line:7}" = *\"*\"* ]] ; then
          sdescmode=0
        else
          sdescmode=1
        fi
        ldescmode=0
        messagemode=0
      elif [[ "${line:0:6}" = "ldesc:" ]] ; then
        ldesc["$package"]="${line:7}"
        sdescmode=0
        if [[ "${line:7}" = *\"*\"* ]] ; then
          ldescmode=0
        else
          ldescmode=1
        fi
        messagemode=0
      elif [[ "${line:0:9}" = "category:" ]] ; then
        category["$package"]="${line:10}"
        for catname in ${category["$package"]} ; do
          category_contents["$catname"]="${category_contents[$catname]} $package"
        done
        sdescmode=0
        ldescmode=0
        messagemode=0
      elif [[ "${line:0:9}" = "requires:" ]] ; then
        line="${line/ _autorebase/}"
        requires["$package"]="${line:10}"
        sdescmode=0
        ldescmode=0
        messagemode=0
      elif [[ "${line:0:8}" = "version:" ]] ; then
        version["$package"]="${line:9}"
        sdescmode=0
        ldescmode=0
        messagemode=0
      elif [[ "${line:0:8}" = "install:" ]] ; then
        install["$package"]="${line:9}"
        sdescmode=0
        ldescmode=0
        messagemode=0
      elif [[ "${line:0:7}" = "source:" ]] ; then
        source["$package"]="${line:8}"
        sdescmode=0
        ldescmode=0
        messagemode=0
      elif [[ "${line:0:8}" = "message:" ]] ; then
        message["$package"]="${line:9}"
        sdescmode=0
        ldescmode=0
        if [[ "${line:7}" = *\"*\"* ]] ; then
          messagemode=0
        else
          messagemode=1
        fi
      elif [[ "$line" != "" ]] &&
           [[ "$line" != "#"* ]] ; then
        echo "Unknown tag in \"$line\"" >&2
        exit 1
      fi
    fi
  done < setup.ini
}

fix_setup_inf_info() {
  sdesc["python-lxml"]="\"Python XML2/XSLT bindings\""
  ldesc["python-lxml"]="\"lxml is a Pythonic binding for the libxml2 and libxslt libraries. It is unique in that it combines the speed and feature completeness of these libraries with the simplicity of a native Python API, mostly compatible but superior to the well-known ElementTree API.\""
  category["python-lxml"]="Python"
  category_contents["Python"]="${category_contents[Python]} python-lxml"
  requires["python-lxml"]="libxml2 libxslt python python-beautifulsoup"
  version["python-lxml"]="3.2.3-1"
  install["python-lxml"]="x86/release/python-lxml/python-lxml-3.2.3-1.tar.bz2 705772 1be0ee09ad688ead386f61702db23ffe"
  source["python-lxml"]="x86/release/python-lxml/python-lxml-3.2.3-1-src.tar.bz2 3353561 977cc672b83383e395b0e0900bc802e1"

  sdesc["python3-lxml"]="\"Py3K XML2/XSLT bindings\""
  ldesc["python3-lxml"]="\"lxml is a Pythonic binding for the libxml2 and libxslt libraries. It is unique in that it combines the speed and feature completeness of these libraries with the simplicity of a native Python API, mostly compatible but superior to the well-known ElementTree API.\""
  category["python3-lxml"]="Python"
  category_contents["Python"]="${category_contents[Python]} python3-lxml"
  requires["python3-lxml"]="libxml2 libxslt python3"
  version["python3-lxml"]="3.2.3-1"
  install["python3-lxml"]="x86/release/python3-lxml/python3-lxml-3.2.3-1.tar.bz2 691721 4e9250266857afbe1cafc7024d42ad36"
  source["python3-lxml"]="x86/release/python3-lxml/python3-lxml-3.2.3-1-src.tar.bz2 3353519 93e02ae23051cecdef3ff820f885de4b"

  requires["coreutils"]="${requires[coreutils]} dash grep patch sed"
  requires["cygwin"]="base-files ${requires[cygwin]} editrights ipc-utils run"
  requires["dash"]="${requires[dash]} cygwin"
  requires["gettext"]="${requires[gettext]} libiconv libiconv2"
}

download_package_dependences() {
  # $1 = name of the package; function is recursive
  # $2 - Section No
  local instname="${install[$1]/.tar.bz2*/.tar.bz2}"
  local srcname="${source[$1]/.tar.bz2*/.tar.bz2}"
  if [[ "$instname" != "" ]] ; then
    if ((verbose)) ; then ( echo ; echo -n "Package: \"$1\"..." ) >&2 ; fi
    if [ ! -e packages/"`basename \"$instname\"`" ] ; then
      # TODO(bradnelson): Check certs when this issue is resolved.
      #     http://code.google.com/p/nativeclient/issues/detail?id=2931
      if wget --no-check-certificate https://storage.googleapis.com/nativeclient-mirror/nacl/cygwin_mirror/packages/"`basename \"$instname\"`" -O packages/"`basename \"$instname\"`" ||
         wget "$mirror_prefix/$instname" -O packages/"`basename \"$instname\"`" ; then
        if ! [[ "${install[$1]}" = "${install[$1]/.tar.bz2*/.tar.bz2} `
                stat -c "%s" packages/\"\`basename \\\"$instname\\\"\`\"` `
                md5sum -b packages/\"\`basename \\\"$instname\\\"\`\" | cut -b -32`" ]] ; then
          echo "Wrong file downloaded: expected ${install[$1]}" >&2
          exit 1
        fi
      else
        # Wget should have complained for us
        exit $?
      fi
    fi
    if [ ! -e packages.unpacked/"`basename \"$instname\"`" ] ; then
      mkdir -p packages.unpacked/"`basename \"$instname\"`"
      if ! tar xSvpf {packages,-Cpackages.unpacked}/"`basename \"$instname\"`" ; then
        # Tar should have complained for us
        exit $?
      fi
      if [[ "$1" = "base-files" ]] ; then
        if ! patch -p0 --no-backup-if-mismatch <<END
--- packages.unpacked/`basename "$instname"`/etc/postinstall/base-files-mketc.sh
+++ packages.unpacked/`basename "$instname"`/etc/postinstall/base-files-mketc.sh
@@ -14,9 +14,14 @@
 WINETC="\${WINSYS32HOME}\\\\drivers\\\\etc"
 
 if [ ! -d "\${WINETC}" ]; then
-  echo "Directory \${WINETC} does not exist; exiting" >&2
-  echo "If directory name is garbage you need to update your cygwin package" >&2
-  exit 1
+  # Vista64 is crazy: symlink is usable, but "-f" to the original is false!
+  if ln -s "\`/bin/cygpath -w -W\`\\\\sysnative\\\\drivers\\\\etc\\\\lmhosts.sam" /etc/lmhosts.sam && [ -f /etc/lmhosts.sam ] && rm /etc/lmhosts.sam ; then
+    WINETC="\`/bin/cygpath -w -W\`\\\\sysnative\\\\drivers\\\\etc"
+  else
+    echo "Directory \${WINETC} does not exist; exiting" >&2
+    echo "If directory name is garbage you need to update your cygwin package" >&2
+    exit 1
+  fi
 fi
 
 for mketc in \$FILES
END
        then
          # Patch should have complained for us
          exit $?
        fi
      elif [[ "$1" = "WindowMaker" ]] ; then
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/WindowMaker/Icons/{,tmp}defaultAppIcon.tiff
        rm packages.unpacked/"`basename \"$instname\"`"/usr/share/WindowMaker/Icons/DefaultAppIcon.tiff 2> /dev/null
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/WindowMaker/Icons/{tmp,}defaultAppIcon.tiff
      elif [[ "$1" = "lilypond-doc" ]] ; then
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/doc/lilypond/html/Documentation/user/lilypond-learning/{,tmp}Lilypond_002dbook-templates.de.html
        rm packages.unpacked/"`basename \"$instname\"`"/usr/share/doc/lilypond/html/Documentation/user/lilypond-learning/lilypond_002dbook-templates.de.html
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/doc/lilypond/html/Documentation/user/lilypond-learning/{tmp,}Lilypond_002dbook-templates.de.html
      elif [[ "$1" = "ruby" ]] ; then
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/ri/1.8/system/Exception2MessageMapper/{,tmp}Fail-i.yaml
        rm packages.unpacked/"`basename \"$instname\"`"/usr/share/ri/1.8/system/Exception2MessageMapper/fail-i.yaml
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/ri/1.8/system/Exception2MessageMapper/{tmp,}Fail-i.yaml
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/ri/1.8/system/REXML/Parsers/XPathParser/{,tmp}Predicate-i.yaml
        rm packages.unpacked/"`basename \"$instname\"`"/usr/share/ri/1.8/system/REXML/Parsers/XPathParser/predicate-i.yaml
        mv packages.unpacked/"`basename \"$instname\"`"/usr/share/ri/1.8/system/REXML/Parsers/XPathParser/{tmp,}tmpPredicate-i.yaml
      fi
      for file in packages.unpacked/"`basename \"$instname\"`"/etc/{postinstall,preremove}/*.sh ; do
        if [ -e "$file" ] ; then
          mv "$file" "$file".tmp
          while IFS= read -r line ; do
            if [[ "$line" != "while read f; do" ]] ; then
              printf "%s\n" "$line"
            else
              echo "while read f; do if [ \"x\$f\" = \"x\" ] ; then break ; fi"
            fi
          done < "$file".tmp > "$file"
          touch -r "$file".tmp "$file"
          rm "$file".tmp
        fi
      done
    fi
    if [ ! -e setup/"$1".lst.gz ] ; then
      if ! rm -f setup/"$1".lst ; then
        # Rm should have complained for us
        exit $?
      fi
      if ! tar tf {packages,-Cpackages.unpacked}/"`basename \"$instname\"`" > setup/"$1".lst ; then
        # Tar should have complained for us
        exit $?
      fi
      if ! gzip -9 setup/"$1".lst ; then
        # Gzip should have complained for us
        exit $?
      fi
    fi
  fi
  if [[ "$srcname" != "" ]] ; then
    if [ ! -e packages.src/"`basename \"$srcname\"`" ] ; then
      # TODO(bradnelson): Check certs when this issue is resolved.
      #     http://code.google.com/p/nativeclient/issues/detail?id=2931
      if wget --no-check-certificate https://storage.googleapis.com/nativeclient-mirror/nacl/cygwin_mirror/packages.src/"`basename \"$srcname\"`" -O packages.src/"`basename \"$srcname\"`" ||
         wget "$mirror_prefix/$srcname" -O packages.src/"`basename \"$srcname\"`" ; then
        if ! [[ "${source[$1]}" = "${source[$1]/.tar.bz2*/.tar.bz2} `
                stat -c "%s" packages.src/\"\`basename \\\"$srcname\\\"\`\"` `
                md5sum -b packages.src/\"\`basename \\\"$srcname\\\"\`\" | cut -b -32`" ]] ; then
          echo "Wrong file downloaded: expected ${install[$1]}" >&2
          exit 1
        fi
      else
        # Wget should have complained for us
        exit $?
      fi
    fi
  fi
  local pkgname
  reqpackages["$1"]="present"
  sectionin["$1"]="${sectionin[$1]} $2"
  for pkgname in ${requires["$1"]} ; do
    if [[ "${reqpackages[$pkgname]}" = "present" ]] ; then
      if ((verbose)) ; then echo -n "." >&2 ; fi
    else
      download_package_dependences "$pkgname" "$2"
    fi
  done
  if [[ "${allinstpackages[$1]}" != "present" ]] ; then
    allinstpackages["$1"]="present"
    allinstalledpackages=(${allinstalledpackages[@]} "$1")
  fi
}

download_package() {
  # $1 - Category name
  # $2 - Section No
  if ((verbose)) ; then echo -n "Downloading packages in \"$1\"..." >&2 ; fi
  local pkgname
  reqpackages=()
  for pkgname in ${category_contents["$1"]} ; do
    if [[ "${reqpackages[$pkgname]}" != "present" ]] ; then
      download_package_dependences "$pkgname" "$2"
    fi
    seed["$pkgname"]="${seed[$pkgname]} $2"
  done
  if ((verbose)) ; then echo >&2 ; fi
}

download_all_packages() {
  # $1 - Section No
  local secname
  for secname in "${!category_contents[@]}" ; do
    if [ "$secname" != "_obsolete" ] ; then
      download_package "$secname" "$1"
    fi
  done
}

download_addon_packages() {
  # $1 - Start SectionN
  local insttype pkgname sectionN="$1"
  if ((verbose)) ; then echo -n "Downloading addon packages..." >&2 ; fi
  if [[ "$defpackage" = "Full" ]] ; then
    definsttype="$((sectionN-1))"
  fi
  local IFS=$'\n'
  for insttype in `
    for insttype in "${!packages[@]}" ; do
      echo $insttype
    done | sort
  ` ; do
    if [[ "$defpackage" = "$insttype" ]] ; then
      definsttype="$sectionN"
    fi
    reqpackages=()
    IFS=$' \t\n'
    for pkgname in ${packages["$insttype"]} ; do
      if [[ "${reqpackages[$pkgname]}" != "present" ]] ; then
        download_package_dependences "$pkgname" "$sectionN"
      fi
      seed["$pkgname"]="${seed[$pkgname]} $sectionN"
    done
    ((sectionN++))
  done
  if [[ "$defpackage" = "Minimal" ]] ; then
    definsttype="$sectionN"
  fi
  if ((verbose)) ; then echo >&2 ; fi
}

fill_required_packages() {
  if ((verbose)) ; then echo "Calculating dependences..." >&2 ; fi
  local pkgname subpkgname subsubpkgname check=1
  while ((check)) ; do
    check=0
    for pkgname in "${allinstalledpackages[@]}" ; do
      if ((verbose)) ; then echo "Package: \"$pkgname\"..." >&2 ; fi
      for subpkgname in ${requires["$pkgname"]} ; do
        for subsubpkgname in ${requires["$subpkgname"]} ; do
          if ! [[ " ${requires[$pkgname]} " = *" $subsubpkgname "* ]] ; then
            requires["$pkgname"]="${requires[$pkgname]} $subsubpkgname"
            check=1
          fi
        done
      done
    done
  done
  for pkgname in "${allinstalledpackages[@]}" ; do
    for subpkgname in ${requires["$pkgname"]} ; do
      required["$subpkgname"]="${required[$subpkgname]} $pkgname"
    done
  done
}

fill_filetype_info() {
  local IFS="
"
  # Corner cases: files created by post-install scripts, mistakes, etc
  filetype=([Cygwin.ico]="file"
            [bin]="directory"
            [usr/bin]="directory"
            [lib]="directory"
            [usr/lib]="directory"
            [bin/exim.exe]="file"
            [bin/latex.exe]="file"
            [lib/icu/current/Makefile.inc]="file"
            [lib/rpm/rpmv.exe]="file"
            [usr/sbin/sendmail]="file"
            [usr/share/man/man1/gcc.1.gz]="file"
            [usr/share/man/man1/g++.1.gz]="file"
            [usr/share/man/man1/g77.1.gz]="file"
            [usr/share/man/man1/mf.1]="file"
            [usr/share/terminfo/E/Eterm]="file"
            [usr/share/terminfo/N/NCR260VT300WPP0]="file")
  if ((verbose)) ; then echo "Scanning filetypes..." >&2 ; fi
  for name in `
    for package in packages.unpacked/* ; do (
      cd $package
      if ((verbose)) ; then echo "Archive: \"${package#*/}\"..." >&2 ; fi
      find -type f
    ) done
  ` ; do
    if [[ "${name:0:10}" = "./usr/bin/" ]] ; then
      filetype["bin/${name:10}"]="file"
    elif [[ "${name:0:10}" = "./usr/lib/" ]] ; then
      filetype["lib/${name:10}"]="file"
    else
      filetype["${name:2}"]="file"
    fi
  done
  for name in `
    for package in packages.unpacked/* ; do (
      cd $package
      if ((verbose)) ; then echo "Archive: \"${package#*/}\"..." >&2 ; fi
      find -type d
    ) done
  ` ; do
    if [[ "${name:0:10}" = "./usr/bin/" ]] ; then
      if [[ "bin/${filetype[${name:10}]}" = "file" ]] ; then
        echo "bin/${filetype[${name:10}]} - file and directory... oops?" >&2
        exit 1
      fi
      filetype["bin/${name:10}"]="directory"
    elif [[ "${name:0:10}" = "./usr/lib/" ]] ; then
      if [[ "lib/${filetype[${name:10}]}" = "file" ]] ; then
        echo "lib/${filetype[${name:10}]} - file and directory... oops?" >&2
        exit 1
      fi
      filetype["lib/${name:10}"]="directory"
    elif ((${#name}>1)) ; then
      if [[ "${filetype[${name:2}]}" = "file" ]] ; then
        echo "${filetype[${name:2}]} - file and directory... oops?" >&2
        exit 1
      fi
      filetype["${name:2}"]="directory"
    fi
  done
}

sanitize_var_name() {
  local varname="$1"
  varname="${varname//-/_}"
  varname="${varname//./_}"
  varname="${varname//+/X}"
  echo -n "$varname"
}

declare_nsis_variables() {
  local outlist
  local pkgname
  if ((verbose)) ; then echo "Declare NSIS variables..." >&2 ; fi
  if [[ "$defpackage" != "NONE" ]] ; then
    echo "InstType \"Full\""
    for pkgname in "${!packages[@]}" ; do
      echo "InstType \"$pkgname\""
    done | sort
    cat <<END
InstType "Minimal"

Var SVV_CmdLineParameters
Var SVV_InstType
Var SVV_SelChangeInProgress

!include "FileFunc.nsh"

END
  fi
  for pkgname in "${allinstalledpackages[@]}" ; do
    sanitized_name["$pkgname"]="`sanitize_var_name \"$pkgname\"`"
    outlist="$outlist
Var PKV_${sanitized_name[$pkgname]}"
  done
  echo "$outlist" | sort
  local secname
  for secname in "${!category_contents[@]}" ; do
    echo "$secname"
  done | sort |
  while read -r secname ; do
    local pkgname
    for pkgname in ${category_contents["$secname"]} ; do
      echo "$pkgname"
    done | sort |
    while read -r pkgname ; do
      if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
        echo "Var sv_${secname}_${sanitized_name[$pkgname]}"
      fi
    done
  done |
  sort
}

generate_unpack_tarball_text() {
  # $1 - package to unpack
  echo "  SetOutPath \$INSTDIR"
  local pkgname="$1"
  if [[ "$pkgname" != "" ]] ; then
    pkgname="`basename ${install[$pkgname]/.tar.bz2*/.tar.bz2}`"
  fi
  if [[ "$pkgname" != "" ]] ; then
    local pkgcontent="`tar tSvpf packages/$pkgname\"\" --numeric-owner`"
    local attrs uidgid size date time filename
    local -A createddirs
    createddirs[0]="done"
    echo "$pkgcontent" | grep "^[dhl-]" |
    while read -r attrs uidgid size date time filename ; do
      if [[ "${attrs:0:1}" = "h" ]] ; then
        filename="${filename%/* link to *}/"
      elif [[ "${attrs:0:1}" = "l" ]] ; then
        filename="${filename%/* -> *}/"
      elif [[ "${attrs:0:1}" = "-" ]] ; then
        if [[ "$filename" = */* ]] ; then
          filename="${filename%/*}/"
        else
          filename=""
        fi
      fi
      if [[ "$filename" = "usr/share/terminfo/e/" ]] ; then
        filename="usr/share/terminfo/E/"
      fi
      if [[ "${filename:0:8}" = "usr/bin/" ]] ; then
        filename="bin/${filename:8}"
      elif [[ "${filename:0:8}" = "usr/lib/" ]] ; then
        filename="lib/${filename:8}"
      fi
      if [[ "$filename" != "" ]] ; then
        filename="${filename:0:$((${#filename}-1))}"
        if [[ "${createddirs[$filename]}" != "done" ]] ; then
          echo "  CreateDirectory \"\$INSTDIR\\${prefix[$1]}${filename//\//\\}\""
          if [[ "$filename" == "bin" ]]; then
            echo "  CreateDirectory \"\$INSTDIR\\${prefix[$1]}bin-unrebased\""
          fi
          createddirs["$filename"]="done"
        fi
      fi
    done
    if [[ "$1" = "libgcj-common" ]] ; then
      echo "  CreateDirectory \"\$INSTDIR\\${prefix[$1]}lib\\security\""
    fi
    if [[ "$1" == "cygwin" ]] ; then
      echo "  File \"/oname=${prefix[cygwin]}Cygwin.ico\" \"setup\\Cygwin.ico\""
    fi
    echo "$pkgcontent" | grep "^-" |
    while read -r attrs uidgid size date time filename ; do
      if [[ "$filename" != "usr/share/WindowMaker/Icons/DefaultAppIcon.tiff" ]] &&
         [[ "$filename" != "usr/share/ri/1.8/system/Exception2MessageMapper/fail-i.yaml" ]] &&
         [[ "$filename" != "usr/share/ri/1.8/system/REXML/Parsers/XPathParser/predicate-i.yaml" ]] ; then
        local fname
        if [[ "${filename:0:8}" = "usr/bin/" ]] ; then
          fname="bin/${filename:8}"
        elif [[ "${filename:0:8}" = "usr/lib/" ]] ; then
          fname="lib/${filename:8}"
        elif [[ "$filename" = "usr/share/terminfo/e/Eterm" ]] ; then
          fname="usr/share/terminfo/E/Eterm"
        else
          fname="$filename"
        fi
        fname="${fname//\//\\}"
        fname="${fname//\$/\$\$}"
        filename="${filename//\//\\}"
        echo "  File \"/oname=${prefix[$1]}$fname\" \"packages.unpacked\\$pkgname\\$filename\""
        if [[ "$fname" = bin\\*.dll ]]; then
          echo "  File \"/oname=${prefix[$1]}bin-unrebased\\${fname:3}\" \"packages.unpacked\\$pkgname\\$filename\""
        fi
      fi
    done
    echo "  File \"/oname=${prefix[coreutils]}etc\\setup\\$1.lst.gz\" \"setup\\$1.lst.gz\""
    echo "$pkgcontent" | grep "^h" |
    while read -r attrs uidgid size date time filename ; do
      local linkname="${filename/ link to */}"
      local linktargetname="${filename/* link to /}"
      if [[ "${linkname:0:8}" = "usr/bin/" ]] ; then
        linkname="bin/${linkname:8}"
      elif [[ "${linkname:0:8}" = "usr/lib/" ]] ; then
        linkname="lib/${linkname:8}"
      fi
      if [[ "${linktargetname:0:8}" = "usr/bin/" ]] ; then
        linktargetname="bin/${linktargetname:8}"
      elif [[ "${linktargetname:0:8}" = "usr/lib/" ]] ; then
        linktargetname="lib/${linktargetname:8}"
      fi
      linkname="${linkname//\//\\}"
      linkname="${linkname//\$/\$\$}"
      linktargetname="${linktargetname//\//\\}"
      linktargetname="${linktargetname//\$/\$\$}"
      echo "  MkLink::Hard \"\$INSTDIR\\${prefix[$1]}$linkname\" \"\$INSTDIR\\${prefix[$1]}$linktargetname\""
    done
    echo "$pkgcontent" | grep "^l" |
    while read -r attrs uidgid size date time filename ; do
      local linkname="${filename/ -> */}"
      local linktargetname="${filename/* -> /}"
      if [[ "$linktargetname" = "/tmp/install/INSTALL/usr/share/doc/lynx/COPYHEADER" ]] ; then
        linktargetname="/usr/share/doc/lynx/COPYHEADER"
      elif [[ "$linktargetname" = "/tmp/install/INSTALL/usr/share/doc/lynx/COPYING" ]] ; then
        linktargetname="/usr/share/doc/lynx/COPYING"
      elif [[ "$linktargetname" = "../Cygwin/stunnel-4.29.README" ]] ; then
        linktargetname="../Cygwin/stunnel.README"
      elif [[ "$linktargetname" = "../../CHANGELOG" ]] &&
           [[ "$linkname" = "usr/X11R6/share/tcm-2.20/help/CHANGELOG" ]] ; then
        linktargetname="../../doc/tcm-2.20/CHANGELOG"
      elif [[ "$linktargetname" = "../../COPYING" ]] &&
           [[ "$linkname" = "usr/X11R6/share/tcm-2.20/help/COPYING" ]] ; then
        linktargetname="../../doc/tcm-2.20/COPYING"
      elif [[ "$linktargetname" = "mf-nowin.1" ]] &&
           [[ "$linkname" = "usr/share/man/mf.1" ]] ; then
        linkname="usr/share/man/man1/mf.1"
        linktargetname="../../../X11R6/share/man/man1/mf.1"
      elif [[ "$linktargetname" = "../N/NCR260VT300WPP0" ]] ; then
         linktargetname="../N/NCR260VT300WPP0"
      elif [[ "$linkname" = "usr/share/terminfo/e/Eterm-color" ]] ; then
        linkname="usr/share/terminfo/E/Eterm-color"
      elif [[ "$linktargetname" = "../../include/qt3" ]] &&
           [[ "$linkname" = "usr/lib/qt3/include" ]] ; then
        linktargetname= "../../usr/include/qt4"
      elif [[ "$linktargetname" = "../../include/qt4" ]] &&
           [[ "$linkname" = "usr/lib/qt4/include" ]] ; then
        linktargetname= "../../usr/include/qt4"
      elif [[ "${linktargetname:0:2}" = "./" ]] ; then
        linktargetname="${linktargetname:2}"
      elif [[ "${linkname:0:8}" = "usr/bin/" ]] ; then
        linkname="bin/${linkname:8}"
      elif [[ "${linkname:0:8}" = "usr/lib/" ]] ; then
        linkname="lib/${linkname:8}"
      fi
      if [[ "${linkname%/*}/$linktargetname" = *//* ]] ; then
        linktargetname="/${linkname%/*}/$linktargetname"
        while [[ "$linktargetname" != //* ]] ; do
          linktargetname="${linktargetname%/*//*}//../${linktargetname#*//}"
        done
        linktargetname="${linktargetname:2}"
      fi
      local linktargetfile="/${linkname%/*}/$linktargetname"
      while [[ "$linktargetfile" = */../* ]] ; do
        local linktargetprefix="${linktargetfile%%/../*}"
        local linktargetsuffix="${linktargetfile#*/../}"
        linktargetfile="${linktargetprefix%/*}/$linktargetsuffix"
      done
      if [[ "${linktargetfile:0:9}" = "/usr/bin/" ]] ; then
        linktargetfile="bin/${linktargetfile:9}"
      elif [[ "${linktargetfile:0:9}" = "/usr/lib/" ]] ; then
        linktargetfile="lib/${linktargetfile:9}"
      else
        linktargetfile="${linktargetfile:1}"
      fi
      linkname="${linkname//\//\\}"
      linkname="${linkname//\$/\$\$}"
      if [[ "${filetype[$linktargetfile]}" = "file" ]] ; then
        linktargetname="${linktargetname//\//\\}"
        linktargetname="${linktargetname//\$/\$\$}"
        if [[ "${linkname:$((${#linkname}-4))}" = ".exe" ]] ||
           [[ "${linktargetfile:$((${#linktargetfile}-4))}" != ".exe" ]] ; then
          echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname\" \"$linktargetname\""
        else
          echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname.exe\" \"$linktargetname\""
        fi
      elif [[ "${filetype[$linktargetfile.exe]}" = "file" ]] ; then
        linktargetname="${linktargetname//\//\\}"
        linktargetname="${linktargetname//\$/\$\$}"
        if [[ "${linkname:$((${#linkname}-4))}" = ".exe" ]] ; then
          echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname\" \"$linktargetname.exe\""
        else
          echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname.exe\" \"$linktargetname.exe\""
        fi
      elif [[ "${filetype[$linktargetfile]}" = "directory" ]] ; then
        linktargetname="${linktargetname//\//\\}"
        linktargetname="${linktargetname//\$/\$\$}"
        echo "  MkLink::SoftD \"\$INSTDIR\\${prefix[$1]}$linkname\" \"$linktargetname\""
      elif [ -f packages.unpacked/*/"$linktargetfile" ] ; then
        linktargetname="${linktargetname//\//\\}"
        linktargetname="${linktargetname//\$/\$\$}"
        if [[ "${linkname:$((${#linkname}-4))}" = ".exe" ]] ||
           [[ "${linktargetfile:$((${#linktargetfile}-4))}" != ".exe" ]] ; then
          echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname\" \"$linktargetname\""
        else
          echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname.exe\" \"$linktargetname\""
        fi
      elif [ -d packages.unpacked/*/"$linktargetfile" ] ; then
        linktargetname="${linktargetname//\//\\}"
        linktargetname="${linktargetname//\$/\$\$}"
        echo "  MkLink::SoftD \"\$INSTDIR\\${prefix[$1]}$linkname\" \"$linktargetname\""
      elif [[ "$linktargetname" = "../share/webcheck/webcheck.py" ]] ; then
        echo "  MkLink::SoftF \"\$INSTDIR\\${prefix[$1]}$linkname\" \"..\\share\\webcheck\\webcheck.py\""
      else
        echo "Can not determine the type of link \"$linktargetname\"" >&2
        exit 1
      fi
    done
    echo "$pkgcontent" |
    grep "^-.............................................. etc/postinstall/.*\\.sh" |
    while read -r attrs uidgid size date time filename ; do
      cat <<END
  FileOpen \$R0 \$INSTDIR\\postinstall.sh a
  FileSeek \$R0 0 END
  FileWrite \$R0 "/$filename\$\n"
  FileClose \$R0
END
      # If we have a postinstall script we need coreutils to execute it!
      if ! [[ " ${requires[$pkgname]} " = *" coreutils "* ]] ; then
        requires["$pkgname"]="${requires[$pkgname]} coreutils"
      fi
      if ! [[ " ${required[coreutils]} " = *" $pkgname "* ]] ; then
        required["coreutils"]="${requires[coreutils]} $pkgname"
      fi
    done
    cat <<END
  Push \$R0
  FileOpen \$R0 \$INSTDIR\\etc\\setup\\installed.log a
  FileSeek \$R0 0 END
  FileWrite \$R0 "$1 $pkgname 0\$\n"
  FileClose \$R0
  Pop \$R0
END
  fi
}

generate_section_list() {
  if ((verbose)) ; then echo "Declaring unpack sections..." >&2 ; fi
  local secname
  for secname in "${!category_contents[@]}" ; do
    echo "$secname"
  done | sort |
  while read -r secname ; do
    local pkgname groupprint=1
    for pkgname in ${category_contents["$secname"]} ; do
      echo "$pkgname"
    done | sort |
    { local groupprint=1
      while read -r pkgname ; do
        if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
          if ((groupprint)) ; then
            if [[ "$secname" != "_PostInstallLast" ]] &&
               [[ "$secname" != "_obsolete" ]] ; then
              echo "SectionGroup \"$secname\" sec_$secname"
              groupprint=0
            fi
          fi
          local insttar="${install[$pkgname]/.tar.bz2*/}"
          if [[ "$secname" = "_PostInstallLast" ]] ||
             [[ "$secname" = "_obsolete" ]] ; then
            echo -n "Section \"-${insttar/*\//}\" "
          elif [[ " ${seed[$pkgname]} " == *" $definsttype "* ]] ; then
            echo -n "Section \"!${insttar/*\//}\" "
          else
            echo -n "Section \"${insttar/*\//}\" "
          fi
          echo      "sec_${secname}_${sanitized_name[$pkgname]}"
          echo "  SectionIn${sectionin[$pkgname]}"
          echo "SectionEnd"
        fi
      done
      if ((!groupprint)) ; then
        echo "SectionGroupEnd"
      fi
    }
  done
  for secname in "${allinstalledpackages[@]}" ; do
    if ((verbose)) ; then echo "Package "$secname"..." >&2 ; fi
    echo "Section \"\" sec_PKG_${sanitized_name[$secname]}"
    echo "  SectionIn${sectionin[$secname]}"
    generate_unpack_tarball_text "$secname"
    echo "SectionEnd"
  done
  echo "!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN"
  for secname in "${!category_contents[@]}" ; do
    echo "$secname"
  done | sort |
  while read -r secname ; do
    local pkgname
    for pkgname in ${category_contents["$secname"]} ; do
      echo "$pkgname"
    done | sort |
    { local groupprint=1
      while read -r pkgname ; do
        if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
          if ((groupprint)) ; then
            if [[ "$secname" != "_PostInstallLast" ]] &&
               [[ "$secname" != "_obsolete" ]] ; then
              echo "  !insertmacro MUI_DESCRIPTION_TEXT \${sec_$secname} \"\""
              groupprint=0
            fi
          fi
          echo -n "  !insertmacro MUI_DESCRIPTION_TEXT \${"
          echo -n "sec_${secname}_${sanitized_name[$pkgname]}} "
          # Modern UI does not like long descriptions
          #if [ "${sdesc[$pkgname]}" != "" ] ; then
            #echo "${ldesc[$pkgname]}"
          #else
            echo  "${sdesc[$pkgname]}"
          #fi
        fi
      done
    }
  done
  echo "!insertmacro MUI_FUNCTION_DESCRIPTION_END"
}

generate_init_function() {
  # $1 - Section number for first options (others should follow)
  if ((verbose)) ; then echo "Declaring .onInit function..." >&2 ; fi
  echo "Function .onInit"
  if [[ "$defpackage" != "NONE" ]] ; then
    echo "  StrCpy \$SVV_InstType $(($definsttype-1))"
    echo "  StrCpy \$SVV_SelChangeInProgress 0"
  fi
  local pkgname
  for pkgname in "${allinstalledpackages[@]}" ; do
    if [[ " ${seed[$pkgname]} " == *" $definsttype "* ]] ; then
      echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 9"
    elif [[ " ${sectionin[$pkgname]} " == *" $definsttype "* ]] ; then
      echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 1"
    else
      echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 0"
    fi
  done
  local secname
  for secname in "${!category_contents[@]}" ; do
    echo "$secname"
  done | sort |
  while read -r secname ; do
    local pkgname
    for pkgname in ${category_contents["$secname"]} ; do
      echo "$pkgname"
    done | sort |
    while read -r pkgname ; do
      if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
        if [[ " ${sectionin[$pkgname]} " == *" $definsttype "* ]] ; then
          echo "  StrCpy \$sv_${secname}_${sanitized_name[$pkgname]} 1"
        else
          echo "  StrCpy \$sv_${secname}_${sanitized_name[$pkgname]} 0"
        fi
      fi
    done
  done
  if [[ "$defpackage" != "NONE" ]] ; then
    if ((definsttype!=1)) ; then
      echo "  SetCurInstType $((definsttype-1))"
    fi
  fi
  cat <<END
  \${GetParameters} \$SVV_CmdLineParameters
  Push \$R0
  ClearErrors
  \${GetOptions} \$cmdLineParams "/?" \$R0
  IfErrors +1 HelpMessage
  \${GetOptions} \$cmdLineParams "--help" \$R0
  IfErrors +3 +1
HelpMessage:
END
  echo -n "  MessageBox MB_OK \"Recognized common options:"
  echo -n "\$\\n  /D=InstDir - use InstDir as target instead of usual \$INSTDIR"
  echo -n "\$\\n  /NCRC - disables the CRC check"
  echo -n "\$\\n  /S - Silent install"
  if [[ "$defpackage" != "NONE" ]] ; then
    local insttype line
    echo -n "\$\\nRecognized install type options:"
    echo -n "\$\\n  /FULL - Install all availabile packages"
    for insttype in "${!packages[@]}" ; do
      echo "\$\\n  /`echo \"$insttype\" | tr 'a-z' 'A-Z'` - ${description[$insttype]}"
    done | sort |
    while IFS= read -r line ; do
      printf "%s" "$line"
    done
    echo -n "\$\\n  /MINIMAL - Install only packages from \$\\\"Base\$\\\" category"
  fi
  echo "\""
  echo "  Abort"
  if [[ "$defpackage" != "NONE" ]] ; then
    local sectionN="$(($1-1))"
    local IFS=$'\n'
    for insttype in `
      echo "Full"
      for insttype in "${!packages[@]}" ; do
        echo $insttype
      done | sort
      echo "Minimal"
    ` ; do
      echo "  \${GetOptions} \$cmdLineParams \"/`echo \"$insttype\" | tr 'a-z' 'A-Z'`\" \$R0"
      echo "  IfErrors No_$insttype +1"
      echo "  StrCpy \$SVV_InstType $((sectionN-1))"
      for pkgname in "${allinstalledpackages[@]}" ; do
        if [[ " ${seed[$pkgname]} " == *" $sectionN "* ]] ; then
          echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 9"
        elif [[ " ${sectionin[$pkgname]} " == *" $sectionN "* ]] ; then
          echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 1"
        else
          echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 0"
        fi
      done
      for secname in "${!category_contents[@]}" ; do
        echo "$secname"
      done | sort |
      while read -r secname ; do
        local pkgname
        for pkgname in ${category_contents["$secname"]} ; do
          echo "$pkgname"
        done | sort |
        while read -r pkgname ; do
          if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
            if [[ " ${sectionin[$pkgname]} " == *" $sectionN "* ]] ; then
              echo "  StrCpy \$sv_${secname}_${sanitized_name[$pkgname]} 1"
            else
              echo "  StrCpy \$sv_${secname}_${sanitized_name[$pkgname]} 0"
            fi
          fi
        done
      done
      for pkgname in "${allinstalledpackages[@]}" ; do
        if [[ " ${seed[$pkgname]} " == *" $sectionN "* ]] ; then
          echo "  SectionSetFlags \${sec_PKG_${sanitized_name[$pkgname]}} 9"
        elif [[ " ${sectionin[$pkgname]} " == *" $sectionN "* ]] ; then
          echo "  SectionSetFlags \${sec_PKG_${sanitized_name[$pkgname]}} 1"
        else
          echo "  SectionSetFlags \${sec_PKG_${sanitized_name[$pkgname]}} 0"
        fi
      done
      for secname in "${!category_contents[@]}" ; do
        echo "$secname"
      done | sort |
      while read -r secname ; do
        local pkgname
        for pkgname in ${category_contents["$secname"]} ; do
          echo "$pkgname"
        done | sort |
        while read -r pkgname ; do
          if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
            if [[ " ${seed[$pkgname]} " == *" $sectionN "* ]] ; then
              echo "  SectionSetFlags \${sec_${secname}_${sanitized_name[$pkgname]}} 9"
            elif [[ " ${sectionin[$pkgname]} " == *" $sectionN "* ]] ; then
              echo "  SectionSetFlags \${sec_${secname}_${sanitized_name[$pkgname]}} 1"
            else
              echo "  SectionSetFlags \${sec_${secname}_${sanitized_name[$pkgname]}} 0"
            fi
          fi
        done
      done
      echo "  SetCurInstType $((sectionN-1))"
      echo "No_$insttype:"
      ((sectionN++))
    done
  fi
  echo "  Pop \$R0"
  echo "FunctionEnd"
}

generate_onselchange_function(){
  if ((verbose)) ; then echo "Declaring .onSelChange function..." >&2 ; fi
  cat <<END
Function .onSelChange
  IntCmp \$SVV_SelChangeInProgress 0 +1 L1_SelChangeInProgress L1_SelChangeInProgress
  StrCpy \$SVV_SelChangeInProgress 1
  Push \$R0
  Push \$R1
END
  if [[ "$defpackage" != "NONE" ]] ; then
    cat <<END
  Push \$R2
  GetCurInstType \$R0
  StrCpy \$R2 0
  IntCmp \$R0 \$SVV_InstType +4 +1 +1
  StrCpy \$SVV_InstType \$R0
  IntCmp \$R0 \${NSIS_MAX_INST_TYPES} +2 +1 +1
  StrCpy \$R2 1
END
  fi
  # sv_XXX/PKV_XXX variables contain information about previous state of affairs
  # Note: there are three states - 0 (not to be installed), 9 (to be installed)
  # and 1 (will be installed because some other package needs it).
  # First we check is some packages were deselected and remove all packages
  # which can not be installed now.
  local secname pkgname subpkgname
  for secname in "${!category_contents[@]}" ; do
    echo "$secname"
  done | sort |
  while read -r secname ; do
    local pkgname
    for pkgname in ${category_contents["$secname"]} ; do
      echo "$pkgname"
    done | sort |
    while read -r pkgname ; do
      if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
        if ((verbose)) ; then echo "Read state: \"$pkgname\"..." >&2 ; fi
        local PKV_name="PKV_${sanitized_name[$pkgname]}"
        local sc_name="${secname}_${sanitized_name[$pkgname]}"
        echo "  SectionGetFlags \${sec_$sc_name} \$R0"
        echo "  IntOp \$R1 \$R0 & \${SF_SELECTED}"
        if [[ "$defpackage" == "NONE" ]] ; then
          echo "  IntCmp \$sv_$sc_name \$R1 L4_$sc_name"
        else
          echo "  IntCmp \$sv_$sc_name \$R1 L2_$sc_name"
        fi
        echo "  IntCmp \$sv_$sc_name 0 L1_$sc_name"
        echo "  StrCpy \$$PKV_name 0"
        for subpkgname in ${required["$pkgname"]} ; do
          if [[ "${allinstpackages[$subpkgname]}" = "present" ]] ; then
            echo "  IntCmp \$PKV_${sanitized_name[$subpkgname]} 9 +2 +1 +2"
            echo "  StrCpy \$PKV_${sanitized_name[$subpkgname]} 0"
          fi
        done
        echo "  Goto L4_$sc_name"
        echo "L1_$sc_name:"
        if [[ "$defpackage" != "NONE" ]] ; then
          echo "  IntCmp \$R2 0 L3_$sc_name +1 +1"
          echo "L2_$sc_name:"
          for subpkgname in ${seed["$pkgname"]} ; do
            echo "  IntCmp \$SVV_InstType $((subpkgname-1)) L3_$sc_name +1 +1"
          done
          echo "  GoTo L4_$sc_name"
          echo "L3_$sc_name:"
        fi
        echo "  StrCpy \$$PKV_name 9"
        echo "L4_$sc_name:"
      fi
    done
  done
  # Now we mark all packages which can be installed or not installed as
  # not-installed
  for pkgname in "${allinstalledpackages[@]}" ; do
    echo "  IntCmp \$PKV_${sanitized_name[$pkgname]} 9 +2 +1 +2"
    echo "  StrCpy \$PKV_${sanitized_name[$pkgname]} 0"
  done
  # Put the dependences back
  for pkgname in "${allinstalledpackages[@]}" ; do
    if ((verbose)) ; then echo "Set state: \"$pkgname\"..." >&2 ; fi
    echo "  IntCmp \$PKV_${sanitized_name[$pkgname]} 9 +1 L5_${sanitized_name[$pkgname]} +1"
    for subpkgname in ${requires["$pkgname"]} ; do
      echo "  IntCmp \$PKV_${sanitized_name[$subpkgname]} 9 +2 +1 +2"
      echo "  StrCpy \$PKV_${sanitized_name[$subpkgname]} 1"
    done
    echo "L5_${sanitized_name[$pkgname]}:"
  done
  # Copy the calculated new state to the screen
  for pkgname in "${allinstalledpackages[@]}" ; do
    echo "  SectionSetFlags \${sec_PKG_${sanitized_name[$pkgname]}} \$PKV_${sanitized_name[$pkgname]}"
  done
  for secname in "${!category_contents[@]}" ; do
    echo "$secname"
  done | sort |
  while read -r secname ; do
    local pkgname
    for pkgname in ${category_contents["$secname"]} ; do
      echo "$pkgname"
    done | sort |
    while read -r pkgname ; do
      if [[ "${allinstpackages[$pkgname]}" = "present" ]] ; then
        echo "  IntOp \$sv_${secname}_${sanitized_name[$pkgname]} \$PKV_${sanitized_name[$pkgname]} & 1"
        echo "  SectionSetFlags \${sec_${secname}_${sanitized_name[$pkgname]}} \$PKV_${sanitized_name[$pkgname]}"
      fi
    done
  done
  if [[ "$defpackage" != "NONE" ]] ; then
    echo "  Pop \$R2"
  fi
  cat <<END
  Pop \$R1
  Pop \$R0
  StrCpy \$SVV_SelChangeInProgress 0
L1_SelChangeInProgress:
FunctionEnd
END
}
