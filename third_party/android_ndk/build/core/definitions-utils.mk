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

# Common utility functions.
#
# NOTE: All the functions here should be purely functional, i.e. avoid
#       using global variables or depend on the file system / environment
#       variables. This makes testing easier.

# -----------------------------------------------------------------------------
# Macro    : empty
# Returns  : an empty macro
# Usage    : $(empty)
# -----------------------------------------------------------------------------
empty :=

# -----------------------------------------------------------------------------
# Macro    : space
# Returns  : a single space
# Usage    : $(space)
# -----------------------------------------------------------------------------
space  := $(empty) $(empty)

space4 := $(space)$(space)$(space)$(space)

# -----------------------------------------------------------------------------
# Macro    : comma
# Returns  : a single comma
# Usage    : $(comma)
# -----------------------------------------------------------------------------
comma := ,

# -----------------------------------------------------------------------------
# Macro    : colon
# Returns  : a single colon
# Usage    : $(colon)
# -----------------------------------------------------------------------------
colon := :

define newline


endef

# -----------------------------------------------------------------------------
# Function : remove-duplicates
# Arguments: a list
# Returns  : the list with duplicate items removed, order is preserved.
# Usage    : $(call remove-duplicates, <LIST>)
# Note     : This is equivalent to the 'uniq' function provided by GMSL,
#            however this implementation is non-recursive and *much*
#            faster. It will also not explode the stack with a lot of
#            items like 'uniq' does.
# -----------------------------------------------------------------------------
remove-duplicates = $(strip \
  $(eval __uniq_ret :=) \
  $(foreach __uniq_item,$1,\
    $(if $(findstring $(__uniq_item),$(__uniq_ret)),,\
      $(eval __uniq_ret += $(__uniq_item))\
    )\
  )\
  $(__uniq_ret))

-test-remove-duplicates = \
  $(call test-expect,,$(call remove-duplicates))\
  $(call test-expect,foo bar,$(call remove-duplicates,foo bar))\
  $(call test-expect,foo bar,$(call remove-duplicates,foo bar foo bar))\
  $(call test-expect,foo bar,$(call remove-duplicates,foo foo bar bar bar))

# -----------------------------------------------------------------------------
# Function : clear-vars
# Arguments: 1: list of variable names
#            2: file where the variable should be defined
# Returns  : None
# Usage    : $(call clear-vars, VAR1 VAR2 VAR3...)
# Rationale: Clears/undefines all variables in argument list
# -----------------------------------------------------------------------------
clear-vars = $(foreach __varname,$1,$(eval $(__varname) := $(empty)))

# -----------------------------------------------------------------------------
# Function : filter-by
# Arguments: 1: list
#            2: predicate function, will be called as $(call $2,<name>)
#               and it this returns a non-empty value, then <name>
#               will be appended to the result.
# Returns  : elements of $1 that satisfy the predicate function $2
# -----------------------------------------------------------------------------
filter-by = $(strip \
  $(foreach __filter_by_n,$1,\
    $(if $(call $2,$(__filter_by_n)),$(__filter_by_n))))

-test-filter-by = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call filter-by,,-local-func))\
    $(call test-expect,foo,$(call filter-by,foo,-local-func))\
    $(call test-expect,foo,$(call filter-by,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call filter-by,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call filter-by,,-local-func))\
    $(call test-expect,,$(call filter-by,foo,-local-func))\
    $(call test-expect,bar,$(call filter-by,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call filter-by,aaa foo bar,-local-func))

# -----------------------------------------------------------------------------
# Function : filter-out-by
# Arguments: 1: list
#            2: predicate function, will be called as $(call $2,<name>)
#               and it this returns an empty value, then <name>
#               will be appended to the result.
# Returns  : elements of $1 that do not satisfy the predicate function $2
# -----------------------------------------------------------------------------
filter-out-by = $(strip \
  $(foreach __filter_out_by_n,$1,\
    $(if $(call $2,$(__filter_out_by_n)),,$(__filter_out_by_n))))

-test-filter-out-by = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call filter-out-by,,-local-func))\
    $(call test-expect,,$(call filter-out-by,foo,-local-func))\
    $(call test-expect,bar,$(call filter-out-by,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call filter-out-by,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call filter-out-by,,-local-func))\
    $(call test-expect,foo,$(call filter-out-by,foo,-local-func))\
    $(call test-expect,foo,$(call filter-out-by,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call filter-out-by,aaa foo bar foo,-local-func))

# -----------------------------------------------------------------------------
# Function : find-first
# Arguments: 1: list
#            2: predicate function, will be called as $(call $2,<name>).
# Returns  : the first item of $1 that satisfies the predicate.
# -----------------------------------------------------------------------------
find-first = $(firstword $(call filter-by,$1,$2))

-test-find-first.empty = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call find-first,,-local-pred))\
    $(call test-expect,,$(call find-first,bar,-local-pred))

-test-find-first.simple = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,foo,$(call find-first,foo,-local-pred))\
    $(call test-expect,foo,$(call find-first,aaa foo bar,-local-pred))\
    $(call test-expect,foo,$(call find-first,aaa foo foo bar,-local-pred))

# -----------------------------------------------------------------------------
# Function : parent-dir
# Arguments: 1: path
# Returns  : Parent dir or path of $1, with final separator removed.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
# On Windows, defining parent-dir is a bit more tricky because the
# GNU Make $(dir ...) function doesn't return an empty string when it
# reaches the top of the directory tree, and we want to enforce this to
# avoid infinite loops.
#
#   $(dir C:)     -> C:       (empty expected)
#   $(dir C:/)    -> C:/      (empty expected)
#   $(dir C:\)    -> C:\      (empty expected)
#   $(dir C:/foo) -> C:/      (correct)
#   $(dir C:\foo) -> C:\      (correct)
#
parent-dir = $(patsubst %/,%,$(strip \
    $(eval __dir_node := $(patsubst %/,%,$(subst \,/,$1)))\
    $(eval __dir_parent := $(dir $(__dir_node)))\
    $(filter-out $1,$(__dir_parent))\
    ))
else
parent-dir = $(patsubst %/,%,$(dir $(1:%/=%)))
endif

-test-parent-dir = \
  $(call test-expect,,$(call parent-dir))\
  $(call test-expect,.,$(call parent-dir,foo))\
  $(call test-expect,foo,$(call parent-dir,foo/bar))\
  $(call test-expect,foo,$(call parent-dir,foo/bar/))

# -----------------------------------------------------------------------------
# Strip any 'lib' prefix in front of a given string.
#
# Function : strip-lib-prefix
# Arguments: 1: module name
# Returns  : module name, without any 'lib' prefix if any
# Usage    : $(call strip-lib-prefix,$(LOCAL_MODULE))
# -----------------------------------------------------------------------------
strip-lib-prefix = $(1:lib%=%)

-test-strip-lib-prefix = \
  $(call test-expect,,$(call strip-lib-prefix,))\
  $(call test-expect,foo,$(call strip-lib-prefix,foo))\
  $(call test-expect,foo,$(call strip-lib-prefix,libfoo))\
  $(call test-expect,nolibfoo,$(call strip-lib-prefix,nolibfoo))\
  $(call test-expect,foolib,$(call strip-lib-prefix,foolib))\
  $(call test-expect,foo bar,$(call strip-lib-prefix,libfoo libbar))

# -----------------------------------------------------------------------------
# Left-justify input string with spaces to fill a width of 15.
# Function: left-justify-quoted-15
# Arguments: 1: Input text
# Returns: A quoted string that can be used in command scripts to print
#          the left-justified input with host-echo.
#
# Usage:  ---->@$(call host-echo, $(call left-justify-quoted-15,$(_TEXT)): Do stuff)
#         Where ----> is a TAB character.
# -----------------------------------------------------------------------------
left-justify-quoted-15 = $(call -left-justify,$1,xxxxxxxxxxxxxxx)

-test-left-justify-quoted-15 = \
  $(call test-expect,"               ",$(call left-justify-quoted-15,))\
  $(call test-expect,"Foo Bar        ",$(call left-justify-quoted-15,Foo Bar))\
  $(call test-expect,"Very long string over 15 characters wide",$(strip \
    $(call left-justify-quoted-15,Very long string over 15 characters wide)))

# Used internally to compute a quoted left-justified text string.
# $1: Input string.
# $2: A series of contiguous x's, its length determines the full width to justify to.
# Return: A quoted string with the input text left-justified appropriately.
-left-justify = $(strip \
    $(eval __lj_temp := $(subst $(space),x,$1))\
    $(foreach __lj_a,$(__gmsl_characters),$(eval __lj_temp := $$(subst $$(__lj_a),x,$(__lj_temp))))\
    $(eval __lj_margin := $$(call -justification-margin,$(__lj_temp),$2)))"$1$(subst x,$(space),$(__lj_margin))"

-test-left-justify = \
  $(call test-expect,"",$(call -left-justify,,))\
  $(call test-expect,"foo",$(call -left-justify,foo,xxx))\
  $(call test-expect,"foo ",$(call -left-justify,foo,xxxx))\
  $(call test-expect,"foo   ",$(call -left-justify,foo,xxxxxx))\
  $(call test-expect,"foo         ",$(call -left-justify,foo,xxxxxxxxxxxx))\
  $(call test-expect,"very long string",$(call -left-justify,very long string,xxx))\

# Used internally to compute a justification margin.
# Expects $1 to be defined to a string of consecutive x's (e.g. 'xxxx')
# Expects $2 to be defined to a maximum string of x's (e.g. 'xxxxxxxxx')
# Returns a string of x's such as $1 + $(result) is equal to $2
# If $1 is larger than $2, return empty string..
-justification-margin = $(strip \
    $(if $2,\
      $(if $1,\
        $(call -justification-margin-inner,$1,$2),\
        $2\
      ),\
    $1))

-justification-margin-inner = $(if $(findstring $2,$1),,x$(call -justification-margin-inner,x$1,$2))

-test-justification-margin = \
  $(call test-expect,,$(call -justification-margin,,))\
  $(call test-expect,,$(call -justification-margin,xxx,xxx))\
  $(call test-expect,xxxxxx,$(call -justification-margin,,xxxxxx))\
  $(call test-expect,xxxxx,$(call -justification-margin,x,xxxxxx))\
  $(call test-expect,xxxx,$(call -justification-margin,xx,xxxxxx))\
  $(call test-expect,xxx,$(call -justification-margin,xxx,xxxxxx))\
  $(call test-expect,xx,$(call -justification-margin,xxxx,xxxxxx))\
  $(call test-expect,x,$(call -justification-margin,xxxxx,xxxxxx))\
  $(call test-expect,,$(call -justification-margin,xxxxxx,xxxxxx))\
  $(call test-expect,,$(call -justification-margin,xxxxxxxxxxx,xxxxxx))\

# Escapes \ to \\. Useful for escaping Windows paths.
escape-backslashes = $(subst \,\\,$1)
