# Copyright 2014 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import logging
import urlparse

import httpserver_mock


class CipdServerHandler(httpserver_mock.MockHandler):
  """An extremely minimal implementation of the cipd server API v1.0."""

  ### Mocked HTTP Methods

  def do_GET(self):
    logging.info('GET %s', self.path)
    if self.path == '/auth/api/v1/server/oauth_config':
      self.send_json({
        'client_id': 'c',
        'client_not_so_secret': 's',
        'primary_url': self.server.url})
    elif self.path.startswith('/_ah/api/repo/v1/instance/resolve?'):
      self.send_json({
        'status': 'SUCCESS',
        'instance_id': 'a' * 40,
      })
    elif self.path.startswith('/_ah/api/repo/v1/client?'):
      qs = urlparse.parse_qs(urlparse.urlparse(self.path).query)
      pkg_name = qs.get('package_name', [])
      if not pkg_name:
        self.send_json({
          'status': 'FAILED',
          'error_message': 'package_name not specified',
        })
      if '$' in pkg_name[0]:
        self.send_json({
          'status': 'FAILED',
          'error_message': 'unknown package %r' % pkg_name[0],
        })
      else:
        self.send_json({
          'status': 'SUCCESS',
          'client_binary': {
            'fetch_url': self.server.url + '/fake_google_storage/cipd_client',
          },
        })
    elif self.path == '/fake_google_storage/cipd_client':
      # The content is not actually used because run_isolated_test.py
      # mocks popen.
      self.send_octet_stream('#!/usr/sh\n')
    else:
      raise NotImplementedError(self.path)


class MockCipdServer(httpserver_mock.MockServer):
  _HANDLER_CLS = CipdServerHandler
