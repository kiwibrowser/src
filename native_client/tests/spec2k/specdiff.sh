#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

dir=$(dirname $0)
perl -I ${dir}/bin/ ${dir}/bin/specdiff "$@"
