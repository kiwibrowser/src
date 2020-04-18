# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-implements-statements
class Implements(object):
    """Implement class represents Implements statement in Web IDL spec."""

    def __init__(self, **kwargs):
        self._implementer_name = kwargs.pop('implementer_name')
        self._implementee_name = kwargs.pop('implementee_name')
        assert_no_extra_args(kwargs)

        if self.implementer_name == self.implementee_name:
            raise ValueError('Implements cannot refer same identifiers: %s' % self.implementer_name)

    @property
    def implementer_name(self):
        return self._implementer_name

    @property
    def implementee_name(self):
        return self._implementee_name
