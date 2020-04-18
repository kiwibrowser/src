#!/bin/bash
# Copyright (c) 2017 Google Inc.

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
# Script to determine if source code in Pull Request is properly formatted.
# Exits with non 0 exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

FILES_TO_CHECK=$(git diff --name-only master | grep -v -E "^include/vulkan" | grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp)$")

if [ -z "${FILES_TO_CHECK}" ]; then
  echo -e "${GREEN}No source code to check for formatting.${NC}"
  exit 0
fi

FORMAT_DIFF=$(git diff -U0 master -- ${FILES_TO_CHECK} | python ./scripts/clang-format-diff.py -p1 -style=file)

if [ -z "${FORMAT_DIFF}" ]; then
  echo -e "${GREEN}All source code in PR properly formatted.${NC}"
  exit 0
else
  echo -e "${RED}Found formatting errors!${NC}"
  echo "${FORMAT_DIFF}"
  exit 1
fi
