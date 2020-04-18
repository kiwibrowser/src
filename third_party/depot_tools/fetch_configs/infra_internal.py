# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

import config_util  # pylint: disable=import-error


# This class doesn't need an __init__ method, so we disable the warning
# pylint: disable=no-init
class InfraInternal(config_util.Config):
  """Basic Config class for the whole set of Infrastructure repositories."""

  @staticmethod
  def fetch_spec(_props):
    def url(host, repo):
      return 'https://%s.googlesource.com/%s.git' % (host, repo)

    spec = {
      'solutions': [
        {
          'name': 'infra_internal',
          'url': url('chrome-internal', 'infra/infra_internal'),
          'managed': False
        },
      ],
    }
    return {
        'type': 'gclient_git',
        'gclient_git_spec': spec,
    }

  @staticmethod
  def expected_root(_props):
    return 'infra_internal'


def main(argv=None):
  return InfraInternal().handle_args(argv)


if __name__ == '__main__':
  sys.exit(main(sys.argv))

