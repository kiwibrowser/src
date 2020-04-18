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

# Check that we have a Cygwin-compatible make.
#
# For some reason, a lot of application developers on Windows
# have another GNU Make installed in their path, that fails
# miserably with our build scripts. If we can detect this use
# case, early, we will be able to dump a human-readable error
# message with some help to fix the issue.
#

.PHONY: all
all:

# Get the cygwin-specific path to the make executable
# (e.g. /cygdrive/c/cygwin/usr/bin/make), then strip the
# .exe suffix, if any.
#
CYGWIN_MAKE := $(shell cygpath --unix --absolute $(firstword $(MAKE)))
CYGWIN_MAKE := $(CYGWIN_MAKE:%.exe=%)

# Now try to find it on the file system, a non-cygwin compatible
# GNU Make, even if launched from a Cygwin shell, will not
#
SELF_MAKE := $(strip $(wildcard $(CYGWIN_MAKE).exe))
ifeq ($(SELF_MAKE),)
    $(error Android NDK: $(firstword $(MAKE)) is not cygwin-compatible)
endif

# that's all
