# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The main url dispatcher for this project."""

from __future__ import print_function

from django import http
from django.conf import urls


# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()
urlpatterns = urls.patterns(
    '',
    urls.url(r'^$', lambda r: http.HttpResponseRedirect('build_annotations/')),
    urls.url(r'^build_annotations/', urls.include(
        'build_annotations.urls',
        namespace='build_annotations')))
