# Copyright (C) 2010 The Android Open Source Project
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

# This file is designed to be called from the 'ndk-build' script
# or similar wrapper tool.
#

# Detect the NDK installation path by processing this Makefile's location.
# This assumes we are located under $NDK_ROOT/build/core/main.mk
#

# Don't output to stdout if we're being invoked to dump a variable
DUMP_VAR := $(patsubst DUMP_%,%,$(filter DUMP_%,$(MAKECMDGOALS)))
ifneq (,$(DUMP_VAR))
    NDK_NO_INFO := 1
    NDK_NO_WARNINGS := 1
endif

NDK_ROOT := $(dir $(lastword $(MAKEFILE_LIST)))
NDK_ROOT := $(subst \,/,$(NDK_ROOT))
NDK_ROOT := $(strip $(NDK_ROOT:%build/core/=%))
NDK_ROOT := $(NDK_ROOT:%/=%)
ifeq ($(NDK_ROOT),)
    # for the case when we're invoked from the NDK install path
    NDK_ROOT := .
endif
ifeq ($(NDK_LOG),1)
    $(info Android NDK: NDK installation path auto-detected: '$(NDK_ROOT)')
endif
ifneq ($(words $(NDK_ROOT)),1)
    $(info Android NDK: Your NDK installation path contains spaces.)
    $(info Android NDK: Please re-install to a different location to fix the issue !)
    $(error Aborting.)
endif

include $(NDK_ROOT)/build/core/init.mk

# ====================================================================
#
# If NDK_PROJECT_PATH is not defined, find the application's project
# path by looking at the manifest file in the current directory or
# any of its parents. If none is found, try again with 'jni/Android.mk'
#
# Note that we first look at the current directory to avoid using
# absolute NDK_PROJECT_PATH values. This reduces the length of all
# source, object and binary paths that are passed to build commands.
#
# It turns out that some people use ndk-build to generate static
# libraries without a full Android project tree.
#
# If NDK_PROJECT_PATH=null, ndk-build make no attempt to look for it, but does
# need the following variables depending on NDK_PROJECT_PATH to be explicitly
# specified (from the default, if any):
#
#   NDK_OUT
#   NDK_LIBS_OUT
#   APP_BUILD_SCRIPT
#   NDK_DEBUG (optional, default to 0)
#   Other APP_* used to be in Application.mk
#
# This behavior may be useful in an integrated build system.
#
# ====================================================================

find-project-dir = $(strip $(call find-project-dir-inner,$(abspath $1),$2))

find-project-dir-inner = \
    $(eval __found_project_path := )\
    $(eval __find_project_path := $1)\
    $(eval __find_project_file := $2)\
    $(call find-project-dir-inner-2)\
    $(__found_project_path)

find-project-dir-inner-2 = \
    $(call ndk_log,Looking for $(__find_project_file) in $(__find_project_path))\
    $(eval __find_project_manifest := $(strip $(wildcard $(__find_project_path)/$(__find_project_file))))\
    $(if $(__find_project_manifest),\
        $(call ndk_log,    Found it !)\
        $(eval __found_project_path := $(__find_project_path))\
        ,\
        $(eval __find_project_parent := $(call parent-dir,$(__find_project_path)))\
        $(if $(__find_project_parent),\
            $(eval __find_project_path := $(__find_project_parent))\
            $(call find-project-dir-inner-2)\
        )\
    )

NDK_PROJECT_PATH := $(strip $(NDK_PROJECT_PATH))
APP_PROJECT_PATH := $(strip $(APP_PROJECT_PATH))

ifneq (,$(APP_PROJECT_PATH))
    ifeq (,$(NDK_PROJECT_PATH))
        # If NDK_PROJECT_PATH isn't set and APP_PROJECT_PATH is present, use APP_PROJECT_PATH
        $(call ndk_log,Use APP_PROJECT_PATH for NDK_PROJECT_PATH: $(APP_PROJECT_PATH))
        NDK_PROJECT_PATH := $(APP_PROJECT_PATH)
    else
        # If both NDK_PROJECT_PATH and APP_PROJECT_PATH are present, check consistency
        ifneq ($(NDK_PROJECT_PATH),$(APP_PROJECT_PATH))
            $(call __ndk_info,WARNING: NDK_PROJECT_PATH and APP_PROJECT_PATH are both set but not equal literally)
            $(call __ndk_info,  NDK_PROJECT_PATH = $(NDK_PROJECT_PATH))
            $(call __ndk_info,  APP_PROJECT_PATH = $(APP_PROJECT_PATH))
        endif
    endif
endif

ifeq (null,$(NDK_PROJECT_PATH))

$(call ndk_log,Make no attempt to look for NDK_PROJECT_PATH.)

else

# To keep paths as short as possible during the build, we first look if the
# current directory is the top of our project path. If this is the case, we
# will define NDK_PROJECT_PATH to simply '.'
#
# Otherwise, we will use find-project-dir which will first get the absolute
# path of the current directory the climb back the hierarchy until we find
# something. The result will always be a much longer definition for
# NDK_PROJECT_PATH
#
ifndef NDK_PROJECT_PATH
    ifneq (,$(strip $(wildcard AndroidManifest.xml)))
        NDK_PROJECT_PATH := .
    else
        ifneq (,$(strip $(wildcard jni/Android.mk)))
            NDK_PROJECT_PATH := .
        endif
    endif
endif
ifndef NDK_PROJECT_PATH
    NDK_PROJECT_PATH := $(call find-project-dir,.,jni/Android.mk)
endif
ifndef NDK_PROJECT_PATH
    NDK_PROJECT_PATH := $(call find-project-dir,.,AndroidManifest.xml)
endif
ifndef NDK_PROJECT_PATH
    $(call __ndk_info,Could not find application project directory !)
    $(call __ndk_info,Please define the NDK_PROJECT_PATH variable to point to it.)
    $(call __ndk_error,Aborting)
endif

# Check that there are no spaces in the project path, or bad things will happen
ifneq ($(words $(NDK_PROJECT_PATH)),1)
    $(call __ndk_info,Your Android application project path contains spaces: '$(NDK_PROJECT_PATH)')
    $(call __ndk_info,The Android NDK build cannot work here. Please move your project to a different location.)
    $(call __ndk_error,Aborting.)
endif

$(call ndk_log,Found project path: $(NDK_PROJECT_PATH))

NDK_APPLICATION_MK := $(strip $(wildcard $(NDK_PROJECT_PATH)/jni/Application.mk))

endif # NDK_PROJECT_PATH == null

ifndef NDK_APPLICATION_MK
    NDK_APPLICATION_MK := $(NDK_ROOT)/build/core/default-application.mk
endif


# Place all generated intermediate files here
NDK_APP_OUT := $(strip $(NDK_OUT))
ifndef NDK_APP_OUT
  ifeq (null,$(NDK_PROJECT_PATH))
    $(call __ndk_info,NDK_PROJECT_PATH==null.  Please explicitly set NDK_OUT to directory for all generated intermediate files.)
    $(call __ndk_error,Aborting.)
  endif
  NDK_APP_OUT := $(NDK_PROJECT_PATH)/obj
endif
$(call ndk_log,Ouput path for intermediate files: $(NDK_APP_OUT))

# Place all generated library files here.  This is rarely changed since aapt expects the default libs/
NDK_APP_LIBS_OUT := $(strip $(NDK_LIBS_OUT))
ifndef NDK_APP_LIBS_OUT
  ifeq (null,$(NDK_PROJECT_PATH))
    $(call __ndk_info,NDK_PROJECT_PATH==null.  Please explicitly set NDK_LIBS_OUT to directory for generated library files.)
    $(call __ndk_error,Aborting.)
  endif
  NDK_APP_LIBS_OUT := $(NDK_PROJECT_PATH)/libs
endif
$(call ndk_log,Ouput path for generated library files: $(NDK_APP_LIBS_OUT))

# Fake an application named 'local'
_app            := local
_application_mk := $(NDK_APPLICATION_MK)
NDK_APPS        := $(_app)

include $(BUILD_SYSTEM)/add-application.mk

# If a goal is DUMP_xxx then we dump a variable xxx instead
# of building anything
#
MAKECMDGOALS := $(filter-out DUMP_$(DUMP_VAR),$(MAKECMDGOALS))

include $(BUILD_SYSTEM)/setup-imports.mk

ifneq (,$(DUMP_VAR))

# We only support a single DUMP_XXX goal at a time for now.
ifneq ($(words $(DUMP_VAR)),1)
    $(call __ndk_error,!!TOO-MANY-DUMP-VARIABLES!!)
endif

$(foreach _app,$(NDK_APPS),\
  $(eval include $(BUILD_SYSTEM)/setup-app.mk)\
)

.PHONY : DUMP_$(DUMP_VAR)
DUMP_$(DUMP_VAR):
	@echo $($(DUMP_VAR))
else
    # Build it
    include $(BUILD_SYSTEM)/build-all.mk
endif
