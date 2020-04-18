# Copyright (C) 2013 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)/googletest

# Common definitions

googletest_includes := $(LOCAL_PATH)/include

googletest_sources := \
  src/gtest-death-test.cc \
  src/gtest-filepath.cc \
  src/gtest-port.cc \
  src/gtest-printers.cc \
  src/gtest-test-part.cc \
  src/gtest-typed-test.cc \
  src/gtest.cc

googletest_main_sources := \
  src/gtest_main.cc

# GoogleTest library modules.

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_static
LOCAL_SRC_FILES := $(googletest_sources)
LOCAL_C_INCLUDES := $(googletest_includes)
LOCAL_EXPORT_C_INCLUDES := $(googletest_includes)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_shared
LOCAL_SRC_FILES := $(googletest_sources)
LOCAL_C_INCLUDES := $(googletest_includes)
LOCAL_CFLAGS := -DGTEST_CREATE_SHARED_LIBRARY
LOCAL_EXPORT_C_INCLUDES := $(googletest_includes)
include $(BUILD_SHARED_LIBRARY)

# GoogleTest 'main' helper modules.

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_main
LOCAL_SRC_FILES := $(googletest_main_sources)
LOCAL_STATIC_LIBRARIES := googletest_static
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_main_shared
LOCAL_SRC_FILES := $(googletest_main_sources)
LOCAL_SHARED_LIBRARIES := googletest_shared
include $(BUILD_STATIC_LIBRARY)

# The GoogleTest test programs.
#

# The unit test suite itself.
# This excludes tests that require a Python wrapper to run properly.
# Note that this is _very_ slow to compile :)
include $(CLEAR_VARS)
LOCAL_MODULE := googletest_unittest

# IMPORTANT: The test looks at the name of its executable and expects
# the following. Otherwise OutputFileHelpersTest.GetCurrentExecutableName
# will FAIL.
#
LOCAL_MODULE_FILENAME := gtest_all_test
LOCAL_SRC_FILES := \
    test/gtest-filepath_test.cc \
    test/gtest-linked_ptr_test.cc \
    test/gtest-message_test.cc \
    test/gtest-options_test.cc \
    test/gtest-port_test.cc \
    test/gtest_pred_impl_unittest.cc \
    test/gtest_prod_test.cc \
    test/gtest-test-part_test.cc \
    test/gtest-typed-test_test.cc \
    test/gtest-typed-test2_test.cc \
    test/gtest_unittest.cc \
    test/production.cc
LOCAL_STATIC_LIBRARIES := googletest_main
include $(BUILD_EXECUTABLE)

# The GoogleTest samples.
# These are much quicker to build and run.

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_1
LOCAL_SRC_FILES := \
    samples/sample1.cc \
    samples/sample1_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_2
LOCAL_SRC_FILES := \
    samples/sample2.cc \
    samples/sample2_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_3
LOCAL_SRC_FILES := \
    samples/sample3_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_4
LOCAL_SRC_FILES := \
    samples/sample4.cc \
    samples/sample4_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_5
LOCAL_SRC_FILES := \
    samples/sample1.cc \
    samples/sample5_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_6
LOCAL_SRC_FILES := \
    samples/sample6_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_7
LOCAL_SRC_FILES := \
    samples/sample7_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_8
LOCAL_SRC_FILES := \
    samples/sample8_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := googletest_sample_9
LOCAL_SRC_FILES := \
    samples/sample9_unittest.cc
LOCAL_STATIC_LIBRARIES := googletest_main_shared
include $(BUILD_EXECUTABLE)
