#!/bin/bash -ex
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export USER=user
useradd -mU -u $HOST_UID $USER

sudo -u $USER bash -c "Xvfb :1 -screen 0 1280x1024x24 -ac -nolisten tcp >/dev/null 2>&1 &"
sudo -u $USER DISPLAY=:1 HOME=/home/$USER "$@"
