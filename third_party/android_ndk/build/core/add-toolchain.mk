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

# this script is included repeatedly by main.mk to add a new toolchain
# definition to the NDK build system.
#
# '_config_mk' must be defined as the path of a toolchain
# configuration file (config.mk) that will be included here.
#
$(call assert-defined, _config_mk)

# The list of variables that must or may be defined
# by the toolchain configuration file
#
NDK_TOOLCHAIN_VARS_REQUIRED := TOOLCHAIN_ABIS TOOLCHAIN_ARCH
NDK_TOOLCHAIN_VARS_OPTIONAL :=

# Clear variables that are supposed to be defined by the config file
$(call clear-vars,$(NDK_TOOLCHAIN_VARS_REQUIRED))
$(call clear-vars,$(NDK_TOOLCHAIN_VARS_OPTIONAL))

# Include the config file
include $(_config_mk)

ifeq ($(TOOLCHAIN_ABIS)$(TOOLCHAIN_ARCH),)
# Ignore if both TOOLCHAIN_ABIS and TOOLCHAIN_ARCH are not defined
else

# Check that the proper variables were defined
$(call check-required-vars,$(NDK_TOOLCHAIN_VARS_REQUIRED),$(_config_mk))

# Check that the file didn't do something stupid
$(call assert-defined, _config_mk)

# Now record the toolchain-specific information
_dir  := $(patsubst %/,%,$(dir $(_config_mk)))
_name := $(notdir $(_dir))
_arch := $(TOOLCHAIN_ARCH)
_abis := $(TOOLCHAIN_ABIS)

_toolchain := NDK_TOOLCHAIN.$(_name)

# check that the toolchain name is unique
$(if $(strip $($(_toolchain).defined)),\
  $(call __ndk_error,Toolchain $(_name) defined in $(_parent) is\
                     already defined in $(NDK_TOOLCHAIN.$(_name).defined)))

$(_toolchain).defined := $(_toolchain_config)
$(_toolchain).arch    := $(_arch)
$(_toolchain).abis    := $(_abis)
$(_toolchain).setup   := $(wildcard $(_dir)/setup.mk)

$(if $(strip $($(_toolchain).setup)),,\
  $(call __ndk_error, Toolchain $(_name) lacks a setup.mk in $(_dir)))

NDK_ALL_TOOLCHAINS += $(_name)
NDK_ALL_ARCHS      += $(_arch)
NDK_ALL_ABIS       += $(_abis)

# NDK_ABI.<abi>.toolchains records the list of toolchains that support
# a given ABI
#
$(foreach _abi,$(_abis),\
    $(eval NDK_ABI.$(_abi).toolchains += $(_name)) \
    $(eval NDK_ABI.$(_abi).arch := $(sort $(NDK_ABI.$(_abi).arch) $(_arch)))\
)

NDK_ARCH.$(_arch).toolchains += $(_name)
NDK_ARCH.$(_arch).abis := $(sort $(NDK_ARCH.$(_arch).abis) $(_abis))

endif

# done
