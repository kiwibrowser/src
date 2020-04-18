# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Dispatches requests to request handler classes."""

import webapp2

from dashboard.pinpoint import handlers


_URL_MAPPING = [
    # Public API.
    webapp2.Route(r'/api/config', handlers.Config),
    webapp2.Route(r'/api/generate-results2/<job_id>',
                  handlers.Results2Generator),
    webapp2.Route(r'/api/isolate', handlers.Isolate),
    webapp2.Route(r'/api/isolate/<builder_name>/<git_hash>/<target>',
                  handlers.Isolate),
    webapp2.Route(r'/api/job/<job_id>', handlers.Job),
    webapp2.Route(r'/api/jobs', handlers.Jobs),
    webapp2.Route(r'/api/migrate', handlers.Migrate),
    webapp2.Route(r'/api/new', handlers.New),
    webapp2.Route(r'/api/results2/<job_id>', handlers.Results2),
    webapp2.Route(r'/api/stats', handlers.Stats),

    # Used internally by Pinpoint. Not accessible from the public API.
    webapp2.Route(r'/api/run/<job_id>', handlers.Run),
]

APP = webapp2.WSGIApplication(_URL_MAPPING, debug=False)
