#-------------------------------------------------------------------------
# drawElements CMake utilities
# ----------------------------
#
# Copyright 2016 The Android Open Source Project
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
#-------------------------------------------------------------------------

message("*** Using null context target")

set(DEQP_TARGET_NAME "Null")

set(TCUTIL_PLATFORM_SRCS
	null/tcuNullPlatform.cpp
	null/tcuNullPlatform.hpp
	null/tcuNullRenderContext.cpp
	null/tcuNullRenderContext.hpp
	null/tcuNullContextFactory.cpp
	null/tcuNullContextFactory.hpp
	)
