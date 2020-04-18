# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Sends a single message on opening containing the headers received from the
# browser. The header keys have been converted to lower-case, while the values
# retain the original case.

import json

from mod_pywebsocket import msgutil


def web_socket_do_extra_handshake(request):
    pass


def web_socket_transfer_data(request):
    msgutil.send_message(request, json.dumps(dict(request.headers_in.items())))
