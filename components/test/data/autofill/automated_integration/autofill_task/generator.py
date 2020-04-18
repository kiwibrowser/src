# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from random import choice
from string import ascii_lowercase


class Generator(object):
  """A string generator utility.
  """
  def __init__(self):
    super(Generator, self).__init__()

  @staticmethod
  def _lower_case_string(length=8):
    return ''.join(choice(ascii_lowercase) for i in range(length))

  @staticmethod
  def email():
    """Generates a fake email address.

    Format: 8 character string at an 8 character .com domain name

    Returns: The generated email address string.
    """
    return '%s@%s.com' % (Generator._lower_case_string(),
                          Generator._lower_case_string())

  @staticmethod
  def password():
    """Generates a fake password.

    Format: 8 character lowercase string plus 'A!234&'
      The postpended string exists to assist in satisfying common "secure
      password" requirements

    Returns: The generated password string.
    """
    return 'A!234&%s' % Generator._lower_case_string()
