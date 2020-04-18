# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# These definitions contain a few host-specific functions. I.e. they are
# typically used to generate shell commands during the build and their
# implementation will depend on the value of the HOST_OS variable.
#

# -----------------------------------------------------------------------------
# Function : host-path
# Arguments: 1: file path
# Returns  : file path, as understood by the host file system
# Usage    : $(call host-path,<path>)
# Rationale: This function is used to translate Cygwin paths into
#            Cygwin-specific ones. On other platforms, it will just
#            return its argument.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),cygwin)
host-path = $(if $(strip $1),$(call cygwin-to-host-path,$1))
else
host-path = $1
endif

# -----------------------------------------------------------------------------
# Function : host-rm
# Arguments: 1: list of files
# Usage    : $(call host-rm,<files>)
# Rationale: This function expands to the host-specific shell command used
#            to remove some files.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-rm = \
    $(eval __host_rm_files := $(foreach __host_rm_file,$1,$(subst /,\,$(wildcard $(__host_rm_file)))))\
    $(if $(__host_rm_files),del /f/q $(__host_rm_files) >NUL 2>NUL)
else
host-rm = rm -f $1
endif

# -----------------------------------------------------------------------------
# Function : host-rmdir
# Arguments: 1: list of files or directories
# Usage    : $(call host-rm,<files>)
# Rationale: This function expands to the host-specific shell command used
#            to remove some files _and_ directories.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-rmdir = \
    $(eval __host_rmdir_files := $(foreach __host_rmdir_file,$1,$(subst /,\,$(wildcard $(__host_rmdir_file)))))\
    $(if $(__host_rmdir_files),del /f/s/q $(__host_rmdir_files) >NUL 2>NUL)
else
host-rmdir = rm -rf $1
endif

# -----------------------------------------------------------------------------
# Function : host-mkdir
# Arguments: 1: directory path
# Usage    : $(call host-mkdir,<path>
# Rationale: This function expands to the host-specific shell command used
#            to create a path if it doesn't exist.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-mkdir = md $(subst /,\,"$1") >NUL 2>NUL || rem
else
host-mkdir = mkdir -p $1
endif

# -----------------------------------------------------------------------------
# Function : host-cp
# Arguments: 1: source file
#            2: target file
# Usage    : $(call host-cp,<src-file>,<dst-file>)
# Rationale: This function expands to the host-specific shell command used
#            to copy a single file
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-cp = copy /b/y $(subst /,\,"$1" "$2") > NUL
else
host-cp = cp -f $1 $2
endif

# -----------------------------------------------------------------------------
# Function : host-mv
# Arguments: 1: source file
#            2: target file
# Usage    : $(call host-mv,<src-file>,<dst-file>)
# Rationale: This function expands to the host-specific shell command used
#            to move a single file
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-mv = move /y $(subst /,\,"$1" "$2") > NUL
else
host-mv = mv -f $1 $2
endif

# -----------------------------------------------------------------------------
# Function : host-install
# Arguments: 1: source file
#            2: target file
# Usage    : $(call host-install,<src-file>,<dst-file>)
# Rationale: This function expands to the host-specific shell command used
#            to install a file or directory, while preserving its timestamps
#            (if possible).
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-install = copy /b/y $(subst /,\,"$1" "$2") > NUL
else
host-install = install -p $1 $2
endif

# -----------------------------------------------------------------------------
# Function : host-echo-build-step
# Arguments: 1: ABI
#            2: Step description (e.g. 'Compile C++', or 'StaticLibrary')
# Usage    : ---->|$(call host-echo-build-step,Compile) ....other text...
# Rationale: This function expands to the host-specific shell command used
#            to print the prefix of a given build step / command.
# -----------------------------------------------------------------------------
host-echo-build-step = @ $(HOST_ECHO) [$1] $(call left-justify-quoted-15,$2):

# -----------------------------------------------------------------------------
# Function : host-c-includes
# Arguments: 1: list of file paths (e.g. "foo bar")
# Returns  : list of include compiler options (e.g. "-Ifoo -Ibar")
# Usage    : $(call host-c-includes,<paths>)
# Rationale: This function is used to translate Cygwin paths into
#            Cygwin-specific ones. On other platforms, it will just
#            return its argument.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),cygwin)
host-c-includes = $(patsubst %,-I%,$(call host-path,$1))
else
host-c-includes = $(1:%=-I%)
endif

# -----------------------------------------------------------------------------
# Function : host-copy-if-differ
# Arguments: 1: source file
#            2: destination file
# Usage    : $(call host-copy-if-differ,<src-file>,<dst-file>)
# Rationale: This function copy source file to destination file if contents are
#            different.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-copy-if-differ = $(HOST_CMP) -s $1 $2 > NUL || copy /b/y $(subst /,\,"$1" "$2") > NUL
else
host-copy-if-differ = $(HOST_CMP) -s $1 $2 > /dev/null 2>&1 || cp -f $1 $2
endif


# -----------------------------------------------------------------------------
# Function : host-path-is-absolute
# Arguments: 1: file path
# Usage    : $(call host-path-is-absolute,<path>)
# Rationale: This function returns a non-empty result if the input path is
#            absolute on the host filesystem.
# -----------------------------------------------------------------------------

# On Windows, we need to take care drive prefix in file paths, e.g.:
#    /foo       -> top-level 'foo' directory on current drive.
#    //bar/foo  -> top-level 'foo' on network share 'bar'
#    c:/foo     -> top-level 'foo' directory on C drive.
#    c:foo      -> 'foo' subdirectory on C drive's current directory.
#
# Treat all of them as absolute. Filtering the first two cases is easy
# by simply looking at the first character. The other ones are more
# complicated and the simplest way is still to try all alphabet letters
# directly. Anything else involves very complicated GNU Make parsing
# voodoo.
ndk-windows-drive-letters := a b c d e f g h i j k l m n o p q r s t u v w x y z \
                             A B C D E F G H I J K L M N O P Q R S T U V W X Y Z

ndk-windows-drive-patterns := $(foreach _drive,$(ndk-windows-drive-letters),$(_drive):%)

windows-path-is-absolute = $(if $(filter /% $(ndk-windows-drive-patterns),$(subst \,/,$1)),true)

ifeq ($(HOST_OS),windows)
host-path-is-absolute = $(call windows-path-is-absolute,$1)
else
host-path-is-absolute = $(if $(filter /%,$1),true)
endif

-test-host-path-is-absolute.relative-paths = \
  $(call test-expect,,$(call host-path-is-absolute,foo))\
  $(call test-expect,,$(call host-path-is-absolute,foo/bar))\
  $(call test-expect,,$(call host-path-is-absolute,.))\
  $(call test-expect,,$(call host-path-is-absolute,..))

-test-host-path-is-absolute.absolute-paths = \
  $(call test-expect,true,$(call host-path-is-absolute,/))\
  $(call test-expect,true,$(call host-path-is-absolute,/foo))\
  $(call test-expect,true,$(call host-path-is-absolute,/foo/bar))\
  $(call test-expect,true,$(call host-path-is-absolute,//foo))\
  $(call test-expect,true,$(call host-path-is-absolute,/.))

-test-host-path-is-asbolute.windows-relative-paths = \
  $(call test-expect,$(call windows-path-is-absolute,foo))\
  $(call test-expect,$(call windows-path-is-absolute,foo/bar))\
  $(call test-expect,$(call windows-path-is-absolute,.))\
  $(call test-expect,$(call windows-path-is-absolute,..))

-test-host-path-is-asbolute.windows-absolute-paths = \
  $(call test-expect,true,$(call windows-path-is-absolute,c:/))\
  $(call test-expect,true,$(call windows-path-is-absolute,x:))\
  $(call test-expect,true,$(call windows-path-is-absolute,K:foo))\
  $(call test-expect,true,$(call windows-path-is-absolute,C:\Foo\Bar))\
  $(call test-expect,true,$(call windows-path-is-absolute,\Foo))
