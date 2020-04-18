#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script assumed to be run in native_client/
if [[ "x${OSTYPE}" = "xcygwin" ]]; then
  cd "$(cygpath "${PWD}")"
fi
if [[ ${PWD} != */native_client ]]; then
  echo "ERROR: must be run in native_client!"
  exit 1
fi

if [[ $# -ne 3 ]]; then
  echo "USAGE: $0 version_file win/mac/linux glibc/newlib"
  exit 2
fi

if [[ "${0:0:1}" = "/" ]]; then
  declare -r scriptname="$0"
else
  declare -r scriptname="$PWD/$0"
fi

cd tools/BACKPORTS

if [[ "$((sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") 2>/dev/null)" = "$(cat "$1.lastver")" ]]; then
  # Everything is already done
  exit 0
fi

set -x
set -e
set -u

declare -r GIT_BASE_URL=https://chromium.googlesource.com/native_client

rm -f "$$.error"

# Checkout toolchain sources from git repo.  We'll need them later when we'll
# patch sources toolchain before calling build script.
for dirname in binutils gcc gdb glibc linux-headers-for-nacl newlib ; do
  repo="${dirname}"
  if [[ "$repo" != "linux-headers-for-nacl" ]]; then
    repo="nacl-${repo}"
  fi
  if [[ -d "$dirname" ]]; then (
    cd "$dirname"
    git pull --all ||
    (cd .. &&
     rm -rf "$dirname")
  ) fi
  if [[ ! -d "$dirname" ]]; then (
    git clone "${GIT_BASE_URL}/${repo}.git" "$dirname"
  ) fi || touch $$.error &
done

wait

# If we were unable to checkout some sources then it's time to stop.
if [[ -e "$$.error" ]]; then
  rm -f "$$.error"
  # Errors are reported by git above already
  exit 1
fi

# Here we'll checkout correct versions of all the sources for all supported
# ppapi versions.
while read name id comment ; do
  case "$name" in
    binutils | gcc | glibc | newlib | '' | \#*)
      # Skip this line
      ;;
    *)
      if [[ "$((sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") 2>/dev/null)" = "$(cat "$1.$name.lastver")" ]]; then
	# Everything is already done
	continue
      fi
      # First step is to use glient to sync sources.
      if [[ -d "$name" ]]; then
	cd "$name"
	if [[ "$2" = "win" ]]; then
	  # "gclient.bat revert" automatically calls "runhooks"… ⇒ it fails…
	  # "gclient runhooks" downloads toolchain then calls gyp… ⇒ it fails…
	  # "glient.bat runhooks" sees toolchain and simply calls gyp ⇒ success!
	  # Additional fun: error codes are lost somewhere in gclient.bat…
	  ( gclient.bat revert || true
	    gclient runhooks --force || true
	    gclient.bat runhooks --force || true
	  ) < /dev/null
	else
	  gclient revert < /dev/null
	fi
      else
	mkdir "$name"
	cd "$name"
	if [[ "$name" == ppapi17 ]]; then
	  cat >.gclient <<-END
	solutions = [
	  { "name"        : "native_client",
	    "url"         : "http://src.chromium.org/native_client/trunk/src/native_client@$id",
	    "deps_file"   : "DEPS",
	    "managed"     : True,
	    "custom_deps" : {
	      "third_party/jsoncpp/source/include": "http://svn.code.sf.net/p/jsoncpp/code/trunk/jsoncpp/include@246",
	      "third_party/jsoncpp/source/src/lib_json": "http://svn.code.sf.net/p/jsoncpp/code/trunk/jsoncpp/src/lib_json@246",
	    },
	    "safesync_url": "",
	  },
	]
	END
	elif [[ "$name" == ppapi1[89] ]] || [[ "$name" == ppapi2[0-6] ]]; then
	  cat >.gclient <<-END
	solutions = [
	  { "name"        : "native_client",
	    "url"         : "http://src.chromium.org/native_client/trunk/src/native_client@$id",
	    "deps_file"   : "DEPS",
	    "managed"     : True,
	    "custom_deps" : {
	      "third_party/jsoncpp/source/include": "http://svn.code.sf.net/p/jsoncpp/code/trunk/jsoncpp/include@248",
	      "third_party/jsoncpp/source/src/lib_json": "http://svn.code.sf.net/p/jsoncpp/code/trunk/jsoncpp/src/lib_json@248",
	    },
	    "safesync_url": "",
	  },
	]
	END
	else
	  cat >.gclient <<-END
	solutions = [
	  { "name"        : "native_client",
	    "url"         : "http://src.chromium.org/native_client/trunk/src/native_client@$id",
	    "deps_file"   : "DEPS",
	    "managed"     : True,
	    "custom_deps" : {
	    },
	    "safesync_url": "",
	  },
	]
	END
	fi
	if [[ "$2" = "win" ]]; then
	  # "gclient.bat revert" automatically calls "runhooks"… ⇒ it fails…
	  # "gclient runhooks" downloads toolchain then calls gyp… ⇒ it fails…
	  # "glient.bat runhooks" sees toolchain and simply calls gyp ⇒ success!
	  # Additional fun: error codes are lost somewhere in gclient.bat…
	  ( gclient.bat sync || true
	    gclient runhooks --force || true
	    gclient.bat runhooks --force || true
	  ) < /dev/null
	else
	  gclient sync < /dev/null
	fi
      fi
      # Now we need to change versions to officialy mark binaries.  We don't
      # show git revision because we combine different branches here.
      patch -p0 <<-END
	--- native_client/buildbot/buildbot_standard.py
	+++ native_client/buildbot/buildbot_standard.py
	@@ -148 +148,2 @@
	-  with Step('cleanup_temp', status):
	+  if False:
	+   with Step('cleanup_temp', status):
	--- native_client/tools/glibc_download.sh
	+++ native_client/tools/glibc_download.sh
	@@ -42 +42 @@
	-  for ((j=glibc_revision+1;j<glibc_revision+revisions_count;j++)); do
	+  for ((j=\${glibc_revision%~*}+1;j<\${glibc_revision%~*}+revisions_count;j++)); do
	--- native_client/tools/glibc_revision.sh
	+++ native_client/tools/glibc_revision.sh
	@@ -13 +13 @@
	-for i in REVISIONS glibc_revision.sh Makefile ; do
	+for i in ../../../VERSIONS ../../../build_backports.sh REVISIONS glibc_revision.sh Makefile ; do
	@@ -20 +20 @@
	-echo "\$REVISION"
	+echo "\$REVISION~$name"
	END
      if [[ "$name" = ppapi1? ]] || [[ "$name" = ppapi2[0-6] ]]; then
	patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -202,2 +202,4 @@
	-  CFLAGS="-m\$(HOST_TOOLCHAIN_BITS) \$(CFLAGS)" \\
	-  CXXFLAGS="-m\$(HOST_TOOLCHAIN_BITS) \$(CXXFLAGS)" \\
	+  CC="\$(GCC_CC)" \\
	+  CXX="\$(GCC_CXX)" \\
	+  CFLAGS="\$(CFLAGS)" \\
	+  CXXFLAGS="\$(CXXFLAGS)" \\
	@@ -363 +365,7 @@
	+ifeq (\$(PLATFORM), mac)
	+GCC_CC = clang -m\$(HOST_TOOLCHAIN_BITS) -fgnu89-inline
	+GCC_CXX = clang++ -m\$(HOST_TOOLCHAIN_BITS)
	+else
	 GCC_CC = gcc -m\$(HOST_TOOLCHAIN_BITS)
	+GCC_CXX = g++ -m\$(HOST_TOOLCHAIN_BITS)
	+endif
	END
      fi
      if [[ "$name" = ppapi1? ]] || [[ "$name" = ppapi2[0-7] ]]; then
	patch -p0 <<-END
	--- native_client/tools/glibc_download.sh
	+++ native_client/tools/glibc_download.sh
	@@ -20 +20 @@
	-declare -r glibc_url_prefix=http://gsdview.appspot.com/nativeclient-archive2/between_builders/x86_glibc/r
	+declare -r glibc_url_prefix=http://storage.googleapis.com/nativeclient-archive2/between_builders/x86_glibc/r
	END
      fi
      declare rev="$(native_client/tools/glibc_revision.sh)"
      if [[ "$name" = ppapi14 ]]; then
	patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -237 +237 @@
	-	sed -e s'|cloog_LDADD = \$(LDADD)|cloog_LDADD = \$(LDADD) -lstdc++ -lm |' \\
	+	sed -e s'|LIBS = @LIBS@|LIBS = @LIBS@ -lstdc++ -lm |' \\
	@@ -315,2 +315,2 @@
	-	  CC="gcc" \\
	-	  CFLAGS="-m\$(HOST_TOOLCHAIN_BITS) -O2 \$(CFLAGS)" \\
	+	  CC="\$(GCC_CC)" \\
	+	  CFLAGS="\$(CFLAGS)" \\
	@@ -847 +847 @@
	-	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1-\\2-\\3+'\` (Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/binutils ; LC_ALL=C git rev-parse HEAD\`)\\"\\n" |\\
	+	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`cd ../../.. ; LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1-\\2-\\3+'\` (Native Client r$rev)\\"\\n" |\\
	@@ -849,2 +849,3 @@
	-	LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1-\\2-\\3+' > SRC/gcc/gcc/DATESTAMP
	-	echo "Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/gcc ; LC_ALL=C git rev-parse HEAD\`" > SRC/gcc/gcc/DEV-PHASE
	+	( cd ../../.. ; LC_ALL=C svn info ) | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1-\\2-\\3+' > SRC/gcc/gcc/DATESTAMP
	+	echo "Native Client r$rev" > SRC/gcc/gcc/DEV-PHASE
	+	cp -aiv SRC/gdb/gdb/version.in SRC/gdb/gdb/version.inT
	@@ -854 +855 @@
	-	+\`cat SRC/gdb/gdb/version.in\` \`LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1-\\2-\\3+'\` (Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/gdb ; LC_ALL=C git rev-parse HEAD\`)\\n" |\\
	+	+\`cat SRC/gdb/gdb/version.in\` \`cd ../../.. ; LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1-\\2-\\3+'\` (Native Client r$rev)\\n" |\\
	END
      elif [[ "$name" = ppapi15 ]]; then
	patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -237 +237 @@
	-	sed -e s'|cloog_LDADD = \$(LDADD)|cloog_LDADD = \$(LDADD) -lstdc++ -lm |' \\
	+	sed -e s'|LIBS = @LIBS@|LIBS = @LIBS@ -lstdc++ -lm |' \\
	@@ -315,2 +315,2 @@
	-	  CC="gcc" \\
	-	  CFLAGS="-m\$(HOST_TOOLCHAIN_BITS) -O2 \$(CFLAGS)" \\
	+	  CC="\$(GCC_CC)" \\
	+	  CFLAGS="\$(CFLAGS)" \\
	@@ -847 +847 @@
	-	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/binutils ; LC_ALL=C git rev-parse HEAD\`)\\"\\n" |\\
	+	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`cd ../../.. ; LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r$rev)\\"\\n" |\\
	@@ -849,2 +849,3 @@
	-	LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+' > SRC/gcc/gcc/DATESTAMP
	-	echo "Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/gcc ; LC_ALL=C git rev-parse HEAD\`" > SRC/gcc/gcc/DEV-PHASE
	+	( cd ../../.. ; LC_ALL=C svn info ) | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+' > SRC/gcc/gcc/DATESTAMP
	+	echo "Native Client r$rev" > SRC/gcc/gcc/DEV-PHASE
	+	cp -aiv SRC/gdb/gdb/version.in SRC/gdb/gdb/version.inT
	@@ -854 +855 @@
	-	+\`cat SRC/gdb/gdb/version.in\` \`LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/gdb ; LC_ALL=C git rev-parse HEAD\`)\\n" |\\
	+	+\`cat SRC/gdb/gdb/version.in\` \`cd ../../.. LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r$rev)\\n" |\\
	END
      elif [[ "$name" = ppapi1? ]] || [[ "$name" = ppapi2[01] ]]; then
	patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -700,5 +700,5 @@
	-BINUTILS_PATCHNAME := naclbinutils-\$(BINUTILS_VERSION)-r\$(shell svnversion | tr : _)
	-GCC_PATCHNAME := naclgcc-\$(GCC_VERSION)-r\$(shell svnversion | tr : _)
	-#GDB_PATCHNAME := naclgdb-\$(GDB_VERSION)-r\$(shell svnversion | tr : _)
	-GLIBC_PATCHNAME := naclglibc-\$(GLIBC_VERSION)-r\$(shell svnversion | tr : _)
	-NEWLIB_PATCHNAME := naclnewlib-\$(NEWLIB_VERSION)-r\$(shell svnversion | tr : _)
	+BINUTILS_PATCHNAME := naclbinutils-\$(BINUTILS_VERSION)-r\$(shell ./glibc_revision.sh)
	+GCC_PATCHNAME := naclgcc-\$(GCC_VERSION)-r\$(shell ./glibc_revision.sh)
	+#GDB_PATCHNAME := naclgdb-\$(GDB_VERSION)-r\$(shell ./glibc_revision.sh)
	+GLIBC_PATCHNAME := naclglibc-\$(GLIBC_VERSION)-r\$(shell ./glibc_revision.sh)
	+NEWLIB_PATCHNAME := naclnewlib-\$(NEWLIB_VERSION)-r\$(shell ./glibc_revision.sh)
	@@ -847 +847 @@
	-	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/binutils ; LC_ALL=C git rev-parse HEAD\`)\\"\\n" |\\
	+	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`cd ../../.. ; LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r$rev)\\"\\n" |\\
	@@ -849,2 +849,3 @@
	-	LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+' > SRC/gcc/gcc/DATESTAMP
	-	echo "Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/gcc ; LC_ALL=C git rev-parse HEAD\`" > SRC/gcc/gcc/DEV-PHASE
	+	( cd ../../.. ; LC_ALL=C svn info ) | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+' > SRC/gcc/gcc/DATESTAMP
	+	echo "Native Client r$rev" > SRC/gcc/gcc/DEV-PHASE
	+	cp -aiv SRC/gdb/gdb/version.in SRC/gdb/gdb/version.inT
	@@ -854 +855 @@
	-	+\`cat SRC/gdb/gdb/version.in\` \`LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r\`LC_ALL=C svnversion\`, Git Commit \`cd SRC/gdb ; LC_ALL=C git rev-parse HEAD\`)\\n" |\\
	+	+\`cat SRC/gdb/gdb/version.in\` \`cd ../../.. LC_ALL=C svn info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r$rev)\\n" |\\
	END
      else
	patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -700,5 +700,5 @@
	-BINUTILS_PATCHNAME := naclbinutils-\$(BINUTILS_VERSION)-r\$(shell \$(SVNVERSION) | tr : _)
	-GCC_PATCHNAME := naclgcc-\$(GCC_VERSION)-r\$(shell \$(SVNVERSION) | tr : _)
	-#GDB_PATCHNAME := naclgdb-\$(GDB_VERSION)-r\$(shell \$(SVNVERSION) | tr : _)
	-GLIBC_PATCHNAME := naclglibc-\$(GLIBC_VERSION)-r\$(shell \$(SVNVERSION) | tr : _)
	-NEWLIB_PATCHNAME := naclnewlib-\$(NEWLIB_VERSION)-r\$(shell \$(SVNVERSION) | tr : _)
	+BINUTILS_PATCHNAME := naclbinutils-\$(BINUTILS_VERSION)-r\$(shell ./glibc_revision.sh)
	+GCC_PATCHNAME := naclgcc-\$(GCC_VERSION)-r\$(shell ./glibc_revision.sh)
	+#GDB_PATCHNAME := naclgdb-\$(GDB_VERSION)-r\$(shell ./glibc_revision.sh)
	+GLIBC_PATCHNAME := naclglibc-\$(GLIBC_VERSION)-r\$(shell ./glibc_revision.sh)
	+NEWLIB_PATCHNAME := naclnewlib-\$(NEWLIB_VERSION)-r\$(shell ./glibc_revision.sh)
	@@ -847 +847 @@
	-	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`LC_ALL=C \$(SVN) info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r\`LC_ALL=C \$(SVNVERSION)\`, Git Commit \`cd SRC/binutils ; LC_ALL=C git rev-parse HEAD\`)\\"\\n" |\\
	+	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \\" \`cd ../../.. ; LC_ALL=C \$(SVN) info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r$rev)\\"\\n" |\\
	@@ -849,2 +849,3 @@
	-	LC_ALL=C \$(SVN) info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+' > SRC/gcc/gcc/DATESTAMP
	-	echo "Native Client r\`LC_ALL=C \$(SVNVERSION)\`, Git Commit \`cd SRC/gcc ; LC_ALL=C git rev-parse HEAD\`" > SRC/gcc/gcc/DEV-PHASE
	+	( cd ../../.. ; LC_ALL=C \$(SVN) info ) | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+' > SRC/gcc/gcc/DATESTAMP
	+	echo "Native Client r$rev" > SRC/gcc/gcc/DEV-PHASE
	+	cp -aiv SRC/gdb/gdb/version.in SRC/gdb/gdb/version.inT
	@@ -854 +855 @@
	-	+\`cat SRC/gdb/gdb/version.in\` \`LC_ALL=C \$(SVN) info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r\`LC_ALL=C \$(SVNVERSION)\`, Git Commit \`cd SRC/gdb ; LC_ALL=C git rev-parse HEAD\`)\\n" |\\
	+	+\`cat SRC/gdb/gdb/version.in\` \`cd ../../.. LC_ALL=C \$(SVN) info | grep 'Last Changed Date' | sed -e s'+Last Changed Date: \\(....\\)-\\(..\\)-\\(..\\).*+\\1\\2\\3+'\` (Native Client r$rev)\\n" |\\
	END
      fi
      # The buildbot has become strict in enforcing tag names.
      # We used to emit STEP_SUCCESS, but it never got added to the annotator.
      # Dropping the tag in old versions.
      if [[ "$name" = ppapi1[0-9] ]] || [[ "$name" = ppapi2[0-9] ]] || \
         [[ "$name" = ppapi30 ]]; then
          patch -p0 <<-END
	--- native_client/buildbot/buildbot_lib.py
	+++ native_client/buildbot/buildbot_lib.py
	@@ -378,7 +378,6 @@ class Step(object):
	         raise StopBuild()
	     else:
	       self.status.ReportPass(self.name)
	-      print '@@@STEP_SUCCESS@@@'

	     # Suppress any exception that occurred.
	     return True
	END
      fi
      if [[ "$name" = ppapi19 ]] || [[ "$name" = ppapi20 ]]; then
	  patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -154,2 +154,2 @@
	-NEWLIB_VERSION = 1.18.0
	-NACL_NEWLIB_GIT_BASE = 65e6baefeb2874011001c2f843cf3083e771b62f
	+NEWLIB_VERSION = 1.20.0
	+NACL_NEWLIB_GIT_BASE = 151b2c72fb87849bbc6e3ef569718c6344eed2e6
	END
      fi
      patch -p0 <<-END
	--- native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	+++ native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	@@ -27 +27 @@
	-rm -rf scons-out tools/SRC/* tools/BUILD/* tools/out tools/toolchain \\
	+rm -rf scons-out tools/BUILD/* tools/out tools/toolchain \\
	--- native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	+++ native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	@@ -90 +90 @@
	-    make glibc-check
	+    true make glibc-check
	--- native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	+++ native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	@@ -109 +109 @@
	-  rev="\$(tools/glibc_revision.sh)"
	+  rev="$rev"
	--- native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	+++ native_client/buildbot/buildbot_lucid64-glibc-makefile.sh
	@@ -164 +164 @@
	-    make glibc-check
	+    true make glibc-check
	--- native_client/buildbot/buildbot_mac-glibc-makefile.sh
	+++ native_client/buildbot/buildbot_mac-glibc-makefile.sh
	@@ -28 +28 @@
	-rm -rf scons-out tools/SRC/* tools/BUILD/* tools/out/* tools/toolchain \\
	+rm -rf scons-out tools/BUILD/* tools/out/* tools/toolchain \\
	--- native_client/buildbot/buildbot_windows-glibc-makefile.sh
	+++ native_client/buildbot/buildbot_windows-glibc-makefile.sh
	@@ -40 +40 @@
	-rm -rf scons-out tools/SRC/* tools/BUILD/* tools/out tools/toolchain \\
	+rm -rf scons-out tools/BUILD/* tools/out tools/toolchain \\
	--- native_client/buildbot/gsutil.sh
	+++ native_client/buildbot/gsutil.sh
	@@ -29 +29 @@
	-  gsutil="\${SCRIPT_DIR_ABS}/../../../../../scripts/slave/gsutil.bat"
	+  gsutil="\${SCRIPT_DIR_ABS}/../../../../../../../../../scripts/slave/gsutil.bat"
	END
      mv native_client/buildbot/buildbot_windows-glibc-makefile.bat \
	native_client/buildbot/buildbot_windows-glibc-makefile.bat.orig
      sed -e s'/  ..\\..\\..\\..\\scripts\\slave\\gsutil/  ..\\..\\..\\..\\..\\..\\..\\..\\scripts\\slave\\gsutil/' \
	< native_client/buildbot/buildbot_windows-glibc-makefile.bat.orig \
	> native_client/buildbot/buildbot_windows-glibc-makefile.bat
      rm native_client/buildbot/buildbot_windows-glibc-makefile.bat.orig
      if [[ "$name" = ppapi14 ]]; then
	patch -p0 <<-END
	--- native_client/buildbot/buildbot_toolchain.sh
	+++ native_client/buildbot/buildbot_toolchain.sh
	@@ -36 +36 @@
	-rm -rf ../scons-out sdk-out sdk ../toolchain SRC/* BUILD/*
	+rm -rf ../scons-out sdk-out sdk ../toolchain BUILD/*
	@@ -61 +61 @@
	-      \${GS_BASE}/latest/naclsdk_\${PLATFORM}_x86.tgz
	+      \${GS_BASE}/latest~"$name"/naclsdk_\${PLATFORM}_x86.tgz
	END
      elif [[ "$name" = ppapi1[5-7] ]]; then
	patch -p0 <<-END
	--- native_client/buildbot/buildbot_toolchain.sh
	+++ native_client/buildbot/buildbot_toolchain.sh
	@@ -36 +36 @@
	-rm -rf ../scons-out sdk-out sdk ../toolchain SRC/* BUILD/*
	+rm -rf ../scons-out sdk-out sdk ../toolchain BUILD/*
	@@ -69 +69 @@
	-    for destrevision in \${BUILDBOT_GOT_REVISION} latest ; do
	+    for destrevision in \${BUILDBOT_GOT_REVISION} latest~"$name" ; do
	@@ -73 +73 @@
	-          \${GS_BASE}/\${destrevision}/naclsdk_\${PLATFORM}_x86.\${suffix}
	+          \${GS_BASE}/"\${destrevision}"/naclsdk_\${PLATFORM}_x86.\${suffix}
	END
      else
	patch -p0 <<-END
	--- native_client/buildbot/buildbot_toolchain.sh
	+++ native_client/buildbot/buildbot_toolchain.sh
	@@ -36 +36 @@
	-rm -rf ../scons-out sdk-out sdk ../toolchain/*_newlib SRC/* BUILD/*
	+rm -rf ../scons-out sdk-out sdk ../toolchain/*_newlib BUILD/*
	@@ -69 +69 @@
	-    for destrevision in \${BUILDBOT_GOT_REVISION} latest ; do
	+    for destrevision in \${BUILDBOT_GOT_REVISION} latest~"$name" ; do
	@@ -73 +73 @@
	-          \${GS_BASE}/\${destrevision}/naclsdk_\${PLATFORM}_x86.\${suffix}
	+          \${GS_BASE}/"\${destrevision}"/naclsdk_\${PLATFORM}_x86.\${suffix}
	END
      fi
      # Patch sources and build the toolchains.
      if [[ "$name" != "ppapi14" ]] || [[ "$3" != glibc ]]; then
	make -C native_client/tools clean
	rm -rf native_client/tools/SRC/*
	for i in binutils gcc gdb glibc linux-headers-for-nacl newlib ; do (
	  if [[ "$name" != "ppapi14" ]] || [[ "$i" != glibc ]]; then
	    rm -rf native_client/tools/SRC/"$i"
	    git clone ../"$i" native_client/tools/SRC/"$i"
	    cd native_client/tools/SRC/"$i"
	    . ../../REVISIONS
	    declare varname="NACL_$(echo "$i" | LC_ALL=C tr a-z A-Z)_COMMIT"
	    if [[ "$varname" = "NACL_LINUX-HEADERS-FOR-NACL_COMMIT" ]]; then
	      . ../../../../../../REVISIONS
	      git checkout "$LINUX_HEADERS_FOR_NACL_COMMIT"
	    else
	      git checkout "${!varname}"
	    fi
	    cd ../../../../..
	    ( while read n id comment && [[ "$n" != "$name" ]]; do
		: # Nothing
	      done
	      cd "$name/native_client/tools/SRC/$i"
	      while read tag id comment ; do
		if [[ "$i" = "$tag" ]]; then
		  if [[ "$name" = ppapi1[4-8] ]] && [[ "$i" = "newlib" ]] &&
		     [[ "$id" = "4353bc00936874bb78aa3ba21c648b4f4c3f946b" ]]; then
		    # Ignore error
		    git diff "$id"{^..,} | patch -p1 ||
		    ( rejfiles="$(find -name '*.rej')"
		      if [[ "$rejfiles" != "./newlib/libc/include/machine/setjmp.h.rej" ]]; then
			touch "../../../../../$$.error" "../../../../../$$.error.$name"
		      else
			rm ./newlib/libc/include/machine/setjmp.h.rej
		      fi
		    )
		  elif [[ "$name" = ppapi1[4-7] ]] &&
		       [[ "$id" = "f96a3cbfb8777e1e47471b357929b8a1e3340a23" ]]; then
		    patch -p0 <<-END
			--- gcc/config/i386/nacl.h
			+++ gcc/config/i386/nacl.h
			@@ -269,3 +269,6 @@
			 #define DWARF2_ADDR_SIZE \\
			     (TARGET_NACL ? (TARGET_64BIT ? 8 : 4) : \\
			                    (POINTER_SIZE / BITS_PER_UNIT))
			+
			+/* Profile counters are not available under Native Client. */
			+#define NO_PROFILE_COUNTERS 1
			END
		  elif [[ "$name" = ppapi1[5-9] || "$name" == ppapi2[0-7] ]] &&
		       [[ "$id" = "2324fd9e11f551e367cbe714ff49a4df3309396e" ]]; then
		    for ldscript in elf{,64}_nacl.x{,.static,s}; do
		      patch -p0 <<-END
			--- nacl/dyn-link/ldscripts/$ldscript
			+++ nacl/dyn-link/ldscripts/$ldscript
			@@ -50 +50,6 @@
			-  .note.gnu.build-id : { *(.note.gnu.build-id) } :seg_rodata
			+  .note.gnu.build-id :
			+  {
			+    PROVIDE_HIDDEN (__note_gnu_build_id_start = .);
			+    *(.note.gnu.build-id)
			+    PROVIDE_HIDDEN (__note_gnu_build_id_end = .);
			+  } :seg_rodata
			END
		    done
		  elif [[ "$name" != ppapi1[5-8] ]] ||
		       [[ "$id" != "8ec02f0e5af28bd478ce262f04d156e4ef09c4d9" ]]; then
		    git diff "$id"{^..,} | patch -p1 ||
		      touch "../../../../../$$.error" "../../../../../$$.error.$name"
		  fi
		fi
	      done
	    ) < "$1"
	  fi
	) done
	if [[ "$name" == ppapi1[45] ]]; then
	  patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -747,6 +747,7 @@
	 	  CC="\$\${CC}" \\
	 	  LDFLAGS="-s" \\
	 	  ../../SRC/gdb/configure \\
	+	    --disable-werror \\
	 	    --prefix=\$(SDKROOT) \\
	 	    \$\${BUILD} \\
	 	    --target=nacl
	END
	elif [[ "$name" == ppapi1[6-9] ]]; then
	  patch -p0 <<-END
	--- native_client/tools/Makefile
	+++ native_client/tools/Makefile
	@@ -747,6 +747,7 @@
	 	  CC="\$\${CC}" \\
	 	  LDFLAGS="-s" \\
	 	  ../../SRC/gdb/configure \\
	+	    --disable-werror \\
	 	    --prefix=\$(PREFIX) \\
	 	    \$\${BUILD} \\
	 	    --target=nacl
	END
	else
	  patch -p0 <<-END
	--- native_client/tools/SRC/gdb/gdb/doc/Makefile.in
	+++ native_client/tools/SRC/gdb/gdb/doc/Makefile.in
	@@ -306 +306 @@
	-	echo "@set GDBVN \`sed q \$(srcdir)/../version.in\`" > ./GDBvn.new
	+	echo "@set GDBVN \`sed q \$(srcdir)/../version.inT\`" > ./GDBvn.new
	END
	fi
	declare url_prefix=http://storage.googleapis.com/nativeclient-archive2
	if [[ "$3" = "glibc" ]]; then
	  declare url=$url_prefix/x86_toolchain/r"$rev"/toolchain_"$2"_x86.tar.gz
	else
	  declare url=$url_prefix/toolchain/"$rev"/naclsdk_"$2"_x86.tgz
	fi
	if [[ ! -e "../$$.error.$name" ]]; then
	  # If toolchain is already available then another try will not change anything
	  curl --fail --location --url "$url" -o /dev/null &&
	  (cd .. ; sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") >"../$1.$name.lastver" 2>/dev/null &&
	  rm -rf "../$name" ||
	  (
	    cd native_client
	    export BUILD_COMPATIBLE_TOOLCHAINS=no
	    export BUILDBOT_GOT_REVISION="$rev"
	    if [[ "$2" = "win" ]]; then (
	      # Use extended globbing (cygwin should always have it).
	      shopt -s extglob
	      # Filter out cygwin python (everything under /usr or /bin, or *cygwin*).
	      export PATH=${PATH/#\/bin*([^:])/}
	      export PATH=${PATH//:\/bin*([^:])/}
	      export PATH=${PATH/#\/usr*([^:])/}
	      export PATH=${PATH//:\/usr*([^:])/}
	      export PATH=${PATH/#*([^:])cygwin*([^:])/}
	      export PATH=${PATH//:*([^:])cygwin*([^:])/}
	      python_slave buildbot/buildbot_selector.py
	      (cd ../.. ; sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") >"../../$1.$name.lastver" 2>/dev/null
	      rm -rf "../../$name"
	    ) else
	      # PPAPI14 to PPAPI30 were designed to be built on Ubuntu Lucid,
	      # not on Ubuntu Precise
	      if [[ "$name" == ppapi[12]? ]] || [[ "$name" == ppapi30 ]]; then
		BUILDBOT_BUILDERNAME="${BUILDBOT_BUILDERNAME/precise64/lucid64}"
	      fi
	      python buildbot/buildbot_selector.py
	      (cd ../.. ; sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") >"../../$1.$name.lastver" 2>/dev/null
	      rm -rf "../../$name"
	    fi
	  )
	  cd ..
	fi
      else
	(cd .. ; sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") >"../$1.$name.lastver" 2>/dev/null &&
	rm -rf "../$name"
	cd ..
      fi
    ;;
  esac
done < "$1"

(sha1sum "$scriptname" "$1" || shasum "$scriptname" "$1") > "$1.lastver" 2>/dev/null
