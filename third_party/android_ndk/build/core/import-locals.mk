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
# Handle local variable expor/import during the build
#

$(call assert-defined,LOCAL_MODULE)

# For LOCAL_CFLAGS, LOCAL_CONLYFLAGS, LOCAL_CPPFLAGS and LOCAL_C_INCLUDES, etc,
# we need to use the exported definitions of the closure of all modules
# we depend on.
#
# I.e. If module 'foo' depends on 'bar' which depends on 'zoo',
# then 'foo' will get the CFLAGS/CONLYFLAGS/CPPFLAGS/C_INCLUDES/... of both 'bar'
# and 'zoo'
#

all_depends := $(call module-get-all-dependencies-topo,$(LOCAL_MODULE))
all_depends := $(filter-out $(LOCAL_MODULE),$(all_depends))

imported_CFLAGS     := $(call module-get-listed-export,$(all_depends),CFLAGS)
imported_CONLYFLAGS := $(call module-get-listed-export,$(all_depends),CONLYFLAGS)
imported_CPPFLAGS   := $(call module-get-listed-export,$(all_depends),CPPFLAGS)
imported_RENDERSCRIPT_FLAGS := $(call module-get-listed-export,$(all_depends),RENDERSCRIPT_FLAGS)
imported_ASMFLAGS   := $(call module-get-listed-export,$(all_depends),ASMFLAGS)
imported_C_INCLUDES := $(call module-get-listed-export,$(all_depends),C_INCLUDES)
imported_LDFLAGS    := $(call module-get-listed-export,$(all_depends),LDFLAGS)
imported_SHARED_LIBRARIES := $(call module-get-listed-export,$(all_depends),SHARED_LIBRARIES)
imported_STATIC_LIBRARIES := $(call module-get-listed-export,$(all_depends),STATIC_LIBRARIES)

ifdef NDK_DEBUG_IMPORTS
    $(info Imports for module $(LOCAL_MODULE):)
    $(info   CFLAGS='$(imported_CFLAGS)')
    $(info   CONLYFLAGS='$(imported_CONLYFLAGS)')
    $(info   CPPFLAGS='$(imported_CPPFLAGS)')
    $(info   RENDERSCRIPT_FLAGS='$(imported_RENDERSCRIPT_FLAGS)')
    $(info   ASMFLAGS='$(imported_ASMFLAGS)')
    $(info   C_INCLUDES='$(imported_C_INCLUDES)')
    $(info   LDFLAGS='$(imported_LDFLAGS)')
    $(info   SHARED_LIBRARIES='$(imported_SHARED_LIBRARIES)')
    $(info   STATIC_LIBRARIES='$(imported_STATIC_LIBRARIES)')
    $(info All depends='$(all_depends)')
endif

#
# The imported compiler flags are prepended to their LOCAL_XXXX value
# (this allows the module to override them).
#
LOCAL_CFLAGS     := $(strip $(imported_CFLAGS) $(LOCAL_CFLAGS))
LOCAL_CONLYFLAGS := $(strip $(imported_CONLYFLAGS) $(LOCAL_CONLYFLAGS))
LOCAL_CPPFLAGS   := $(strip $(imported_CPPFLAGS) $(LOCAL_CPPFLAGS))
LOCAL_RENDERSCRIPT_FLAGS := $(strip $(imported_RENDERSCRIPT_FLAGS) $(LOCAL_RENDERSCRIPT_FLAGS))
LOCAL_ASMFLAGS := $(strip $(imported_ASMFLAGS) $(LOCAL_ASMFLAGS))
LOCAL_LDFLAGS    := $(strip $(imported_LDFLAGS) $(LOCAL_LDFLAGS))

__ndk_modules.$(LOCAL_MODULE).STATIC_LIBRARIES += \
    $(strip $(call strip-lib-prefix,$(imported_STATIC_LIBRARIES)))
__ndk_modules.$(LOCAL_MODULE).SHARED_LIBRARIES += \
    $(strip $(call strip-lib-prefix,$(imported_SHARED_LIBRARIES)))
$(call module-add-static-depends,$(LOCAL_MODULE),$(imported_STATIC_LIBRARIES))
$(call module-add-shared-depends,$(LOCAL_MODULE),$(imported_SHARED_LIBRARIES))

#
# The imported include directories are appended to their LOCAL_XXX value
# (this allows the module to override them)
#
LOCAL_C_INCLUDES := $(strip $(LOCAL_C_INCLUDES) $(imported_C_INCLUDES))

# Similarly, you want the imported flags to appear _after_ the LOCAL_LDLIBS
# due to the way Unix linkers work (depending libraries must appear before
# dependees on final link command).
#
imported_LDLIBS := $(call module-get-listed-export,$(all_depends),LDLIBS)

LOCAL_LDLIBS := $(strip $(LOCAL_LDLIBS) $(imported_LDLIBS))

# We're done here
