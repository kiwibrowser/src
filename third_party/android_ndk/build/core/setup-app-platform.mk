#
# Copyright (C) 2017 The Android Open Source Project
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

# Included from add-application.mk to configure the APP_PLATFORM setting.

ifeq (null,$(APP_PROJECT_PATH))

ifndef APP_PLATFORM
    $(call __ndk_info,APP_PLATFORM not set. Defaulting to minimum supported \
        version $(NDK_MIN_PLATFORM).)
    APP_PLATFORM := $(NDK_MIN_PLATFORM)
endif

else

# Set APP_PLATFORM with the following precedence:
#
# 1. APP_PLATFORM setting from the user.
# 2. Value in project.properties.
# 3. Minimum supported platform level.
APP_PLATFORM := $(strip $(APP_PLATFORM))
ifndef APP_PLATFORM
    _local_props := $(strip $(wildcard $(APP_PROJECT_PATH)/project.properties))
    ifndef _local_props
        # NOTE: project.properties was called default.properties before
        _local_props := $(strip \
            $(wildcard $(APP_PROJECT_PATH)/default.properties))
    endif

    ifdef _local_props
        APP_PLATFORM := $(strip \
            $(shell $(HOST_PYTHON) $(BUILD_PY)/extract_platform.py \
                $(call host-path,$(_local_props))))
        $(call __ndk_info,Found platform level in $(_local_props). Setting \
            APP_PLATFORM to $(APP_PLATFORM).)
    else
        $(call __ndk_info,APP_PLATFORM not set. Defaulting to minimum \
            supported version $(NDK_MIN_PLATFORM).)
        APP_PLATFORM := $(NDK_MIN_PLATFORM)
    endif
endif

ifeq ($(APP_PLATFORM),latest)
    $(call __ndk_info,Using latest available APP_PLATFORM: $(NDK_MAX_PLATFORM).)
    override APP_PLATFORM := $(NDK_MAX_PLATFORM)
endif

# Handle any platform codenames.
ifeq ($(APP_PLATFORM),android-I)
    override APP_PLATFORM := android-14
else ifeq ($(APP_PLATFORM),android-J)
    override APP_PLATFORM := android-16
else ifeq ($(APP_PLATFORM),android-J-MR1)
    override APP_PLATFORM := android-17
else ifeq ($(APP_PLATFORM),android-J-MR2)
    override APP_PLATFORM := android-18
else ifeq ($(APP_PLATFORM),android-K)
    override APP_PLATFORM := android-19
else ifeq ($(APP_PLATFORM),android-L)
    override APP_PLATFORM := android-21
else ifeq ($(APP_PLATFORM),android-L-MR1)
    override APP_PLATFORM := android-22
else ifeq ($(APP_PLATFORM),android-M)
    override APP_PLATFORM := android-23
else ifeq ($(APP_PLATFORM),android-N)
    override APP_PLATFORM := android-24
else ifeq ($(APP_PLATFORM),android-N-MR1)
    override APP_PLATFORM := android-25
else ifeq ($(APP_PLATFORM),android-O)
    override APP_PLATFORM := android-26
endif

endif # APP_PROJECT_PATH == null

# Though the platform emits library directories for every API level, the
# deprecated headers have gaps, and we only end up shipping libraries that match
# a directory for which we actually have deprecated headers. We'll need to fix
# this soon since we'll be adding O APIs to only the new headers, but for now
# just preserve the old behavior.
APP_PLATFORM_LEVEL := $(strip $(subst android-,,$(APP_PLATFORM)))
ifneq (,$(filter 20,$(APP_PLATFORM_LEVEL)))
    override APP_PLATFORM := android-19
else ifneq (,$(filter 25,$(APP_PLATFORM_LEVEL)))
    override APP_PLATFORM := android-24
endif

# For APP_PLATFORM values set below the minimum supported version, we could
# either pull up or error out. Since it's very unlikely that someone actually
# *needs* a lower platform level to build their app, we'll emit a warning and
# pull up.
APP_PLATFORM_LEVEL := $(strip $(subst android-,,$(APP_PLATFORM)))
ifneq ($(call lt,$(APP_PLATFORM_LEVEL),$(NDK_MIN_PLATFORM_LEVEL)),)
    $(call __ndk_info,$(APP_PLATFORM) is unsupported. Using minimum supported \
        version $(NDK_MIN_PLATFORM).)
    override APP_PLATFORM := $(NDK_MIN_PLATFORM)
endif

# There are two reasons (aside from user error) a user might have picked an API
# level that is higher than what we support:
#
# 1. To get a pseudo "latest" version that will raise with each new version.
# 2. Their using an old NDK that doesn't support the required API level.
#
# For 1, we now support `APP_PLATFORM := latest`, and users should just switch
# to that.
#
# For 2, clamping to the maximum API level for the older NDK when a newer API
# level is in fact needed to compile the project will just present the user with
# a lot of unclear errors. We'll save them time by failing early.
ifneq ($(call gt,$(APP_PLATFORM_LEVEL),$(NDK_MAX_PLATFORM_LEVEL)),)
    $(call __ndk_info,$(APP_PLATFORM) is above the maximum supported version \
        $(NDK_MAX_PLATFORM). Choose a supported API level or set APP_PLATFORM \
        to "latest".)
    $(call __ndk_error,Aborting.)
endif

# We pull low values up, fill in gaps, replace platform code names, replace
# "latest", and error out on high values. Anything left is either a gap or
# codename we missed, or user error.
ifneq (,$(strip $(filter-out $(NDK_ALL_PLATFORMS),$(APP_PLATFORM))))
    $(call __ndk_info,APP_PLATFORM set to unknown platform: $(APP_PLATFORM).)
    $(call __ndk_error,Aborting)
endif

ifneq (null,$(APP_PROJECT_PATH))

# Check platform level (after adjustment) against android:minSdkVersion in AndroidManifest.xml
#
APP_MANIFEST := $(strip $(wildcard $(APP_PROJECT_PATH)/AndroidManifest.xml))
APP_PLATFORM_LEVEL := $(strip $(subst android-,,$(APP_PLATFORM)))
ifdef APP_MANIFEST
    _minsdkversion := $(strip \
        $(shell $(HOST_PYTHON) $(BUILD_PY)/extract_manifest.py \
            minSdkVersion $(call host-path,$(APP_MANIFEST))))
    ifndef _minsdkversion
        # minSdkVersion defaults to 1.
        # https://developer.android.com/guide/topics/manifest/uses-sdk-element.html
        _minsdkversion := 1
    endif

    ifneq (,$(call gt,$(APP_PLATFORM_LEVEL),$(_minsdkversion)))
        $(call __ndk_info,WARNING: APP_PLATFORM $(APP_PLATFORM) is higher than \
            android:minSdkVersion $(_minsdkversion) in $(APP_MANIFEST). NDK \
            binaries will *not* be comptible with devices older than \
            $(APP_PLATFORM). See \
            https://android.googlesource.com/platform/ndk/+/master/docs/user/common_problems.md \
            for more information.)
    endif
endif

endif # APP_PROJECT_PATH == null
