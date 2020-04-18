#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a python sync server used for testing Chrome Sync.

By default, it listens on an ephemeral port and xmpp_port and sends the port
numbers back to the originating process over a pipe. The originating process can
specify an explicit port and xmpp_port if necessary.
"""

import asyncore
import BaseHTTPServer
import errno
import gzip
import os
import select
import StringIO
import socket
import sys
import urlparse

import chromiumsync
import echo_message
import testserver_base
import xmppserver


class SyncHTTPServer(testserver_base.ClientRestrictingServerMixIn,
                     testserver_base.BrokenPipeHandlerMixIn,
                     testserver_base.StoppableHTTPServer):
  """An HTTP server that handles sync commands."""

  def __init__(self, server_address, xmpp_port, request_handler_class):
    testserver_base.StoppableHTTPServer.__init__(self,
                                                 server_address,
                                                 request_handler_class)
    self._sync_handler = chromiumsync.TestServer()
    self._xmpp_socket_map = {}
    self._xmpp_server = xmppserver.XmppServer(
      self._xmpp_socket_map, ('localhost', xmpp_port))
    self.xmpp_port = self._xmpp_server.getsockname()[1]
    self.authenticated = True

  def GetXmppServer(self):
    return self._xmpp_server

  def HandleCommand(self, query, raw_request):
    return self._sync_handler.HandleCommand(query, raw_request)

  def HandleRequestNoBlock(self):
    """Handles a single request.

    Copied from SocketServer._handle_request_noblock().
    """

    try:
      request, client_address = self.get_request()
    except socket.error:
      return
    if self.verify_request(request, client_address):
      try:
        self.process_request(request, client_address)
      except Exception:
        self.handle_error(request, client_address)
        self.close_request(request)

  def SetAuthenticated(self, auth_valid):
    self.authenticated = auth_valid

  def GetAuthenticated(self):
    return self.authenticated

  def handle_request(self):
    """Adaptation of asyncore.loop"""
    def HandleXmppSocket(fd, socket_map, handler):
      """Runs the handler for the xmpp connection for fd.

      Adapted from asyncore.read() et al.
      """

      xmpp_connection = socket_map.get(fd)
      # This could happen if a previous handler call caused fd to get
      # removed from socket_map.
      if xmpp_connection is None:
        return
      try:
        handler(xmpp_connection)
      except (asyncore.ExitNow, KeyboardInterrupt, SystemExit):
        raise
      except:
        xmpp_connection.handle_error()

    read_fds = [ self.fileno() ]
    write_fds = []
    exceptional_fds = []

    for fd, xmpp_connection in self._xmpp_socket_map.items():
      is_r = xmpp_connection.readable()
      is_w = xmpp_connection.writable()
      if is_r:
        read_fds.append(fd)
      if is_w:
        write_fds.append(fd)
      if is_r or is_w:
        exceptional_fds.append(fd)

    try:
      read_fds, write_fds, exceptional_fds = (
        select.select(read_fds, write_fds, exceptional_fds))
    except select.error, err:
      if err.args[0] != errno.EINTR:
        raise
      else:
        return

    for fd in read_fds:
      if fd == self.fileno():
        self.HandleRequestNoBlock()
        return
      HandleXmppSocket(fd, self._xmpp_socket_map,
                       asyncore.dispatcher.handle_read_event)

    for fd in write_fds:
      HandleXmppSocket(fd, self._xmpp_socket_map,
                       asyncore.dispatcher.handle_write_event)

    for fd in exceptional_fds:
      HandleXmppSocket(fd, self._xmpp_socket_map,
                       asyncore.dispatcher.handle_expt_event)


class SyncPageHandler(testserver_base.BasePageHandler):
  """Handler for the main HTTP sync server."""

  def __init__(self, request, client_address, sync_http_server):
    get_handlers = [self.ChromiumSyncTimeHandler,
                    self.ChromiumSyncMigrationOpHandler,
                    self.ChromiumSyncCredHandler,
                    self.ChromiumSyncXmppCredHandler,
                    self.ChromiumSyncDisableNotificationsOpHandler,
                    self.ChromiumSyncEnableNotificationsOpHandler,
                    self.ChromiumSyncSendNotificationOpHandler,
                    self.ChromiumSyncBirthdayErrorOpHandler,
                    self.ChromiumSyncTransientErrorOpHandler,
                    self.ChromiumSyncErrorOpHandler,
                    self.ChromiumSyncSyncTabFaviconsOpHandler,
                    self.ChromiumSyncCreateSyncedBookmarksOpHandler,
                    self.ChromiumSyncEnableKeystoreEncryptionOpHandler,
                    self.ChromiumSyncRotateKeystoreKeysOpHandler,
                    self.ChromiumSyncEnableManagedUserAcknowledgementHandler,
                    self.ChromiumSyncEnablePreCommitGetUpdateAvoidanceHandler,
                    self.GaiaOAuth2TokenHandler,
                    self.GaiaSetOAuth2TokenResponseHandler,
                    self.CustomizeClientCommandHandler]

    post_handlers = [self.ChromiumSyncCommandHandler,
                     self.ChromiumSyncTimeHandler,
                     self.GaiaOAuth2TokenHandler,
                     self.GaiaSetOAuth2TokenResponseHandler]
    testserver_base.BasePageHandler.__init__(self, request, client_address,
                                             sync_http_server, [], get_handlers,
                                             [], post_handlers, [])


  def ChromiumSyncTimeHandler(self):
    """Handle Chromium sync .../time requests.

    The syncer sometimes checks server reachability by examining /time.
    """

    test_name = "/chromiumsync/time"
    if not self._ShouldHandleRequest(test_name):
      return False

    # Chrome hates it if we send a response before reading the request.
    if self.headers.getheader('content-length'):
      length = int(self.headers.getheader('content-length'))
      _raw_request = self.rfile.read(length)

    self.send_response(200)
    self.send_header('Content-Type', 'text/plain')
    self.end_headers()
    self.wfile.write('0123456789')
    return True

  def ChromiumSyncCommandHandler(self):
    """Handle a chromiumsync command arriving via http.

    This covers all sync protocol commands: authentication, getupdates, and
    commit.
    """

    test_name = "/chromiumsync/command"
    if not self._ShouldHandleRequest(test_name):
      return False

    length = int(self.headers.getheader('content-length'))
    raw_request = self.rfile.read(length)
    if self.headers.getheader('Content-Encoding'):
      encode = self.headers.getheader('Content-Encoding')
      if encode == "gzip":
        raw_request = gzip.GzipFile(
            fileobj=StringIO.StringIO(raw_request)).read()

    http_response = 200
    raw_reply = None
    if not self.server.GetAuthenticated():
      http_response = 401
      challenge = 'GoogleLogin realm="http://%s", service="chromiumsync"' % (
        self.server.server_address[0])
    else:
      http_response, raw_reply = self.server.HandleCommand(
          self.path, raw_request)

    ### Now send the response to the client. ###
    self.send_response(http_response)
    if http_response == 401:
      self.send_header('www-Authenticate', challenge)
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncMigrationOpHandler(self):
    test_name = "/chromiumsync/migrate"
    if not self._ShouldHandleRequest(test_name):
      return False

    http_response, raw_reply = self.server._sync_handler.HandleMigrate(
        self.path)
    self.send_response(http_response)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncCredHandler(self):
    test_name = "/chromiumsync/cred"
    if not self._ShouldHandleRequest(test_name):
      return False
    try:
      query = urlparse.urlparse(self.path)[4]
      cred_valid = urlparse.parse_qs(query)['valid']
      if cred_valid[0] == 'True':
        self.server.SetAuthenticated(True)
      else:
        self.server.SetAuthenticated(False)
    except Exception:
      self.server.SetAuthenticated(False)

    http_response = 200
    raw_reply = 'Authenticated: %s ' % self.server.GetAuthenticated()
    self.send_response(http_response)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncXmppCredHandler(self):
    test_name = "/chromiumsync/xmppcred"
    if not self._ShouldHandleRequest(test_name):
      return False
    xmpp_server = self.server.GetXmppServer()
    try:
      query = urlparse.urlparse(self.path)[4]
      cred_valid = urlparse.parse_qs(query)['valid']
      if cred_valid[0] == 'True':
        xmpp_server.SetAuthenticated(True)
      else:
        xmpp_server.SetAuthenticated(False)
    except:
      xmpp_server.SetAuthenticated(False)

    http_response = 200
    raw_reply = 'XMPP Authenticated: %s ' % xmpp_server.GetAuthenticated()
    self.send_response(http_response)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncDisableNotificationsOpHandler(self):
    test_name = "/chromiumsync/disablenotifications"
    if not self._ShouldHandleRequest(test_name):
      return False
    self.server.GetXmppServer().DisableNotifications()
    result = 200
    raw_reply = ('<html><title>Notifications disabled</title>'
                 '<H1>Notifications disabled</H1></html>')
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnableNotificationsOpHandler(self):
    test_name = "/chromiumsync/enablenotifications"
    if not self._ShouldHandleRequest(test_name):
      return False
    self.server.GetXmppServer().EnableNotifications()
    result = 200
    raw_reply = ('<html><title>Notifications enabled</title>'
                 '<H1>Notifications enabled</H1></html>')
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncSendNotificationOpHandler(self):
    test_name = "/chromiumsync/sendnotification"
    if not self._ShouldHandleRequest(test_name):
      return False
    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)
    channel = ''
    data = ''
    if 'channel' in query_params:
      channel = query_params['channel'][0]
    if 'data' in query_params:
      data = query_params['data'][0]
    self.server.GetXmppServer().SendNotification(channel, data)
    result = 200
    raw_reply = ('<html><title>Notification sent</title>'
                 '<H1>Notification sent with channel "%s" '
                 'and data "%s"</H1></html>'
                 % (channel, data))
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncBirthdayErrorOpHandler(self):
    test_name = "/chromiumsync/birthdayerror"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleCreateBirthdayError()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncTransientErrorOpHandler(self):
    test_name = "/chromiumsync/transienterror"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleSetTransientError()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncErrorOpHandler(self):
    test_name = "/chromiumsync/error"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleSetInducedError(
        self.path)
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncSyncTabFaviconsOpHandler(self):
    test_name = "/chromiumsync/synctabfavicons"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleSetSyncTabFavicons()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncCreateSyncedBookmarksOpHandler(self):
    test_name = "/chromiumsync/createsyncedbookmarks"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleCreateSyncedBookmarks()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnableKeystoreEncryptionOpHandler(self):
    test_name = "/chromiumsync/enablekeystoreencryption"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleEnableKeystoreEncryption())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncRotateKeystoreKeysOpHandler(self):
    test_name = "/chromiumsync/rotatekeystorekeys"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleRotateKeystoreKeys())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnableManagedUserAcknowledgementHandler(self):
    test_name = "/chromiumsync/enablemanageduseracknowledgement"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleEnableManagedUserAcknowledgement())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnablePreCommitGetUpdateAvoidanceHandler(self):
    test_name = "/chromiumsync/enableprecommitgetupdateavoidance"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleEnablePreCommitGetUpdateAvoidance())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def GaiaOAuth2TokenHandler(self):
    test_name = "/o/oauth2/token"
    if not self._ShouldHandleRequest(test_name):
      return False
    if self.headers.getheader('content-length'):
      length = int(self.headers.getheader('content-length'))
      _raw_request = self.rfile.read(length)
    result, raw_reply = (
        self.server._sync_handler.HandleGetOauth2Token())
    self.send_response(result)
    self.send_header('Content-Type', 'application/json')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def GaiaSetOAuth2TokenResponseHandler(self):
    test_name = "/setfakeoauth2token"
    if not self._ShouldHandleRequest(test_name):
      return False

    # The index of 'query' is 4.
    # See http://docs.python.org/2/library/urlparse.html
    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)

    response_code = 0
    request_token = ''
    access_token = ''
    expires_in = 0
    token_type = ''

    if 'response_code' in query_params:
      response_code = query_params['response_code'][0]
    if 'request_token' in query_params:
      request_token = query_params['request_token'][0]
    if 'access_token' in query_params:
      access_token = query_params['access_token'][0]
    if 'expires_in' in query_params:
      expires_in = query_params['expires_in'][0]
    if 'token_type' in query_params:
      token_type = query_params['token_type'][0]

    result, raw_reply = (
        self.server._sync_handler.HandleSetOauth2Token(
            response_code, request_token, access_token, expires_in, token_type))
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def CustomizeClientCommandHandler(self):
    test_name = "/customizeclientcommand"
    if not self._ShouldHandleRequest(test_name):
      return False

    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)

    if 'sessions_commit_delay_seconds' in query_params:
      sessions_commit_delay = query_params['sessions_commit_delay_seconds'][0]
      try:
        command_string = self.server._sync_handler.CustomizeClientCommand(
            int(sessions_commit_delay))
        response_code = 200
        reply = "The ClientCommand was customized:\n\n"
        reply += "<code>{}</code>.".format(command_string)
      except ValueError:
        response_code = 400
        reply = "sessions_commit_delay_seconds was not an int"
    else:
      response_code = 400
      reply = "sessions_commit_delay_seconds is required"

    self.send_response(response_code)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(reply))
    self.end_headers()
    self.wfile.write(reply)
    return True

class SyncServerRunner(testserver_base.TestServerRunner):
  """TestServerRunner for the net test servers."""

  def __init__(self):
    super(SyncServerRunner, self).__init__()

  def create_server(self, server_data):
    port = self.options.port
    host = self.options.host
    xmpp_port = self.options.xmpp_port
    server = SyncHTTPServer((host, port), xmpp_port, SyncPageHandler)
    print ('Sync HTTP server started at %s:%d/chromiumsync...' %
           (host, server.server_port))
    print ('Fake OAuth2 Token server started at %s:%d/o/oauth2/token...' %
           (host, server.server_port))
    print ('Sync XMPP server started at %s:%d...' %
           (host, server.xmpp_port))
    server_data['port'] = server.server_port
    server_data['xmpp_port'] = server.xmpp_port
    return server

  def run_server(self):
    testserver_base.TestServerRunner.run_server(self)

  def add_options(self):
    testserver_base.TestServerRunner.add_options(self)
    self.option_parser.add_option('--xmpp-port', default='0', type='int',
                                  help='Port used by the XMPP server. If '
                                  'unspecified, the XMPP server will listen on '
                                  'an ephemeral port.')
    # Override the default logfile name used in testserver.py.
    self.option_parser.set_defaults(log_file='sync_testserver.log')

if __name__ == '__main__':
  sys.exit(SyncServerRunner().main())
