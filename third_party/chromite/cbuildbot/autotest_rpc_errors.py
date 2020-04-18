# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Error codes used for the Autotest RPC Client, Proxy, and Server.

This is a copy of scripts/slave-internal/autotest_rpc/autotest_rpc_errors.py
from https://chrome-internal.googlesource.com/chrome/tools/build.
"""

PROXY_CANNOT_SEND_REQUEST = 11
PROXY_CONNECTION_LOST = 12
PROXY_TIMED_OUT = 13

SERVER_NO_COMMAND = 21
SERVER_NO_ARGUMENTS = 22
SERVER_UNKNOWN_COMMAND = 23
SERVER_BAD_ARGUMENT_COUNT = 24

CLIENT_CANNOT_CONNECT = 31
CLIENT_HTTP_CODE = 32
CLIENT_EMPTY_RESPONSE = 33
CLIENT_NO_RETURN_CODE = 34
