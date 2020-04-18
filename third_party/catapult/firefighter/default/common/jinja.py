# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import jinja2


_TEMPLATES_DIR = os.path.join(
    os.path.dirname(os.path.dirname(os.path.realpath(__file__))), 'templates')


ENVIRONMENT = jinja2.Environment(loader=jinja2.FileSystemLoader(_TEMPLATES_DIR))
