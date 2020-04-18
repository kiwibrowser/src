# Copyright (C) 2012 The Android Open Source Project
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
# Definitions for the Android NDK build system's internal unit tests.
#

#
# A function which names begin with -test- (e.g. -test-foo) is assumed
# to be an internal unit test. It will be run automatically by ndk-build
# if NDK_UNIT_TESTS is defined in your environment.
#
# Each test should call one of the following functions that will
# register a failure:
#
#   $(call test-expect,<expected-value>,<actual-value>)
#
#      This will check that <actual-value> is equal to <expected-value>.
#      If not, this will print an error message and increment the failure
#      counter.
#
#   $(call test-assert,<expected-value>,<actual-value>)
#
#      This is similar to test-expect, though it will abort the program
#      immediately after displaying an error message.
#
# Here's an example that checks that the 'filter' function works correctly:
#
#   -test-filter = \
#     $(call test-expect,foo,$(filter bar,foo bar))
#
#

-ndk-test-start = \
  $(eval _test_name := $1)\
  $(eval _test_list += $1)\
  $(eval _test_failed :=)\
  $(info [$1  RUN])

# End current unit test.
#
-ndk-test-end = \
  $(if $(_test_failed),\
    $(info [$(_test_name) FAIL])$(error Aborting)\
    $(eval _test_failures += $$(_test_name))\
  ,\
    $(info [$(_test_name)   OK])\
  )

# Define NDK_UNIT_TESTS to 2 to dump each test-expect/assert check.
#
ifeq (2,$(NDK_UNIT_TESTS))
-ndk-test-log = $(info .  $(_test_name): $1)
else
-ndk-test-log = $(empty)
endif

test-expect = \
  $(call -ndk-test-log,expect '$2' == '$1')\
  $(if $(call sne,$1,$2),\
    $(info ERROR <$(_test_name)>:$3)\
    $(info .  expected value:'$1')\
    $(info .  actual value:  '$2')\
    $(eval _test_failed := true)\
  )

test-assert = \
  $(call -ndk-test-log,assert '$2' == '$1')\
  $(if $(call sne,$1,$2),\
    $(info ASSERT <$(_test_name)>:$3)\
    $(info .  expected value:'$1')\
    $(info .  actual value:  '$2')\
    $(eval _test_failed := true)\
    $(error Aborting.)\
  )

# Run all the tests, i.e. all functions that are defined with a -test-
# prefix will be called now in succession.
ndk-run-all-tests = \
  $(info ================= STARTING NDK-BUILD UNIT TESTS =================)\
  $(eval _test_list :=)\
  $(eval _test_failures :=)\
  $(foreach _test,$(filter -test-%,$(.VARIABLES)),\
    $(call -ndk-test-start,$(_test))\
    $(call $(_test))\
    $(call -ndk-test-end)\
  )\
  $(eval _test_count := $$(words $$(_test_list)))\
  $(eval _test_fail_count := $$(words $$(_test_failures)))\
  $(if $(_test_failures),\
    $(info @@@@@@@@@@@ FAILED $(_test_fail_count) of $(_test_count) NDK-BUILD UNIT TESTS @@@@@@@)\
    $(foreach _test_name,$(_test_failures),\
      $(info .  $(_test_name)))\
  ,\
    $(info =================== PASSED $(_test_count) NDK-BUILD UNIT TESTS =================)\
  )
