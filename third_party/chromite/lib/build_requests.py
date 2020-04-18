# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes to manage build requests."""

from __future__ import print_function

import collections


REASON_SANITY_PRE_CQ = 'sanity-pre-cq'
REASON_IMPORTANT_CQ_SLAVE = 'important_cq_slave'
REASON_EXPERIMENTAL_CQ_SLAVE = 'experimental_cq_slave'

# Valid build request reasons
BUILD_REQUEST_REASONS = (
    REASON_SANITY_PRE_CQ,
    REASON_IMPORTANT_CQ_SLAVE,
    REASON_EXPERIMENTAL_CQ_SLAVE)

BUILD_REQUEST_COLUMNS = (
    'id', 'build_id', 'request_build_config', 'request_build_args',
    'request_buildbucket_id', 'request_reason', 'timestamp')

# A namedtuple representing a BuildRequest record as recorded in cidb.
BuildRequest = collections.namedtuple('BuildRequest', BUILD_REQUEST_COLUMNS)
