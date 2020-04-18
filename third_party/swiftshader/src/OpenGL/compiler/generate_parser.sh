#!/bin/bash
# Copyright 2016 The SwiftShader Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Generates GLSL ES parser - glslang_lex.cpp, glslang_tab.h, and glslang_tab.cpp

run_flex()
{
input_file=$script_dir/$1.l
output_source=$script_dir/$1_lex.cpp
flex --noline --nounistd --outfile=$output_source $input_file
}

run_bison()
{
input_file=$script_dir/$1.y
output_header=$script_dir/$1_tab.h
output_source=$script_dir/$1_tab.cpp
bison --no-lines --skeleton=yacc.c --defines=$output_header --output=$output_source $input_file
}

script_dir=$(dirname $0)

# Generate Parser
run_flex glslang
run_bison glslang
