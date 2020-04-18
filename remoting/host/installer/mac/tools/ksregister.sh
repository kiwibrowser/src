#!/bin/sh

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NAME=org.chromium.chromoting
PLIST=/Library/LaunchAgents/$NAME.plist

KSADMIN=/Library/Google/GoogleSoftwareUpdate/GoogleSoftwareUpdate.bundle/Contents/MacOS/ksadmin
KSUPDATE=https://tools.google.com/service/update2
KSPID=com.google.chrome_remote_desktop
KSPVERSION=0.5

# Register a ticket with Keystone so we're updated.
$KSADMIN --register --productid $KSPID --version $KSPVERSION --xcpath $PLIST --url $KSUPDATE
