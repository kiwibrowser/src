# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from google.appengine.ext import vendor

from base import constants


for library in constants.THIRD_PARTY_LIBRARIES:
  vendor.add(os.path.join('third_party', library))
