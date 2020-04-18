#!/bin/bash
# Copyright (c) 2018 Valve Corporation
# Copyright (c) 2018 LunarG, Inc.

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
# Checks commit messages against project standards in CONTRIBUTING.md document
# Script to determine if commit messages in Pull Request are properly formatted.
# Exits with non 0 exit code if reformatting is needed.

# Disable subshells
shopt -s lastpipe

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# TRAVIS_COMMIT_RANGE contains range of commits for this PR

# Get user-supplied commit message text for applicable commits and insert
# a unique separator string identifier. The git command returns ONLY the
# subject line and body for each of the commits.
COMMIT_TEXT=$(git log ${TRAVIS_COMMIT_RANGE} --pretty=format:"XXXNEWLINEXXX"%n%B)

# Bail if there are none
if [ -z "${COMMIT_TEXT}" ]; then
  echo -e "${GREEN}No commit messgages to check for formatting.${NC}"
  exit 0
elif ! echo $TRAVIS_COMMIT_RANGE | grep -q "\.\.\."; then
  echo -e "${GREEN}No commit messgages to check for formatting.${NC}"
  exit 0
fi

# Process commit messages
success=1
current_line=0
prevline=""

# Process each line of the commit message output, resetting counter on separator
printf %s "$COMMIT_TEXT" | while IFS='' read -r line; do
  # echo "Count = $current_line <Line> = $line"
  current_line=$((current_line+1))
  if [ "$line" = "XXXNEWLINEXXX" ]; then
    current_line=0
  fi
  chars=${#line}
  if [ $current_line -eq 1 ]; then
    # Subject line should be 50 chars or less (but give some slack here)
    if [ $chars -gt 54 ]; then
      echo "The following subject line exceeds 50 characters in length."
      echo "     '$line'"
      success=0
    fi
    i=$(($chars-1))
    last_char=${line:$i:1}
    # Output error if last char of subject line is not alpha-numeric
    if [[ ! $last_char =~ [0-9a-zA-Z] ]]; then
      echo "For the following commit, the last character of the subject line must not be non-alphanumeric."
      echo "     '$line'"
      success=0
    fi
    # Checking if subject line doesn't start with 'module: '
    prefix=$(echo $line | cut -f1 -d " ")
    if [ "${prefix: -1}" != ":" ]; then
      echo "The following subject line must start with a single word specifying the functional area of the change, followed by a colon and space. I.e., 'layers: Subject line here'"
      echo "     '$line'"
      success=0
    fi
  elif [ $current_line -eq 2 ]; then
    # Commit message must have a blank line between subject and body
    if [ $chars -ne 0 ]; then
      echo "The following subject line must be followed by a blank line."
      echo "     '$prevline'"
      success=0
    fi
  else
    # Lines in a commit message body must be less than 72 characters in length (but give some slack)
    if [ $chars -gt 76 ]; then
      echo "The following commit message body line exceeds the 72 character limit."
      echo "'$line\'"
      success=0
    fi
  fi
  prevline=$line
done

if [ $success -eq 1 ]; then
  echo -e "${GREEN}All commit messages in pull request are properly formatted.${NC}"
  exit 0
else
  exit 1
fi
