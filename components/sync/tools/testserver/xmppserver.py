# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A bare-bones and non-compliant XMPP server.

Just enough of the protocol is implemented to get it to work with
Chrome's sync notification system.
"""

import asynchat
import asyncore
import base64
import re
import socket
from xml.dom import minidom

# pychecker complains about the use of fileno(), which is implemented
# by asyncore by forwarding to an internal object via __getattr__.
__pychecker__ = 'no-classattr'


class Error(Exception):
  """Error class for this module."""
  pass


class UnexpectedXml(Error):
  """Raised when an unexpected XML element has been encountered."""

  def __init__(self, xml_element):
    xml_text = xml_element.toxml()
    Error.__init__(self, 'Unexpected XML element', xml_text)


def ParseXml(xml_string):
  """Parses the given string as XML and returns a minidom element
  object.
  """
  dom = minidom.parseString(xml_string)

  # minidom handles xmlns specially, but there's a bug where it sets
  # the attribute value to None, which causes toxml() or toprettyxml()
  # to break.
  def FixMinidomXmlnsBug(xml_element):
    if xml_element.getAttribute('xmlns') is None:
      xml_element.setAttribute('xmlns', '')

  def ApplyToAllDescendantElements(xml_element, fn):
    fn(xml_element)
    for node in xml_element.childNodes:
      if node.nodeType == node.ELEMENT_NODE:
        ApplyToAllDescendantElements(node, fn)

  root = dom.documentElement
  ApplyToAllDescendantElements(root, FixMinidomXmlnsBug)
  return root


def CloneXml(xml):
  """Returns a deep copy of the given XML element.

  Args:
    xml: The XML element, which should be something returned from
         ParseXml() (i.e., a root element).
  """
  return xml.ownerDocument.cloneNode(True).documentElement


class StanzaParser(object):
  """A hacky incremental XML parser.

  StanzaParser consumes data incrementally via FeedString() and feeds
  its delegate complete parsed stanzas (i.e., XML documents) via
  FeedStanza().  Any stanzas passed to FeedStanza() are unlinked after
  the callback is done.

  Use like so:

  class MyClass(object):
    ...
    def __init__(self, ...):
      ...
      self._parser = StanzaParser(self)
      ...

    def SomeFunction(self, ...):
      ...
      self._parser.FeedString(some_data)
      ...

    def FeedStanza(self, stanza):
      ...
      print stanza.toprettyxml()
      ...
  """

  # NOTE(akalin): The following regexps are naive, but necessary since
  # none of the existing Python 2.4/2.5 XML libraries support
  # incremental parsing.  This works well enough for our purposes.
  #
  # The regexps below assume that any present XML element starts at
  # the beginning of the string, but there may be trailing whitespace.

  # Matches an opening stream tag (e.g., '<stream:stream foo="bar">')
  # (assumes that the stream XML namespace is defined in the tag).
  _stream_re = re.compile(r'^(<stream:stream [^>]*>)\s*')

  # Matches an empty element tag (e.g., '<foo bar="baz"/>').
  _empty_element_re = re.compile(r'^(<[^>]*/>)\s*')

  # Matches a non-empty element (e.g., '<foo bar="baz">quux</foo>').
  # Does *not* handle nested elements.
  _non_empty_element_re = re.compile(r'^(<([^ >]*)[^>]*>.*?</\2>)\s*')

  # The closing tag for a stream tag.  We have to insert this
  # ourselves since all XML stanzas are children of the stream tag,
  # which is never closed until the connection is closed.
  _stream_suffix = '</stream:stream>'

  def __init__(self, delegate):
    self._buffer = ''
    self._delegate = delegate

  def FeedString(self, data):
    """Consumes the given string data, possibly feeding one or more
    stanzas to the delegate.
    """
    self._buffer += data
    while (self._ProcessBuffer(self._stream_re, self._stream_suffix) or
           self._ProcessBuffer(self._empty_element_re) or
           self._ProcessBuffer(self._non_empty_element_re)):
      pass

  def _ProcessBuffer(self, regexp, xml_suffix=''):
    """If the buffer matches the given regexp, removes the match from
    the buffer, appends the given suffix, parses it, and feeds it to
    the delegate.

    Returns:
      Whether or not the buffer matched the given regexp.
    """
    results = regexp.match(self._buffer)
    if not results:
      return False
    xml_text = self._buffer[:results.end()] + xml_suffix
    self._buffer = self._buffer[results.end():]
    stanza = ParseXml(xml_text)
    self._delegate.FeedStanza(stanza)
    # Needed because stanza may have cycles.
    stanza.unlink()
    return True


class Jid(object):
  """Simple struct for an XMPP jid (essentially an e-mail address with
  an optional resource string).
  """

  def __init__(self, username, domain, resource=''):
    self.username = username
    self.domain = domain
    self.resource = resource

  def __str__(self):
    jid_str = "%s@%s" % (self.username, self.domain)
    if self.resource:
      jid_str += '/' + self.resource
    return jid_str

  def GetBareJid(self):
    return Jid(self.username, self.domain)


class IdGenerator(object):
  """Simple class to generate unique IDs for XMPP messages."""

  def __init__(self, prefix):
    self._prefix = prefix
    self._id = 0

  def GetNextId(self):
    next_id = "%s.%s" % (self._prefix, self._id)
    self._id += 1
    return next_id


class HandshakeTask(object):
  """Class to handle the initial handshake with a connected XMPP
  client.
  """

  # The handshake states in order.
  (_INITIAL_STREAM_NEEDED,
   _AUTH_NEEDED,
   _AUTH_STREAM_NEEDED,
   _BIND_NEEDED,
   _SESSION_NEEDED,
   _FINISHED) = range(6)

  # Used when in the _INITIAL_STREAM_NEEDED and _AUTH_STREAM_NEEDED
  # states.  Not an XML object as it's only the opening tag.
  #
  # The from and id attributes are filled in later.
  _STREAM_DATA = (
    '<stream:stream from="%s" id="%s" '
    'version="1.0" xmlns:stream="http://etherx.jabber.org/streams" '
    'xmlns="jabber:client">')

  # Used when in the _INITIAL_STREAM_NEEDED state.
  _AUTH_STANZA = ParseXml(
    '<stream:features xmlns:stream="http://etherx.jabber.org/streams">'
    '  <mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">'
    '    <mechanism>PLAIN</mechanism>'
    '    <mechanism>X-GOOGLE-TOKEN</mechanism>'
    '    <mechanism>X-OAUTH2</mechanism>'
    '  </mechanisms>'
    '</stream:features>')

  # Used when in the _AUTH_NEEDED state.
  _AUTH_SUCCESS_STANZA = ParseXml(
    '<success xmlns="urn:ietf:params:xml:ns:xmpp-sasl"/>')

  # Used when in the _AUTH_NEEDED state.
  _AUTH_FAILURE_STANZA = ParseXml(
    '<failure xmlns="urn:ietf:params:xml:ns:xmpp-sasl"/>')

  # Used when in the _AUTH_STREAM_NEEDED state.
  _BIND_STANZA = ParseXml(
    '<stream:features xmlns:stream="http://etherx.jabber.org/streams">'
    '  <bind xmlns="urn:ietf:params:xml:ns:xmpp-bind"/>'
    '  <session xmlns="urn:ietf:params:xml:ns:xmpp-session"/>'
    '</stream:features>')

  # Used when in the _BIND_NEEDED state.
  #
  # The id and jid attributes are filled in later.
  _BIND_RESULT_STANZA = ParseXml(
    '<iq id="" type="result">'
    '  <bind xmlns="urn:ietf:params:xml:ns:xmpp-bind">'
    '    <jid/>'
    '  </bind>'
    '</iq>')

  # Used when in the _SESSION_NEEDED state.
  #
  # The id attribute is filled in later.
  _IQ_RESPONSE_STANZA = ParseXml('<iq id="" type="result"/>')

  def __init__(self, connection, resource_prefix, authenticated):
    self._connection = connection
    self._id_generator = IdGenerator(resource_prefix)
    self._username = ''
    self._domain = ''
    self._jid = None
    self._authenticated = authenticated
    self._resource_prefix = resource_prefix
    self._state = self._INITIAL_STREAM_NEEDED

  def FeedStanza(self, stanza):
    """Inspects the given stanza and changes the handshake state if needed.

    Called when a stanza is received from the client.  Inspects the
    stanza to make sure it has the expected attributes given the
    current state, advances the state if needed, and sends a reply to
    the client if needed.
    """
    def ExpectStanza(stanza, name):
      if stanza.tagName != name:
        raise UnexpectedXml(stanza)

    def ExpectIq(stanza, type, name):
      ExpectStanza(stanza, 'iq')
      if (stanza.getAttribute('type') != type or
          stanza.firstChild.tagName != name):
        raise UnexpectedXml(stanza)

    def GetStanzaId(stanza):
      return stanza.getAttribute('id')

    def HandleStream(stanza):
      ExpectStanza(stanza, 'stream:stream')
      domain = stanza.getAttribute('to')
      if domain:
        self._domain = domain
      SendStreamData()

    def SendStreamData():
      next_id = self._id_generator.GetNextId()
      stream_data = self._STREAM_DATA % (self._domain, next_id)
      self._connection.SendData(stream_data)

    def GetUserDomain(stanza):
      encoded_username_password = stanza.firstChild.data
      username_password = base64.b64decode(encoded_username_password)
      (_, username_domain, _) = username_password.split('\0')
      # The domain may be omitted.
      #
      # If we were using python 2.5, we'd be able to do:
      #
      #   username, _, domain = username_domain.partition('@')
      #   if not domain:
      #     domain = self._domain
      at_pos = username_domain.find('@')
      if at_pos != -1:
        username = username_domain[:at_pos]
        domain = username_domain[at_pos+1:]
      else:
        username = username_domain
        domain = self._domain
      return (username, domain)

    def Finish():
      self._state = self._FINISHED
      self._connection.HandshakeDone(self._jid)

    if self._state == self._INITIAL_STREAM_NEEDED:
      HandleStream(stanza)
      self._connection.SendStanza(self._AUTH_STANZA, False)
      self._state = self._AUTH_NEEDED

    elif self._state == self._AUTH_NEEDED:
      ExpectStanza(stanza, 'auth')
      (self._username, self._domain) = GetUserDomain(stanza)
      if self._authenticated:
        self._connection.SendStanza(self._AUTH_SUCCESS_STANZA, False)
        self._state = self._AUTH_STREAM_NEEDED
      else:
        self._connection.SendStanza(self._AUTH_FAILURE_STANZA, False)
        Finish()

    elif self._state == self._AUTH_STREAM_NEEDED:
      HandleStream(stanza)
      self._connection.SendStanza(self._BIND_STANZA, False)
      self._state = self._BIND_NEEDED

    elif self._state == self._BIND_NEEDED:
      ExpectIq(stanza, 'set', 'bind')
      stanza_id = GetStanzaId(stanza)
      resource_element = stanza.getElementsByTagName('resource')[0]
      resource = resource_element.firstChild.data
      full_resource = '%s.%s' % (self._resource_prefix, resource)
      response = CloneXml(self._BIND_RESULT_STANZA)
      response.setAttribute('id', stanza_id)
      self._jid = Jid(self._username, self._domain, full_resource)
      jid_text = response.parentNode.createTextNode(str(self._jid))
      response.getElementsByTagName('jid')[0].appendChild(jid_text)
      self._connection.SendStanza(response)
      self._state = self._SESSION_NEEDED

    elif self._state == self._SESSION_NEEDED:
      ExpectIq(stanza, 'set', 'session')
      stanza_id = GetStanzaId(stanza)
      xml = CloneXml(self._IQ_RESPONSE_STANZA)
      xml.setAttribute('id', stanza_id)
      self._connection.SendStanza(xml)
      Finish()


def AddrString(addr):
  return '%s:%d' % addr


class XmppConnection(asynchat.async_chat):
  """A single XMPP client connection.

  This class handles the connection to a single XMPP client (via a
  socket).  It does the XMPP handshake and also implements the (old)
  Google notification protocol.
  """

  # Used for acknowledgements to the client.
  #
  # The from and id attributes are filled in later.
  _IQ_RESPONSE_STANZA = ParseXml('<iq from="" id="" type="result"/>')

  def __init__(self, sock, socket_map, delegate, addr, authenticated):
    """Starts up the xmpp connection.

    Args:
      sock: The socket to the client.
      socket_map: A map from sockets to their owning objects.
      delegate: The delegate, which is notified when the XMPP
        handshake is successful, when the connection is closed, and
        when a notification has to be broadcast.
      addr: The host/port of the client.
    """
    # We do this because in versions of python < 2.6,
    # async_chat.__init__ doesn't take a map argument nor pass it to
    # dispatcher.__init__.  We rely on the fact that
    # async_chat.__init__ calls dispatcher.__init__ as the last thing
    # it does, and that calling dispatcher.__init__ with socket=None
    # and map=None is essentially a no-op.
    asynchat.async_chat.__init__(self)
    asyncore.dispatcher.__init__(self, sock, socket_map)

    self.set_terminator(None)

    self._delegate = delegate
    self._parser = StanzaParser(self)
    self._jid = None

    self._addr = addr
    addr_str = AddrString(self._addr)
    self._handshake_task = HandshakeTask(self, addr_str, authenticated)
    print 'Starting connection to %s' % self

  def __str__(self):
    if self._jid:
      return str(self._jid)
    else:
      return AddrString(self._addr)

  # async_chat implementation.

  def collect_incoming_data(self, data):
    self._parser.FeedString(data)

  # This is only here to make pychecker happy.
  def found_terminator(self):
    asynchat.async_chat.found_terminator(self)

  def close(self):
    print "Closing connection to %s" % self
    self._delegate.OnXmppConnectionClosed(self)
    asynchat.async_chat.close(self)

  # Called by self._parser.FeedString().
  def FeedStanza(self, stanza):
    if self._handshake_task:
      self._handshake_task.FeedStanza(stanza)
    elif stanza.tagName == 'iq' and stanza.getAttribute('type') == 'result':
      # Ignore all client acks.
      pass
    elif (stanza.firstChild and
          stanza.firstChild.namespaceURI == 'google:push'):
      self._HandlePushCommand(stanza)
    else:
      raise UnexpectedXml(stanza)

  # Called by self._handshake_task.
  def HandshakeDone(self, jid):
    if jid:
      self._jid = jid
      self._handshake_task = None
      self._delegate.OnXmppHandshakeDone(self)
      print "Handshake done for %s" % self
    else:
      print "Handshake failed for %s" % self
      self.close()

  def _HandlePushCommand(self, stanza):
    if stanza.tagName == 'iq' and stanza.firstChild.tagName == 'subscribe':
      # Subscription request.
      self._SendIqResponseStanza(stanza)
    elif stanza.tagName == 'message' and stanza.firstChild.tagName == 'push':
      # Send notification request.
      self._delegate.ForwardNotification(self, stanza)
    else:
      raise UnexpectedXml(command_xml)

  def _SendIqResponseStanza(self, iq):
    stanza = CloneXml(self._IQ_RESPONSE_STANZA)
    stanza.setAttribute('from', str(self._jid.GetBareJid()))
    stanza.setAttribute('id', iq.getAttribute('id'))
    self.SendStanza(stanza)

  def SendStanza(self, stanza, unlink=True):
    """Sends a stanza to the client.

    Args:
      stanza: The stanza to send.
      unlink: Whether to unlink stanza after sending it. (Pass in
      False if stanza is a constant.)
    """
    self.SendData(stanza.toxml())
    if unlink:
      stanza.unlink()

  def SendData(self, data):
    """Sends raw data to the client.
    """
    # We explicitly encode to ascii as that is what the client expects
    # (some minidom library functions return unicode strings).
    self.push(data.encode('ascii'))

  def ForwardNotification(self, notification_stanza):
    """Forwards a notification to the client."""
    notification_stanza.setAttribute('from', str(self._jid.GetBareJid()))
    notification_stanza.setAttribute('to', str(self._jid))
    self.SendStanza(notification_stanza, False)


class XmppServer(asyncore.dispatcher):
  """The main XMPP server class.

  The XMPP server starts accepting connections on the given address
  and spawns off XmppConnection objects for each one.

  Use like so:

    socket_map = {}
    xmpp_server = xmppserver.XmppServer(socket_map, ('127.0.0.1', 5222))
    asyncore.loop(30.0, False, socket_map)
  """

  # Used when sending a notification.
  _NOTIFICATION_STANZA = ParseXml(
    '<message>'
    '  <push xmlns="google:push">'
    '    <data/>'
    '  </push>'
    '</message>')

  def __init__(self, socket_map, addr):
    asyncore.dispatcher.__init__(self, None, socket_map)
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.set_reuse_addr()
    self.bind(addr)
    self.listen(5)
    self._socket_map = socket_map
    self._connections = set()
    self._handshake_done_connections = set()
    self._notifications_enabled = True
    self._authenticated = True

  def handle_accept(self):
    (sock, addr) = self.accept()
    xmpp_connection = XmppConnection(
      sock, self._socket_map, self, addr, self._authenticated)
    self._connections.add(xmpp_connection)
    # Return the new XmppConnection for testing.
    return xmpp_connection

  def close(self):
    # A copy is necessary since calling close on each connection
    # removes it from self._connections.
    for connection in self._connections.copy():
      connection.close()
    asyncore.dispatcher.close(self)

  def EnableNotifications(self):
    self._notifications_enabled = True

  def DisableNotifications(self):
    self._notifications_enabled = False

  def MakeNotification(self, channel, data):
    """Makes a notification from the given channel and encoded data.

    Args:
      channel: The channel on which to send the notification.
      data: The notification payload.
    """
    notification_stanza = CloneXml(self._NOTIFICATION_STANZA)
    push_element = notification_stanza.getElementsByTagName('push')[0]
    push_element.setAttribute('channel', channel)
    data_element = push_element.getElementsByTagName('data')[0]
    encoded_data = base64.b64encode(data)
    data_text = notification_stanza.parentNode.createTextNode(encoded_data)
    data_element.appendChild(data_text)
    return notification_stanza

  def SendNotification(self, channel, data):
    """Sends a notification to all connections.

    Args:
      channel: The channel on which to send the notification.
      data: The notification payload.
    """
    notification_stanza = self.MakeNotification(channel, data)
    self.ForwardNotification(None, notification_stanza)
    notification_stanza.unlink()

  def SetAuthenticated(self, auth_valid):
    self._authenticated = auth_valid

    # We check authentication only when establishing new connections.  We close
    # all existing connections here to make sure previously connected clients
    # pick up on the change.  It's a hack, but it works well enough for our
    # purposes.
    if not self._authenticated:
      for connection in self._handshake_done_connections:
        connection.close()

  def GetAuthenticated(self):
    return self._authenticated

  # XmppConnection delegate methods.
  def OnXmppHandshakeDone(self, xmpp_connection):
    self._handshake_done_connections.add(xmpp_connection)

  def OnXmppConnectionClosed(self, xmpp_connection):
    self._connections.discard(xmpp_connection)
    self._handshake_done_connections.discard(xmpp_connection)

  def ForwardNotification(self, unused_xmpp_connection, notification_stanza):
    if self._notifications_enabled:
      for connection in self._handshake_done_connections:
        print 'Sending notification to %s' % connection
        connection.ForwardNotification(notification_stanza)
    else:
      print 'Notifications disabled; dropping notification'
