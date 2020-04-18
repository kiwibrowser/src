# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class ExpectationFailure(Exception):
  """Represents an unsatisfied expectation.
  """
  def __init__(self, *args,**kwargs):
    super(ExpectationFailure, self).__init__(*args, **kwargs)
