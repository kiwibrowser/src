#!/bin/bash
#set -x
if [ -t 1 ] ; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    NC='\033[0m' # No Color
else
    RED=''
    GREEN=''
    NC=''
fi

printf "$GREEN[ RUN      ]$NC $0\n"

# Run doc validation from project root dir
pushd ../..

# Validate that layer documentation matches source contents
./vk_layer_documentation_generate.py --validate

RES=$?


if [ $RES -eq 0 ] ; then
   printf "$GREEN[  PASSED  ]$NC 1 test\n"
   exit 0
else
   printf "$RED[  FAILED  ]$NC Validation of vk_validation_layer_details.md failed\n"
   printf "$RED[  FAILED  ]$NC 1 test\n"
   printf "1 TEST FAILED\n"
   exit 1
fi
# Restore original directory
popd
