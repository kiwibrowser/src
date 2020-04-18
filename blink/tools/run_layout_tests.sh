#!/bin/sh

# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

exec_dir=$(dirname $0)

if [ "$OSTYPE" = "cygwin" ]; then
  SCRIPT=$(cygpath -wa "$exec_dir/run_layout_tests.py")
else
  SCRIPT="$exec_dir/run_layout_tests.py"
fi

PYTHON_PROG=python
unset PYTHONPATH

"$PYTHON_PROG" "$SCRIPT" "$@"
