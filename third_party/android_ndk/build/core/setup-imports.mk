# Copyright (C) 2009-2010 The Android Open Source Project
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

# ====================================================================
#
# Check the import path
#
# ====================================================================

NDK_MODULE_PATH := $(strip $(NDK_MODULE_PATH))
ifdef NDK_MODULE_PATH
  ifneq ($(words $(NDK_MODULE_PATH)),1)
    $(call __ndk_info,ERROR: You NDK_MODULE_PATH variable contains spaces)
    $(call __ndk_info,Please fix the error and start again.)
    $(call __ndk_error,Aborting)
  endif
endif

$(call import-init)
$(foreach __path,$(subst $(HOST_DIRSEP),$(space),$(NDK_MODULE_PATH)),\
  $(call import-add-path,$(__path))\
)
$(call import-add-path-optional,$(NDK_ROOT)/sources)
$(call import-add-path-optional,$(NDK_ROOT)/../development/ndk/sources)
