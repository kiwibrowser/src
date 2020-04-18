REM Add [include] of repo's .gitconfig in clone's .git/config.
REM This only needs to be run once.

REM
REM Copyright (c) 2016 The Khronos Group Inc.
REM
REM Permission is hereby granted, free of charge, to any person obtaining a
REM copy of this software and/or associated documentation files (the
REM "Materials"), to deal in the Materials without restriction, including
REM without limitation the rights to use, copy, modify, merge, publish,
REM distribute, sublicense, and/or sell copies of the Materials, and to
REM permit persons to whom the Materials are furnished to do so, subject to
REM the following conditions:
REM
REM The above copyright notice and this permission notice shall be included
REM in all copies or substantial portions of the Materials.
REM
REM THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
REM EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
REM MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
REM IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
REM CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
REM TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
REM MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
REM Set colors

git config --local include.path ..\.gitconfig
echo Git config was successfully set.
