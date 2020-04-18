# Copyright 2017 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Common gRPC implementation for Swarming and Isolate"""

import logging
import os
import re
import time
import types
import urlparse
from utils import net

# gRPC may not be installed on the worker machine. This is fine, as long as
# the bot doesn't attempt to use gRPC (checked in IsolateServerGrpc.__init__).
# Full external requirements are: grpcio, certifi.
try:
  import grpc
  from google import auth as google_auth
  from google.auth.transport import grpc as google_auth_transport_grpc
  from google.auth.transport import requests as google_auth_transport_requests
except ImportError as err:
  grpc = None


# If gRPC was successfully imported, try to import certifi as well.  This is not
# actually used anywhere in this module, but if certifi is missing,
# google.auth.transport will fail (see
# https://stackoverflow.com/questions/24973326). So checking it here allows us
# to print out a somewhat-sane error message.
certifi = None
if grpc is not None:
  try:
    import certifi
  except ImportError:
    # Will print out error messages later (ie when we have a logger)
    pass


# How many times to retry a gRPC call
MAX_GRPC_ATTEMPTS = 30


# Longest time to sleep between gRPC calls
MAX_GRPC_SLEEP = 10.


# Start the timeout at three minutes.
GRPC_TIMEOUT_SEC = 3 * 60


def available():
  """Returns true if gRPC can be used on this host."""
  return grpc != None


class Proxy(object):
  """Represents a gRPC proxy.

  If the proxy begins with 'https', the returned channel will be secure and
  authorized using default application credentials - see
  https://developers.google.com/identity/protocols/application-default-credentials.
  Currently, we're using Cloud Container Builder scopes for testing; this may
  change in the future to allow different scopes to be passed in for different
  channels.

  To use the returned channel to call methods directly, say:

    proxy = grpc_proxy.Proxy('https://grpc.luci.org/resource/prefix',
                             myapi_pb2.MyApiStub)

  To make a unary call with retries (recommended):

    proto_output = proxy.call_unary('MyMethod', proto_input)

  To make a unary call without retries, or to pass in a client side stream
  (proto_input can be an iterator here):

    proto_output = proxy.call_no_retries('MyMethod', proto_input)

  You can also call the stub directly (not recommended, since no errors will be
  caught or logged):

    proto_output = proxy.stub.MyMethod(proto_input)

  To make a call to a server-side streaming call (these are not retried):

    for response in proxy.get_stream('MyStreaminingMethod', proto_input):
      <process response>

  To retrieve the prefix:

    prefix = proxy.prefix # returns "prefix/for/resource/names"

  All exceptions are logged using "logging.warning."
  """

  def __init__(self, proxy, stub_class):
    self._verbose = os.environ.get('LUCI_GRPC_PROXY_VERBOSE')
    if self._verbose:
      logging.info('Enabled verbose mode for %s with stub %s',
                   proxy, stub_class.__name__)
    # NB: everything in url is unicode; convert to strings where
    # needed.
    url = urlparse.urlparse(proxy)
    if self._verbose:
      logging.info('Parsed URL for proxy is %r', url)
    if url.scheme == 'http':
      self._secure = False
    elif url.scheme == 'https':
      self._secure = True
    else:
      raise ValueError('gRPC proxy %s must use http[s], not %s' % (
          proxy, url.scheme))
    if url.netloc == '':
      raise ValueError('gRPC proxy is missing hostname: %s' % proxy)
    self._host = url.netloc
    self._prefix = url.path
    if self._prefix.endswith('/'):
      self._prefix = self._prefix[:-1]
    if self._prefix.startswith('/'):
      self._prefix = self._prefix[1:]
    if url.params != '' or url.fragment != '':
      raise ValueError('gRPC proxy may not contain params or fragments: %s' %
                       proxy)
    self._debug_info = ['full proxy name: ' + proxy]
    self._channel = self._create_channel()
    self._stub = stub_class(self._channel)
    logging.info('%s: initialized', self.name)
    if self._verbose:
      self._dump_proxy_info()

  @property
  def prefix(self):
    return self._prefix

  @property
  def channel(self):
    return self._channel

  @property
  def stub(self):
    return self._stub

  @property
  def name(self):
    security = 'insecure'
    if self._secure:
      security = 'secure'
    return 'gRPC %s proxy %s/%s' % (
        security, self._host, self._stub.__class__.__name__)

  def call_unary(self, name, request):
    """Calls a method, waiting if the service is not available.

    Note that it stores the request of generator type into a list for future
    retries, so it is not memory-friendly for streaming requests of large size.

    Usage: proto_output = proxy.call_unary('MyMethod', proto_input)
    """
    # If the request is a generator, store it to a list which can be reused in
    # retries.
    is_generator = False
    if isinstance(request, types.GeneratorType):
      request = list(request)
      is_generator = True

    for attempt in range(1, MAX_GRPC_ATTEMPTS+1):
      try:
        return self.call_no_retries(name, request
                                    if not is_generator else iter(request))
      except grpc.RpcError as g:
        if g.code() is not grpc.StatusCode.UNAVAILABLE:
          raise
        logging.warning('%s: call_grpc - proxy is unavailable (attempt %d/%d)',
                        self.name, attempt, MAX_GRPC_ATTEMPTS)
        # Save the error in case we need to return it
        grpc_error = g
        time.sleep(net.calculate_sleep_before_retry(attempt, MAX_GRPC_SLEEP))
    # If we get here, it must be because we got (and saved) an error
    assert grpc_error is not None
    raise grpc_error

  def get_stream(self, name, request):
    """Calls a server-side streaming method, returning an iterator.

    Usage: for resp in proxy.get_stream('MyMethod', proto_input'):
    """
    stream = self.call_no_retries(name, request)
    while True:
      # The lambda "next(stream, 1)" will return a protobuf on success, or the
      # integer 1 if the stream has ended. This allows us to avoid attempting
      # to catch StopIteration, which gets logged by _wrap_grpc_operation.
      response = self._wrap_grpc_operation(name + ' pull from stream',
                                           lambda: next(stream, 1))
      if isinstance(response, int):
        # Iteration is finished
        return
      yield response

  def call_no_retries(self, name, request):
    """Calls a method without any retries.

    Recommended for client-side streaming or nonidempotent unary calls.
    """
    method = getattr(self._stub, name)
    if method is None:
      raise NameError('%s: "%s" is not a valid method name', self.name, name)
    return self._wrap_grpc_operation(
        name, lambda: method(request, timeout=GRPC_TIMEOUT_SEC))

  def _wrap_grpc_operation(self, name, fn):
    """Wraps a gRPC operation (call or iterator increment) for logging."""
    if self._verbose:
      logging.info('%s/%s - starting gRPC operation', self.name, name)
    try:
      return fn()
    except grpc.RpcError as g:
      logging.warning('\n\nFailure in %s/%s: gRPC error %s', self.name, name, g)
      self._dump_proxy_info()
      raise g
    except Exception as e:
      logging.warning('\n\nFailure in %s/%s: exception %s', self.name, name, e)
      self._dump_proxy_info()
      raise e
    finally:
      if self._verbose:
        logging.info('%s/%s - finished gRPC operation', self.name, name)

  def _dump_proxy_info(self):
    logging.warning('DETAILED PROXY INFO')
    logging.warning('prefix = %s', self.prefix)
    logging.warning('debug info:\n\t%s\n\n',
                    '\n\t'.join(self._debug_info))

  def _create_channel(self):
    # Make sure grpc was successfully imported
    assert available()

    if not self._secure:
      return grpc.insecure_channel(self._host)

    # Authenticate the host.
    #
    # You're allowed to override the root certs and server if necessary. For
    # example, if you're running your proxy on localhost, you'll need to set
    # GRPC_PROXY_TLS_ROOTS to the "roots.crt" file specifying the certificate
    # for the root CA that your localhost server has used to certify itself, and
    # the GRPC_PROXY_TLS_OVERRIDE to the name that your server is using to
    # identify itself. For example, the ROOTS env var might be
    # "/path/to/roots.crt" while the OVERRIDE env var might be "test_server," if
    # this is what's used by the server you're running.
    #
    # If you're connecting to a real server with real SSL, none of this should
    # be used.
    if not certifi:
      self._debug_info.append('CERTIFI IS NOT PRESENT;' +
                              ' gRPC HTTPS CONNECTIONS MAY FAIL')
    root_certs = None
    roots = os.environ.get('LUCI_GRPC_PROXY_TLS_ROOTS')
    if roots:
      self._debug_info.append('Overridden root CA: %s' % roots)
      with open(roots) as f:
        root_certs = f.read()
    else:
      self._debug_info.append('Using default root CAs from certifi')
    overd = os.environ.get('LUCI_GRPC_PROXY_TLS_OVERRIDE')
    options = ()
    if overd:
      options=(('grpc.ssl_target_name_override', overd),)
    ssl_creds = grpc.ssl_channel_credentials(root_certificates=root_certs)

    # Authenticate the user.
    scopes = ('https://www.googleapis.com/auth/cloud-source-tools',)
    self._debug_info.append('Scopes are: %r' % scopes)
    user_creds, _ = google_auth.default(scopes=scopes)

    # Create the channel.
    request = google_auth_transport_requests.Request()
    if len(options) > 0:
      self._debug_info.append('Options are: %r' % options)
    else:
      self._debug_info.append('No options used')
    return google_auth_transport_grpc.secure_authorized_channel(
        user_creds, request, self._host, ssl_creds, options=options)
