#!/usr/bin/env python3
#
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Tobin Ehlis <tobin@lunarg.com>

from inspect import currentframe, getframeinfo

# This is a wrapper class for inspect module that returns a formatted line
#  with details of the source file and line number of python code who called
#  into this class. The result can them be added to codegen to simplify
#  debug as it shows where code was generated from.
class sourcelineinfo():
    def __init__(self):
        self.general_prefix = "// CODEGEN : "
        self.file_prefix = "file "
        self.line_prefix = "line #"
        self.enabled = True

    def get(self):
        if self.enabled:
            frameinfo = getframeinfo(currentframe().f_back)
            return "%s%s%s %s%s" % (self.general_prefix, self.file_prefix, frameinfo.filename, self.line_prefix, frameinfo.lineno)
        return ""
